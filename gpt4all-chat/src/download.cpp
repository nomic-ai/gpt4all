#include "download.h"

#include "modellist.h"
#include "mysettings.h"
#include "network.h"

#include <QByteArray>
#include <QCollator>
#include <QCoreApplication>
#include <QDebug>
#include <QGlobalStatic>
#include <QGuiApplication>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLocale>
#include <QNetworkRequest>
#include <QPair>
#include <QSettings>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QStringList>
#include <QTextStream>
#include <QUrl>
#include <QVariant>
#include <QVector>
#include <Qt>
#include <QtLogging>

#include <algorithm>
#include <compare>
#include <cstddef>
#include <utility>

using namespace Qt::Literals::StringLiterals;

class MyDownload: public Download { };
Q_GLOBAL_STATIC(MyDownload, downloadInstance)
Download *Download::globalInstance()
{
    return downloadInstance();
}

Download::Download()
    : QObject(nullptr)
    , m_hashAndSave(new HashAndSaveFile)
{
    connect(this, &Download::requestHashAndSave, m_hashAndSave,
        &HashAndSaveFile::hashAndSave, Qt::QueuedConnection);
    connect(m_hashAndSave, &HashAndSaveFile::hashAndSaveFinished, this,
        &Download::handleHashAndSaveFinished, Qt::QueuedConnection);
    connect(&m_networkManager, &QNetworkAccessManager::sslErrors, this,
        &Download::handleSslErrors);
    updateLatestNews();
    updateReleaseNotes();
    m_startTime = QDateTime::currentDateTime();
}

std::strong_ordering Download::compareAppVersions(const QString &a, const QString &b)
{
    static QRegularExpression versionRegex(R"(^(\d+(?:\.\d+){0,2})(-.+)?$)");

    // When comparing versions, make sure a2 < a10.
    QCollator versionCollator(QLocale(QLocale::English, QLocale::UnitedStates));
    versionCollator.setNumericMode(true);

    QRegularExpressionMatch aMatch = versionRegex.match(a);
    QRegularExpressionMatch bMatch = versionRegex.match(b);

    Q_ASSERT(aMatch.hasMatch() && bMatch.hasMatch()); // expect valid versions

    // Check for an invalid version. foo < 3.0.0 -> !hasMatch < hasMatch
    if (auto diff = aMatch.hasMatch() <=> bMatch.hasMatch(); diff != 0)
        return diff; // invalid version compares as lower

    // Compare invalid versions. fooa < foob
    if (!aMatch.hasMatch() && !bMatch.hasMatch())
        return versionCollator.compare(a, b) <=> 0; // lexicographic comparison

    // Compare first three components. 3.0.0 < 3.0.1
    QStringList aParts = aMatch.captured(1).split('.');
    QStringList bParts = bMatch.captured(1).split('.');
    for (int i = 0; i < qMax(aParts.size(), bParts.size()); i++) {
        bool ok = false;
        int aInt = aParts.value(i, "0").toInt(&ok);
        Q_ASSERT(ok);
        int bInt = bParts.value(i, "0").toInt(&ok);
        Q_ASSERT(ok);
        if (auto diff = aInt <=> bInt; diff != 0)
            return diff; // version with lower component compares as lower
    }

    // Check for a pre/post-release suffix. 3.0.0-dev0 < 3.0.0-rc1 < 3.0.0 < 3.0.0-post1
    auto getSuffixOrder = [](const QRegularExpressionMatch &match) -> int {
        QString suffix = match.captured(2);
        return suffix.startsWith("-dev") ? 0 :
               suffix.startsWith("-rc")  ? 1 :
               suffix.isEmpty()          ? 2 :
               /* some other suffix */     3;
    };
    if (auto diff = getSuffixOrder(aMatch) <=> getSuffixOrder(bMatch); diff != 0)
        return diff; // different suffix types

    // Lexicographic comparison of suffix. 3.0.0-rc1 < 3.0.0-rc2
    if (aMatch.hasCaptured(2) && bMatch.hasCaptured(2)) {
        if (auto diff = versionCollator.compare(aMatch.captured(2), bMatch.captured(2)); diff != 0)
            return diff <=> 0;
    }

    return std::strong_ordering::equal;
}

ReleaseInfo Download::releaseInfo() const
{
    const QString currentVersion = QCoreApplication::applicationVersion();
    if (m_releaseMap.contains(currentVersion))
        return m_releaseMap.value(currentVersion);
    if (!m_releaseMap.empty())
        return m_releaseMap.last();
    return ReleaseInfo();
}

