#include "download.h"
#include "network.h"

#include <QCoreApplication>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QDir>
#include <QStandardPaths>
#include <QSettings>

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
    connect(this, &Download::downloadLocalModelsPathChanged, this, &Download::updateModelList);
    updateModelList();
    updateReleaseNotes();
    QSettings settings;
    settings.sync();
    m_downloadLocalModelsPath = settings.value("modelPath",
        defaultLocalModelsPath()).toString();
    m_startTime = QDateTime::currentDateTime();
}

bool operator==(const ModelInfo& lhs, const ModelInfo& rhs) {
    return lhs.filename == rhs.filename && lhs.md5sum == rhs.md5sum;
}

bool operator==(const ReleaseInfo& lhs, const ReleaseInfo& rhs) {
    return lhs.version == rhs.version;
}

bool compareVersions(const QString &a, const QString &b) {
    QStringList aParts = a.split('.');
    QStringList bParts = b.split('.');

    for (int i = 0; i < std::min(aParts.size(), bParts.size()); ++i) {
        int aInt = aParts[i].toInt();
        int bInt = bParts[i].toInt();

        if (aInt > bInt) {
            return true;
        } else if (aInt < bInt) {
            return false;
        }
    }

    return aParts.size() > bParts.size();
}

QList<ModelInfo> Download::modelList() const
{
    // We make sure the default model is listed first
    QList<ModelInfo> values = m_modelMap.values();
    ModelInfo defaultInfo;
    ModelInfo bestGPTJInfo;
    ModelInfo bestLlamaInfo;
    ModelInfo bestMPTInfo;
    QList<ModelInfo> filtered;
    for (ModelInfo v : values) {
        if (v.isDefault)
            defaultInfo = v;
        if (v.bestGPTJ)
            bestGPTJInfo = v;
        if (v.bestLlama)
            bestLlamaInfo = v;
        if (v.bestMPT)
            bestMPTInfo = v;
        filtered.append(v);
    }

    Q_ASSERT(defaultInfo == bestGPTJInfo || defaultInfo == bestLlamaInfo || defaultInfo == bestMPTInfo);

    if (bestLlamaInfo.bestLlama) {
        filtered.removeAll(bestLlamaInfo);
        filtered.prepend(bestLlamaInfo);
    }

    if (bestGPTJInfo.bestGPTJ) {
        filtered.removeAll(bestGPTJInfo);
        filtered.prepend(bestGPTJInfo);
    }

    if (bestMPTInfo.bestMPT) {
        filtered.removeAll(bestMPTInfo);
        filtered.prepend(bestMPTInfo);
    }

    return filtered;
}

ReleaseInfo Download::releaseInfo() const
{
    const QString currentVersion = QCoreApplication::applicationVersion();
    if (m_releaseMap.contains(currentVersion))
        return m_releaseMap.value(currentVersion);
    return ReleaseInfo();
}

bool Download::hasNewerRelease() const
{
    const QString currentVersion = QCoreApplication::applicationVersion();
    QList<QString> versions = m_releaseMap.keys();
    std::sort(versions.begin(), versions.end(), compareVersions);
    if (versions.isEmpty())
        return false;
    return compareVersions(versions.first(), currentVersion);
}

QString Download::downloadLocalModelsPath() const {
    return m_downloadLocalModelsPath;
}

void Download::setDownloadLocalModelsPath(const QString &modelPath) {
    QString filePath = (modelPath.startsWith("file://") ?
                        QUrl(modelPath).toLocalFile() : modelPath);
    QString canonical = QFileInfo(filePath).canonicalFilePath() + "/";
    if (m_downloadLocalModelsPath != canonical) {
        m_downloadLocalModelsPath = canonical;
        emit downloadLocalModelsPathChanged();
    }
}

bool Download::isFirstStart() const
{
    QSettings settings;
    settings.sync();
    QString lastVersionStarted = settings.value("download/lastVersionStarted").toString();
    bool first = lastVersionStarted != QCoreApplication::applicationVersion();
    settings.setValue("download/lastVersionStarted", QCoreApplication::applicationVersion());
    settings.sync();
    return first;
}

QString Download::incompleteDownloadPath(const QString &modelFile) {
    QString downloadPath = downloadLocalModelsPath() + "incomplete-" +
                           modelFile;
    return downloadPath;
}

