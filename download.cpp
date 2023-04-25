#include "download.h"

#include <QCoreApplication>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QDir>
#include <QStandardPaths>

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
    m_downloadLocalModelsPath = defaultLocalModelsPath();
}

QList<ModelInfo> Download::modelList() const
{
    // We make sure the default model is listed first
    QList<ModelInfo> values = m_modelMap.values();
    ModelInfo defaultInfo;
    for (ModelInfo v : values) {
        if (v.isDefault) {
            defaultInfo = v;
            break;
        }
    }
    values.removeAll(defaultInfo);
    values.prepend(defaultInfo);
    return values;
}

QString Download::downloadLocalModelsPath() const {
    return m_downloadLocalModelsPath;
}

void Download::setDownloadLocalModelsPath(const QString &modelPath) {
    QString filePath = (modelPath.startsWith("file://") ?
                        QUrl(modelPath).toLocalFile() : modelPath);
    QString canonical = QFileInfo(filePath).canonicalFilePath() + QDir::separator();
    qDebug() << "Set model path: " << canonical;
    if (m_downloadLocalModelsPath != canonical) {
        m_downloadLocalModelsPath = canonical;
        emit downloadLocalModelsPathChanged();
    }
}

QString Download::defaultLocalModelsPath() const
{
    QString localPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        + QDir::separator();
    QString testWritePath = localPath + QString("test_write.txt");
    QString canonicalLocalPath = QFileInfo(localPath).canonicalFilePath() + QDir::separator();
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
    connect(jsonReply, &QNetworkReply::finished, this, &Download::handleJsonDownloadFinished);
}

void Download::downloadModel(const QString &modelFile)
{
    QTemporaryFile *tempFile = new QTemporaryFile;
    bool success = tempFile->open();
    qWarning() << "Opening temp file for writing:" << tempFile->fileName();
    if (!success) {
        qWarning() << "ERROR: Could not open temp file:"
            << tempFile->fileName() << modelFile;
        return;
    }

    QNetworkRequest request("http://gpt4all.io/models/" + modelFile);
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
            // Disconnect the signals
            disconnect(modelReply, &QNetworkReply::downloadProgress, this, &Download::handleDownloadProgress);
            disconnect(modelReply, &QNetworkReply::finished, this, &Download::handleModelDownloadFinished);

            modelReply->abort(); // Abort the download
            modelReply->deleteLater(); // Schedule the reply for deletion

            QTemporaryFile *tempFile = m_activeDownloads.value(modelReply);
            tempFile->deleteLater();
            m_activeDownloads.remove(modelReply);

            // Emit downloadFinished signal for cleanup
            emit downloadFinished(modelFile);
            break;
        }
    }
}

void Download::handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    QUrl url = reply->request().url();
    for (auto e : errors)
        qWarning() << "ERROR: Received ssl error:" << e.errorString() << "for" << url;
}

void Download::handleJsonDownloadFinished()
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
    parseJsonFile(jsonData);
}

void Download::parseJsonFile(const QByteArray &jsonData)
{
    QJsonParseError err;
    QJsonDocument document = QJsonDocument::fromJson(jsonData, &err);
    if (err.error != QJsonParseError::NoError) {
        qDebug() << "ERROR: Couldn't parse: " << jsonData << err.errorString();
        return;
    }

    QJsonArray jsonArray = document.array();

    m_modelMap.clear();
    for (const QJsonValue &value : jsonArray) {
        QJsonObject obj = value.toObject();

        QString modelFilename = obj["filename"].toString();
        QString modelFilesize = obj["filesize"].toString();
        QByteArray modelMd5sum = obj["md5sum"].toString().toLatin1().constData();
        bool isDefault = obj.contains("isDefault") && obj["isDefault"] == QString("true");

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
        m_modelMap.insert(modelInfo.filename, modelInfo);
    }

    emit modelListChanged();
}

void Download::handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    QNetworkReply *modelReply = qobject_cast<QNetworkReply *>(sender());
    if (!modelReply)
        return;

    QString modelFilename = modelReply->url().fileName();
    emit downloadProgress(bytesReceived, bytesTotal, modelFilename);
}

bool operator==(const ModelInfo& lhs, const ModelInfo& rhs) {
    return lhs.filename == rhs.filename && lhs.md5sum == rhs.md5sum;
}

HashAndSaveFile::HashAndSaveFile()
    : QObject(nullptr)
{
    moveToThread(&m_hashAndSaveThread);
    m_hashAndSaveThread.setObjectName("hashandsave thread");
    m_hashAndSaveThread.start();
}

void HashAndSaveFile::hashAndSave(const QString &expectedHash, const QString &saveFilePath,
        QTemporaryFile *tempFile, QNetworkReply *modelReply)
{
    Q_ASSERT(!tempFile->isOpen());
    QString modelFilename = modelReply->url().fileName();

    // Reopen the tempFile for hashing
    if (!tempFile->open()) {
        qWarning() << "ERROR: Could not open temp file for hashing:"
            << tempFile->fileName() << modelFilename;
        emit hashAndSaveFinished(false, tempFile, modelReply);
        return;
    }

    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(tempFile->readAll());
    while(!tempFile->atEnd())
        hash.addData(tempFile->read(16384));
    if (hash.result().toHex() != expectedHash) {
        tempFile->close();
        qWarning() << "ERROR: Download error MD5SUM did not match:"
            << hash.result().toHex()
            << "!=" << expectedHash << "for" << modelFilename;
        emit hashAndSaveFinished(false, tempFile, modelReply);
        return;
    }

    // The file save needs the tempFile closed
    tempFile->close();

    // Reopen the tempFile for copying
    if (!tempFile->open()) {
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
    QTemporaryFile *tempFile = m_activeDownloads.value(modelReply);
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
        QTemporaryFile *tempFile, QNetworkReply *modelReply)
{
    // The hash and save should send back with tempfile closed
    Q_ASSERT(!tempFile->isOpen());
    QString modelFilename = modelReply->url().fileName();

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
    QTemporaryFile *tempFile = m_activeDownloads.value(modelReply);
    QByteArray buffer;
    while (!modelReply->atEnd()) {
        buffer = modelReply->read(16384);
        tempFile->write(buffer);
    }
}