bool Download::hasNewerRelease() const
{
    const QString currentVersion = QCoreApplication::applicationVersion();
    for (const auto &version : m_releaseMap.keys()) {
        if (compareAppVersions(version, currentVersion) > 0)
            return true;
    }
    return false;
}

bool Download::isFirstStart(bool writeVersion) const
{
    auto *mySettings = MySettings::globalInstance();

    QSettings settings;
    QString lastVersionStarted = settings.value("download/lastVersionStarted").toString();
    bool first = lastVersionStarted != QCoreApplication::applicationVersion();
    if (first && writeVersion) {
        settings.setValue("download/lastVersionStarted", QCoreApplication::applicationVersion());
        // let the user select these again
        settings.remove("network/usageStatsActive");
        settings.remove("network/isActive");
        emit mySettings->networkUsageStatsActiveChanged();
        emit mySettings->networkIsActiveChanged();
    }

    return first || !mySettings->isNetworkUsageStatsActiveSet() || !mySettings->isNetworkIsActiveSet();
}

void Download::updateReleaseNotes()
{
    QUrl jsonUrl("http://gpt4all.io/meta/release.json");
    QNetworkRequest request(jsonUrl);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);
    QNetworkReply *jsonReply = m_networkManager.get(request);
    connect(qGuiApp, &QCoreApplication::aboutToQuit, jsonReply, &QNetworkReply::abort);
    connect(jsonReply, &QNetworkReply::finished, this, &Download::handleReleaseJsonDownloadFinished);
}

void Download::updateLatestNews()
{
    QUrl url("http://gpt4all.io/meta/latestnews.md");
    QNetworkRequest request(url);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);
    QNetworkReply *reply = m_networkManager.get(request);
    connect(qGuiApp, &QCoreApplication::aboutToQuit, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::finished, this, &Download::handleLatestNewsDownloadFinished);
}

void Download::downloadModel(const QString &modelFile)
{
    QFile *tempFile = new QFile(ModelList::globalInstance()->incompleteDownloadPath(modelFile));
    bool success = tempFile->open(QIODevice::WriteOnly | QIODevice::Append);
    qWarning() << "Opening temp file for writing:" << tempFile->fileName();
    if (!success) {
        const QString error
            = u"ERROR: Could not open temp file: %1 %2"_s.arg(tempFile->fileName(), modelFile);
        qWarning() << error;
        clearRetry(modelFile);
        ModelList::globalInstance()->updateDataByFilename(modelFile, {{ ModelList::DownloadErrorRole, error }});
        return;
    }
    tempFile->flush();
    size_t incomplete_size = tempFile->size();
    if (incomplete_size > 0) {
        bool success = tempFile->seek(incomplete_size);
        if (!success) {
            incomplete_size = 0;
            success = tempFile->seek(incomplete_size);
            Q_ASSERT(success);
        }
    }

    if (!ModelList::globalInstance()->containsByFilename(modelFile)) {
        qWarning() << "ERROR: Could not find file:" << modelFile;
        return;
    }

    ModelList::globalInstance()->updateDataByFilename(modelFile, {{ ModelList::DownloadingRole, true }});
    ModelInfo info = ModelList::globalInstance()->modelInfoByFilename(modelFile);
    QString url = !info.url().isEmpty() ? info.url() : "http://gpt4all.io/models/gguf/" + modelFile;
    Network::globalInstance()->trackEvent("download_started", { {"model", modelFile} });
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::User, modelFile);
    request.setRawHeader("range", u"bytes=%1-"_s.arg(tempFile->pos()).toUtf8());
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);
    QNetworkReply *modelReply = m_networkManager.get(request);
    connect(qGuiApp, &QCoreApplication::aboutToQuit, modelReply, &QNetworkReply::abort);
    connect(modelReply, &QNetworkReply::downloadProgress, this, &Download::handleDownloadProgress);
    connect(modelReply, &QNetworkReply::errorOccurred, this, &Download::handleErrorOccurred);
    connect(modelReply, &QNetworkReply::finished, this, &Download::handleModelDownloadFinished);
    connect(modelReply, &QNetworkReply::readyRead, this, &Download::handleReadyRead);
    m_activeDownloads.insert(modelReply, tempFile);
}