QString Download::defaultLocalModelsPath() const
{
    QString localPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        + "/";
    QString testWritePath = localPath + QString("test_write.txt");
    QString canonicalLocalPath = QFileInfo(localPath).canonicalFilePath() + "/";
    QDir localDir(localPath);
    if (!localDir.exists()) {
        if (!localDir.mkpath(localPath)) {
            qWarning() << "ERROR: Local download directory can't be created:" << canonicalLocalPath;
            return canonicalLocalPath;
        }
    }

    if (QFileInfo::exists(testWritePath))
        return canonicalLocalPath;

    QFile testWriteFile(testWritePath);
    if (testWriteFile.open(QIODeviceBase::ReadWrite)) {
        testWriteFile.close();
        return canonicalLocalPath;
    }

    qWarning() << "ERROR: Local download path appears not writeable:" << canonicalLocalPath;
    return canonicalLocalPath;
}

void Download::updateModelList()
{
    QUrl jsonUrl("http://gpt4all.io/models/models.json");
    QNetworkRequest request(jsonUrl);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);
    QNetworkReply *jsonReply = m_networkManager.get(request);
    connect(jsonReply, &QNetworkReply::finished, this, &Download::handleModelsJsonDownloadFinished);
}

void Download::updateReleaseNotes()
{
    QUrl jsonUrl("http://gpt4all.io/meta/release.json");
    QNetworkRequest request(jsonUrl);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);
    QNetworkReply *jsonReply = m_networkManager.get(request);
    connect(jsonReply, &QNetworkReply::finished, this, &Download::handleReleaseJsonDownloadFinished);
}

void Download::downloadModel(const QString &modelFile)
{
    QFile *tempFile = new QFile(incompleteDownloadPath(modelFile));
    QDateTime modTime = tempFile->fileTime(QFile::FileModificationTime);
    bool success = tempFile->open(QIODevice::WriteOnly | QIODevice::Append);
    qWarning() << "Opening temp file for writing:" << tempFile->fileName();
    if (!success) {
        qWarning() << "ERROR: Could not open temp file:"
            << tempFile->fileName() << modelFile;
        return;
    }
    size_t incomplete_size = tempFile->size();
    if (incomplete_size > 0) {
        if (modTime < m_startTime) {
            qWarning() << "File last modified before app started, rewinding by 1MB";
            if (incomplete_size >= 1024 * 1024) {
                incomplete_size -= 1024 * 1024;
            } else {
                incomplete_size = 0;
            }
        }
        tempFile->seek(incomplete_size);
    }

    Network::globalInstance()->sendDownloadStarted(modelFile);
    QNetworkRequest request("http://gpt4all.io/models/" + modelFile);
    request.setRawHeader("range", QString("bytes=%1-").arg(incomplete_size).toUtf8());
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);
    QNetworkReply *modelReply = m_networkManager.get(request);
    connect(modelReply, &QNetworkReply::downloadProgress, this, &Download::handleDownloadProgress);
    connect(modelReply, &QNetworkReply::finished, this, &Download::handleModelDownloadFinished);
    connect(modelReply, &QNetworkReply::readyRead, this, &Download::handleReadyRead);
    m_activeDownloads.insert(modelReply, tempFile);
}

void Download::cancelDownload(const QString &modelFile)
{
    for (int i = 0; i < m_activeDownloads.size(); ++i) {
        QNetworkReply *modelReply = m_activeDownloads.keys().at(i);
        QUrl url = modelReply->request().url();
        if (url.toString().endsWith(modelFile)) {
            Network::globalInstance()->sendDownloadCanceled(modelFile);

            // Disconnect the signals
            disconnect(modelReply, &QNetworkReply::downloadProgress, this, &Download::handleDownloadProgress);
            disconnect(modelReply, &QNetworkReply::finished, this, &Download::handleModelDownloadFinished);

            modelReply->abort(); // Abort the download
            modelReply->deleteLater(); // Schedule the reply for deletion

            QFile *tempFile = m_activeDownloads.value(modelReply);
            tempFile->deleteLater();
            m_activeDownloads.remove(modelReply);

            // Emit downloadFinished signal for cleanup
            emit downloadFinished(modelFile);
            break;
        }
    }
}