void Download::cancelDownload(const QString &modelFile)
{
    for (auto [modelReply, tempFile]: m_activeDownloads.asKeyValueRange()) {
        QUrl url = modelReply->request().url();
        if (url.toString().endsWith(modelFile)) {
            Network::globalInstance()->trackEvent("download_canceled", { {"model", modelFile} });

            // Disconnect the signals
            disconnect(modelReply, &QNetworkReply::downloadProgress, this, &Download::handleDownloadProgress);
            disconnect(modelReply, &QNetworkReply::finished, this, &Download::handleModelDownloadFinished);

            modelReply->abort(); // Abort the download
            modelReply->deleteLater(); // Schedule the reply for deletion

            tempFile->deleteLater();
            m_activeDownloads.remove(modelReply);

            ModelList::globalInstance()->updateDataByFilename(modelFile, {{ ModelList::DownloadingRole, false }});
            break;
        }
    }
}

void Download::installModel(const QString &modelFile, const QString &apiKey)
{
    Q_ASSERT(!apiKey.isEmpty());
    if (apiKey.isEmpty())
        return;

    Network::globalInstance()->trackEvent("install_model", { {"model", modelFile} });

    QString filePath = MySettings::globalInstance()->modelPath() + modelFile;
    QFile file(filePath);
    if (file.open(QIODeviceBase::WriteOnly | QIODeviceBase::Text)) {

        QJsonObject obj;
        QString modelName(modelFile);
        modelName.remove(0, 8); // strip "gpt4all-" prefix
        modelName.chop(7); // strip ".rmodel" extension
        obj.insert("apiKey", apiKey);
        obj.insert("modelName", modelName);
        QJsonDocument doc(obj);

        QTextStream stream(&file);
        stream << doc.toJson();
        file.close();
        ModelList::globalInstance()->updateModelsFromDirectory();
        emit toastMessage(tr("Model \"%1\" is installed successfully.").arg(modelName));
    }

    ModelList::globalInstance()->updateDataByFilename(modelFile, {{ ModelList::InstalledRole, true }});
}

void Download::installCompatibleModel(const QString &modelName, const QString &apiKey, const QString &baseUrl)
{
    Q_ASSERT(!modelName.isEmpty());
    if (modelName.isEmpty()) {
        emit toastMessage(tr("ERROR: $MODEL_NAME is empty."));
        return;
    }

    Q_ASSERT(!apiKey.isEmpty());
    if (apiKey.isEmpty()) {
        emit toastMessage(tr("ERROR: $API_KEY is empty."));
        return;
    }

    QUrl apiBaseUrl(QUrl::fromUserInput(baseUrl));
    if (!Network::isHttpUrlValid(baseUrl)) {
        emit toastMessage(tr("ERROR: $BASE_URL is invalid."));
        return;
    }

    QString modelFile(ModelList::compatibleModelFilename(baseUrl, modelName));
    if (ModelList::globalInstance()->contains(modelFile)) {
        emit toastMessage(tr("ERROR: Model \"%1 (%2)\" is conflict.").arg(modelName, baseUrl));
        return;
    }
    ModelList::globalInstance()->addModel(modelFile);
    Network::globalInstance()->trackEvent("install_model", { {"model", modelFile} });

    QString filePath = MySettings::globalInstance()->modelPath() + modelFile;
    QFile file(filePath);
    if (file.open(QIODeviceBase::WriteOnly | QIODeviceBase::Text)) {
        QJsonObject obj;
        obj.insert("apiKey", apiKey);
        obj.insert("modelName", modelName);
        obj.insert("baseUrl", apiBaseUrl.toString());
        QJsonDocument doc(obj);

        QTextStream stream(&file);
        stream << doc.toJson();
        file.close();
        ModelList::globalInstance()->updateModelsFromDirectory();
        emit toastMessage(tr("Model \"%1 (%2)\" is installed successfully.").arg(modelName, baseUrl));
    }

    ModelList::globalInstance()->updateDataByFilename(modelFile, {{ ModelList::InstalledRole, true }});
}

void Download::removeModel(const QString &modelFile)
{
    const QString filePath = MySettings::globalInstance()->modelPath() + modelFile;
    QFile incompleteFile(ModelList::globalInstance()->incompleteDownloadPath(modelFile));
    if (incompleteFile.exists()) {
        incompleteFile.remove();
    }

    bool shouldRemoveInstalled = false;
    QFile file(filePath);
    if (file.exists()) {
        const ModelInfo info = ModelList::globalInstance()->modelInfoByFilename(modelFile);
        MySettings::globalInstance()->eraseModel(info);
        shouldRemoveInstalled = info.installed && !info.isClone() && (info.isDiscovered() || info.isCompatibleApi || info.description() == "" /*indicates sideloaded*/);
        if (shouldRemoveInstalled)
            ModelList::globalInstance()->removeInstalled(info);
        Network::globalInstance()->trackEvent("remove_model", { {"model", modelFile} });
        file.remove();
        emit toastMessage(tr("Model \"%1\" is removed.").arg(info.name()));
    }

    if (!shouldRemoveInstalled) {
        QVector<QPair<int, QVariant>> data {
            { ModelList::InstalledRole, false },
            { ModelList::BytesReceivedRole, 0 },
            { ModelList::BytesTotalRole, 0 },
            { ModelList::TimestampRole, 0 },
            { ModelList::SpeedRole, QString() },
            { ModelList::DownloadErrorRole, QString() },
        };
        ModelList::globalInstance()->updateDataByFilename(modelFile, data);
    }
}

void Download::handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    QUrl url = reply->request().url();
    for (const auto &e : errors)
        qWarning() << "ERROR: Received ssl error:" << e.errorString() << "for" << url;
}

void Download::handleReleaseJsonDownloadFinished()
{
    QNetworkReply *jsonReply = qobject_cast<QNetworkReply *>(sender());
    if (!jsonReply)
        return;

    QByteArray jsonData = jsonReply->readAll();
    jsonReply->deleteLater();
    parseReleaseJsonFile(jsonData);
}

void Download::parseReleaseJsonFile(const QByteArray &jsonData)
{
    QJsonParseError err;
    QJsonDocument document = QJsonDocument::fromJson(jsonData, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "ERROR: Couldn't parse: " << jsonData << err.errorString();
        return;
    }

    QJsonArray jsonArray = document.array();

    m_releaseMap.clear();
    for (const QJsonValue &value : jsonArray) {
        QJsonObject obj = value.toObject();

        QString version = obj["version"].toString();
        // "notes" field intentionally has a trailing newline for compatibility
        QString notes = obj["notes"].toString().trimmed();
        QString contributors = obj["contributors"].toString().trimmed();
        ReleaseInfo releaseInfo;
        releaseInfo.version = version;
        releaseInfo.notes = notes;
        releaseInfo.contributors = contributors;
        m_releaseMap.insert(version, releaseInfo);
    }

    emit hasNewerReleaseChanged();
    emit releaseInfoChanged();
}

void Download::handleLatestNewsDownloadFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "ERROR: network error occurred attempting to download latest news:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    m_latestNews = QString::fromUtf8(responseData);
    reply->deleteLater();
    emit latestNewsChanged();
}

bool Download::hasRetry(const QString &filename) const
{
    return m_activeRetries.contains(filename);
}

bool Download::shouldRetry(const QString &filename)
{
    int retries = 0;
    if (m_activeRetries.contains(filename))
        retries = m_activeRetries.value(filename);

    ++retries;

    // Allow up to ten retries for now
    if (retries < 10) {
        m_activeRetries.insert(filename, retries);
        return true;
    }

    return false;
}

void Download::clearRetry(const QString &filename)
{
    m_activeRetries.remove(filename);
}

void Download::handleErrorOccurred(QNetworkReply::NetworkError code)
{
    QNetworkReply *modelReply = qobject_cast<QNetworkReply *>(sender());
    if (!modelReply)
        return;

    // This occurs when the user explicitly cancels the download
    if (code == QNetworkReply::OperationCanceledError)
        return;

    QString modelFilename = modelReply->request().attribute(QNetworkRequest::User).toString();
    if (shouldRetry(modelFilename)) {
        downloadModel(modelFilename);
        return;
    }

    clearRetry(modelFilename);

    const QString error
        = u"ERROR: Network error occurred attempting to download %1 code: %2 errorString %3"_s
            .arg(modelFilename)
            .arg(code)
            .arg(modelReply->errorString());
    qWarning() << error;
    ModelList::globalInstance()->updateDataByFilename(modelFilename, {{ ModelList::DownloadErrorRole, error }});
    Network::globalInstance()->trackEvent("download_error", {
        {"model", modelFilename},
        {"code", (int)code},
        {"error", modelReply->errorString()},
    });
    cancelDownload(modelFilename);
}