void Download::installModel(const QString &modelFile, const QString &apiKey)
{
    Q_ASSERT(!apiKey.isEmpty());
    if (apiKey.isEmpty())
        return;

    Network::globalInstance()->sendInstallModel(modelFile);
    QString filePath = downloadLocalModelsPath() + modelFile + ".txt";
    QFile file(filePath);
    if (file.open(QIODeviceBase::WriteOnly | QIODeviceBase::Text)) {
        QTextStream stream(&file);
        stream << apiKey;
        file.close();
        ModelInfo info = m_modelMap.value(modelFile);
        info.installed = true;
        m_modelMap.insert(modelFile, info);
        emit modelListChanged();
    }
}

void Download::removeModel(const QString &modelFile)
{
    const bool isChatGPT = modelFile.startsWith("chatgpt-");
    const QString filePath = downloadLocalModelsPath()
        + (!isChatGPT ? "ggml-" : QString())
        + modelFile
        + (!isChatGPT ? ".bin" : ".txt");
    QFile file(filePath);
    if (!file.exists()) {
        qWarning() << "ERROR: Cannot remove file that does not exist" << filePath;
        return;
    }

    Network::globalInstance()->sendRemoveModel(modelFile);
    ModelInfo info = m_modelMap.value(modelFile);
    info.installed = false;
    m_modelMap.insert(modelFile, info);
    file.remove();
    emit modelListChanged();
}

void Download::handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    QUrl url = reply->request().url();
    for (auto e : errors)
        qWarning() << "ERROR: Received ssl error:" << e.errorString() << "for" << url;
}

void Download::handleModelsJsonDownloadFinished()
{
#if 0
    QByteArray jsonData = QString(""
    "["
    "  {"
    "    \"md5sum\": \"61d48a82cb188cceb14ebb8082bfec37\","
    "    \"filename\": \"ggml-gpt4all-j-v1.1-breezy.bin\","
    "    \"filesize\": \"3785248281\""
    "  },"
    "  {"
    "    \"md5sum\": \"879344aaa9d62fdccbda0be7a09e7976\","
    "    \"filename\": \"ggml-gpt4all-j-v1.2-jazzy.bin\","
    "    \"filesize\": \"3785248281\","
    "    \"isDefault\": \"true\""
    "  },"
    "  {"
    "    \"md5sum\": \"5b5a3f9b858d33b29b52b89692415595\","
    "    \"filesize\": \"3785248281\","
    "    \"filename\": \"ggml-gpt4all-j.bin\""
    "  }"
    "]"
    ).toUtf8();
    printf("%s\n", jsonData.toStdString().c_str());
    fflush(stdout);
#else
    QNetworkReply *jsonReply = qobject_cast<QNetworkReply *>(sender());
    if (!jsonReply)
        return;

    QByteArray jsonData = jsonReply->readAll();
    jsonReply->deleteLater();
#endif
    parseModelsJsonFile(jsonData);
}

void Download::parseModelsJsonFile(const QByteArray &jsonData)
{
    QJsonParseError err;
    QJsonDocument document = QJsonDocument::fromJson(jsonData, &err);
    if (err.error != QJsonParseError::NoError) {
        qDebug() << "ERROR: Couldn't parse: " << jsonData << err.errorString();
        return;
    }

    QString defaultModel;
    QJsonArray jsonArray = document.array();
    const QString currentVersion = QCoreApplication::applicationVersion();

    m_modelMap.clear();
    for (const QJsonValue &value : jsonArray) {
        QJsonObject obj = value.toObject();

        QString modelFilename = obj["filename"].toString();
        QString modelFilesize = obj["filesize"].toString();
        QString requires = obj["requires"].toString();
        QByteArray modelMd5sum = obj["md5sum"].toString().toLatin1().constData();
        bool isDefault = obj.contains("isDefault") && obj["isDefault"] == QString("true");
        bool bestGPTJ = obj.contains("bestGPTJ") && obj["bestGPTJ"] == QString("true");
        bool bestLlama = obj.contains("bestLlama") && obj["bestLlama"] == QString("true");
        bool bestMPT = obj.contains("bestMPT") && obj["bestMPT"] == QString("true");
        QString description = obj["description"].toString();

        if (!requires.isEmpty()
            && requires != currentVersion
            && compareVersions(requires, currentVersion)) {
            continue;
        }

        if (isDefault)
            defaultModel = modelFilename;
        quint64 sz = modelFilesize.toULongLong();
        if (sz < 1024) {
            modelFilesize = QString("%1 bytes").arg(sz);
        } else if (sz < 1024 * 1024) {
            modelFilesize = QString("%1 KB").arg(qreal(sz) / 1024, 0, 'g', 3);
        } else if (sz < 1024 * 1024 * 1024) {
            modelFilesize = QString("%1 MB").arg(qreal(sz) / (1024 * 1024), 0, 'g', 3);
        } else {
            modelFilesize = QString("%1 GB").arg(qreal(sz) / (1024 * 1024 * 1024), 0, 'g', 3);
        }

        QString filePath = downloadLocalModelsPath() + modelFilename;
        QFileInfo info(filePath);
        ModelInfo modelInfo;
        modelInfo.filename = modelFilename;
        modelInfo.filesize = modelFilesize;
        modelInfo.md5sum = modelMd5sum;
        modelInfo.installed = info.exists();
        modelInfo.isDefault = isDefault;
        modelInfo.bestGPTJ = bestGPTJ;
        modelInfo.bestLlama = bestLlama;
        modelInfo.bestMPT = bestMPT;
        modelInfo.description = description;
        modelInfo.requires = requires;
        m_modelMap.insert(modelInfo.filename, modelInfo);
    }

    const QString chatGPTDesc = tr("WARNING: requires personal OpenAI API key and usage of this "
        "model will send your chats over the network to OpenAI. Your API key will be stored on disk "
        "and only used to interact with OpenAI models. If you don't have one, you can apply for "
        "an API key <a href=\"https://platform.openai.com/account/api-keys\">here.</a>");

    {
        ModelInfo modelInfo;
        modelInfo.isChatGPT = true;
        modelInfo.filename = "chatgpt-gpt-3.5-turbo";
        modelInfo.description = tr("OpenAI's ChatGPT model gpt-3.5-turbo. ") + chatGPTDesc;
        modelInfo.requires = "2.4.2";
        QString filePath = downloadLocalModelsPath() + modelInfo.filename + ".txt";
        QFileInfo info(filePath);
        modelInfo.installed = info.exists();
        m_modelMap.insert(modelInfo.filename, modelInfo);
    }

    {
        ModelInfo modelInfo;
        modelInfo.isChatGPT = true;
        modelInfo.filename = "chatgpt-gpt-4";
        modelInfo.description = tr("OpenAI's ChatGPT model gpt-4. ") + chatGPTDesc;
        modelInfo.requires = "2.4.2";
        QString filePath = downloadLocalModelsPath() + modelInfo.filename + ".txt";
        QFileInfo info(filePath);
        modelInfo.installed = info.exists();
        m_modelMap.insert(modelInfo.filename, modelInfo);
    }

    // remove ggml- prefix and .bin suffix
    Q_ASSERT(defaultModel.startsWith("ggml-"));
    defaultModel = defaultModel.remove(0, 5);
    Q_ASSERT(defaultModel.endsWith(".bin"));
    defaultModel.chop(4);

    QSettings settings;
    settings.sync();
    settings.setValue("defaultModel", defaultModel);
    settings.sync();
    emit modelListChanged();
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
        qDebug() << "ERROR: Couldn't parse: " << jsonData << err.errorString();
        return;
    }

    QJsonArray jsonArray = document.array();

    m_releaseMap.clear();
    for (const QJsonValue &value : jsonArray) {
        QJsonObject obj = value.toObject();

        QString version = obj["version"].toString();
        QString notes = obj["notes"].toString();
        QString contributors = obj["contributors"].toString();
        ReleaseInfo releaseInfo;
        releaseInfo.version = version;
        releaseInfo.notes = notes;
        releaseInfo.contributors = contributors;
        m_releaseMap.insert(version, releaseInfo);
    }

    emit hasNewerReleaseChanged();
    emit releaseInfoChanged();
}

void Download::handleErrorOccurred(QNetworkReply::NetworkError code)
{
    QNetworkReply *modelReply = qobject_cast<QNetworkReply *>(sender());
    if (!modelReply)
        return;

    QString modelFilename = modelReply->url().fileName();
    qWarning() << "ERROR: Network error occurred attempting to download"
               << modelFilename
               << "code:" << code
               << "errorString" << modelReply->errorString();
    Network::globalInstance()->sendDownloadError(modelFilename, (int)code, modelReply->errorString());
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

    QString modelFilename = modelReply->url().fileName();
    emit downloadProgress(tempFile->pos(), bytesTotal, modelFilename);
}