void Download::handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    QNetworkReply *modelReply = qobject_cast<QNetworkReply *>(sender());
    if (!modelReply)
        return;
    QFile *tempFile = m_activeDownloads.value(modelReply);
    if (!tempFile)
        return;
    QString contentRange = modelReply->rawHeader("content-range");
    if (contentRange.contains("/")) {
        QString contentTotalSize = contentRange.split("/").last();
        bytesTotal = contentTotalSize.toLongLong();
    }

    const QString modelFilename = modelReply->request().attribute(QNetworkRequest::User).toString();
    const qint64 lastUpdate = ModelList::globalInstance()->dataByFilename(modelFilename, ModelList::TimestampRole).toLongLong();
    const qint64 currentUpdate = QDateTime::currentMSecsSinceEpoch();
    if (currentUpdate - lastUpdate < 1000)
        return;

    const qint64 lastBytesReceived = ModelList::globalInstance()->dataByFilename(modelFilename, ModelList::BytesReceivedRole).toLongLong();
    const qint64 currentBytesReceived = tempFile->pos();

    qint64 timeDifference = currentUpdate - lastUpdate;
    qint64 bytesDifference = currentBytesReceived - lastBytesReceived;
    qint64 speed = (bytesDifference / timeDifference) * 1000; // bytes per second
    QString speedText;
    if (speed < 1024)
        speedText = QString::number(static_cast<double>(speed), 'f', 2) + " B/s";
    else if (speed < 1024 * 1024)
        speedText = QString::number(static_cast<double>(speed / 1024.0), 'f', 2) + " KB/s";
    else
        speedText = QString::number(static_cast<double>(speed / (1024.0 * 1024.0)), 'f', 2) + " MB/s";

    QVector<QPair<int, QVariant>> data {
        { ModelList::BytesReceivedRole, currentBytesReceived },
        { ModelList::BytesTotalRole, bytesTotal },
        { ModelList::SpeedRole, speedText },
        { ModelList::TimestampRole, currentUpdate },
    };
    ModelList::globalInstance()->updateDataByFilename(modelFilename, data);
}

HashAndSaveFile::HashAndSaveFile()
    : QObject(nullptr)
{
    moveToThread(&m_hashAndSaveThread);
    m_hashAndSaveThread.setObjectName("hashandsave thread");
    m_hashAndSaveThread.start();
}

void HashAndSaveFile::hashAndSave(const QString &expectedHash, QCryptographicHash::Algorithm a,
    const QString &saveFilePath, QFile *tempFile, QNetworkReply *modelReply)
{
    Q_ASSERT(!tempFile->isOpen());
    QString modelFilename = modelReply->request().attribute(QNetworkRequest::User).toString();

    // Reopen the tempFile for hashing
    if (!tempFile->open(QIODevice::ReadOnly)) {
        const QString error
            = u"ERROR: Could not open temp file for hashing: %1 %2"_s.arg(tempFile->fileName(), modelFilename);
        qWarning() << error;
        emit hashAndSaveFinished(false, error, tempFile, modelReply);
        return;
    }

    QCryptographicHash hash(a);
    while(!tempFile->atEnd())
        hash.addData(tempFile->read(16384));
    if (hash.result().toHex() != expectedHash.toLatin1()) {
        tempFile->close();
        const QString error
            = u"ERROR: Download error hash did not match: %1 != %2 for %3"_s
                .arg(hash.result().toHex(), expectedHash.toLatin1(), modelFilename);
        qWarning() << error;
        tempFile->remove();
        emit hashAndSaveFinished(false, error, tempFile, modelReply);
        return;
    }

    // The file save needs the tempFile closed
    tempFile->close();

    // Attempt to *move* the verified tempfile into place - this should be atomic
    // but will only work if the destination is on the same filesystem
    if (tempFile->rename(saveFilePath)) {
        emit hashAndSaveFinished(true, QString(), tempFile, modelReply);
        ModelList::globalInstance()->updateModelsFromDirectory();
        return;
    }

    // Reopen the tempFile for copying
    if (!tempFile->open(QIODevice::ReadOnly)) {
        const QString error
            = u"ERROR: Could not open temp file at finish: %1 %2"_s.arg(tempFile->fileName(), modelFilename);
        qWarning() << error;
        emit hashAndSaveFinished(false, error, tempFile, modelReply);
        return;
    }

    // Save the model file to disk
    QFile file(saveFilePath);
    if (file.open(QIODevice::WriteOnly)) {
        QByteArray buffer;
        while (!tempFile->atEnd()) {
            buffer = tempFile->read(16384);
            file.write(buffer);
        }
        file.close();
        tempFile->close();
        emit hashAndSaveFinished(true, QString(), tempFile, modelReply);
    } else {
        QFile::FileError error = file.error();
        const QString errorString
            = u"ERROR: Could not save model to location: %1 failed with code %1"_s.arg(saveFilePath).arg(error);
        qWarning() << errorString;
        tempFile->close();
        emit hashAndSaveFinished(false, errorString, tempFile, modelReply);
    }

    ModelList::globalInstance()->updateModelsFromDirectory();
}