HashAndSaveFile::HashAndSaveFile()
    : QObject(nullptr)
{
    moveToThread(&m_hashAndSaveThread);
    m_hashAndSaveThread.setObjectName("hashandsave thread");
    m_hashAndSaveThread.start();
}

void HashAndSaveFile::hashAndSave(const QString &expectedHash, const QString &saveFilePath,
        QFile *tempFile, QNetworkReply *modelReply)
{
    Q_ASSERT(!tempFile->isOpen());
    QString modelFilename = modelReply->url().fileName();

    // Reopen the tempFile for hashing
    if (!tempFile->open(QIODevice::ReadOnly)) {
        qWarning() << "ERROR: Could not open temp file for hashing:"
            << tempFile->fileName() << modelFilename;
        emit hashAndSaveFinished(false, tempFile, modelReply);
        return;
    }

    QCryptographicHash hash(QCryptographicHash::Md5);
    while(!tempFile->atEnd())
        hash.addData(tempFile->read(16384));
    if (hash.result().toHex() != expectedHash) {
        tempFile->close();
        qWarning() << "ERROR: Download error MD5SUM did not match:"
            << hash.result().toHex()
            << "!=" << expectedHash << "for" << modelFilename;
        tempFile->remove();
        emit hashAndSaveFinished(false, tempFile, modelReply);
        return;
    }

    // The file save needs the tempFile closed
    tempFile->close();

    // Attempt to *move* the verified tempfile into place - this should be atomic
    // but will only work if the destination is on the same filesystem
    if (tempFile->rename(saveFilePath)) {
        emit hashAndSaveFinished(true, tempFile, modelReply);
        return;
    }

    // Reopen the tempFile for copying
    if (!tempFile->open(QIODevice::ReadOnly)) {
        qWarning() << "ERROR: Could not open temp file at finish:"
            << tempFile->fileName() << modelFilename;
        emit hashAndSaveFinished(false, tempFile, modelReply);
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
        emit hashAndSaveFinished(true, tempFile, modelReply);
    } else {
        QFile::FileError error = file.error();
        qWarning() << "ERROR: Could not save model to location:"
            << saveFilePath
            << "failed with code" << error;
        tempFile->close();
        emit hashAndSaveFinished(false, tempFile, modelReply);
        return;
    }
}

void Download::handleModelDownloadFinished()
{
    QNetworkReply *modelReply = qobject_cast<QNetworkReply *>(sender());
    if (!modelReply)
        return;

    QString modelFilename = modelReply->url().fileName();
    QFile *tempFile = m_activeDownloads.value(modelReply);
    m_activeDownloads.remove(modelReply);

    if (modelReply->error()) {
        qWarning() << "ERROR: downloading:" << modelReply->errorString();
        modelReply->deleteLater();
        tempFile->deleteLater();
        emit downloadFinished(modelFilename);
        return;
    }

    // The hash and save needs the tempFile closed
    tempFile->close();

    // Notify that we are calculating hash
    ModelInfo info = m_modelMap.value(modelFilename);
    info.calcHash = true;
    m_modelMap.insert(modelFilename, info);
    emit modelListChanged();

    const QString saveFilePath = downloadLocalModelsPath() + modelFilename;
    emit requestHashAndSave(info.md5sum, saveFilePath, tempFile, modelReply);
}

void Download::handleHashAndSaveFinished(bool success,
        QFile *tempFile, QNetworkReply *modelReply)
{
    // The hash and save should send back with tempfile closed
    Q_ASSERT(!tempFile->isOpen());
    QString modelFilename = modelReply->url().fileName();
    Network::globalInstance()->sendDownloadFinished(modelFilename, success);

    ModelInfo info = m_modelMap.value(modelFilename);
    info.calcHash = false;
    info.installed = success;
    m_modelMap.insert(modelFilename, info);
    emit modelListChanged();

    modelReply->deleteLater();
    tempFile->deleteLater();
    emit downloadFinished(modelFilename);
}

void Download::handleReadyRead()
{
    QNetworkReply *modelReply = qobject_cast<QNetworkReply *>(sender());
    if (!modelReply)
        return;

    QString modelFilename = modelReply->url().fileName();
    QFile *tempFile = m_activeDownloads.value(modelReply);
    QByteArray buffer;
    while (!modelReply->atEnd()) {
        buffer = modelReply->read(16384);
        tempFile->write(buffer);
    }
    tempFile->flush();
}