void Download::handleModelDownloadFinished()
{
    QNetworkReply *modelReply = qobject_cast<QNetworkReply *>(sender());
    if (!modelReply)
        return;

    QString modelFilename = modelReply->request().attribute(QNetworkRequest::User).toString();
    QFile *tempFile = m_activeDownloads.value(modelReply);
    m_activeDownloads.remove(modelReply);

    if (modelReply->error()) {
        const QString errorString
            = u"ERROR: Downloading failed with code %1 \"%2\""_s.arg(modelReply->error()).arg(modelReply->errorString());
        qWarning() << errorString;
        modelReply->deleteLater();
        tempFile->deleteLater();
        if (!hasRetry(modelFilename)) {
            QVector<QPair<int, QVariant>> data {
                { ModelList::DownloadingRole, false },
                { ModelList::DownloadErrorRole, errorString },
            };
            ModelList::globalInstance()->updateDataByFilename(modelFilename, data);
        }
        return;
    }

    clearRetry(modelFilename);

    // The hash and save needs the tempFile closed
    tempFile->close();

    if (!ModelList::globalInstance()->containsByFilename(modelFilename)) {
        qWarning() << "ERROR: downloading no such file:" << modelFilename;
        modelReply->deleteLater();
        tempFile->deleteLater();
        return;
    }

    // Notify that we are calculating hash
    ModelList::globalInstance()->updateDataByFilename(modelFilename, {{ ModelList::CalcHashRole, true }});
    QByteArray hash =  ModelList::globalInstance()->modelInfoByFilename(modelFilename).hash;
    ModelInfo::HashAlgorithm hashAlgorithm =  ModelList::globalInstance()->modelInfoByFilename(modelFilename).hashAlgorithm;
    const QString saveFilePath = MySettings::globalInstance()->modelPath() + modelFilename;
    emit requestHashAndSave(hash,
        (hashAlgorithm == ModelInfo::Md5 ? QCryptographicHash::Md5 : QCryptographicHash::Sha256),
        saveFilePath, tempFile, modelReply);
}

void Download::handleHashAndSaveFinished(bool success, const QString &error,
        QFile *tempFile, QNetworkReply *modelReply)
{
    // The hash and save should send back with tempfile closed
    Q_ASSERT(!tempFile->isOpen());
    QString modelFilename = modelReply->request().attribute(QNetworkRequest::User).toString();
    Network::globalInstance()->trackEvent("download_finished", { {"model", modelFilename}, {"success", success} });

    QVector<QPair<int, QVariant>> data {
        { ModelList::CalcHashRole, false },
        { ModelList::DownloadingRole, false },
    };

    modelReply->deleteLater();
    tempFile->deleteLater();

    if (!success) {
        data.append({ ModelList::DownloadErrorRole, error });
    } else {
        data.append({ ModelList::DownloadErrorRole, QString() });
        ModelInfo info = ModelList::globalInstance()->modelInfoByFilename(modelFilename);
        if (info.isDiscovered())
            ModelList::globalInstance()->updateDiscoveredInstalled(info);
    }

    ModelList::globalInstance()->updateDataByFilename(modelFilename, data);
}

void Download::handleReadyRead()
{
    QNetworkReply *modelReply = qobject_cast<QNetworkReply *>(sender());
    if (!modelReply)
        return;

    QFile *tempFile = m_activeDownloads.value(modelReply);
    QByteArray buffer;
    while (!modelReply->atEnd()) {
        buffer = modelReply->read(16384);
        tempFile->write(buffer);
    }
    tempFile->flush();
}
