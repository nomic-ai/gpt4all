#include "download.h"

#include <QCoreApplication>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QDir>

class MyDownload: public Download { };
Q_GLOBAL_STATIC(MyDownload, downloadInstance)
Download *Download::globalInstance()
{
    return downloadInstance();
}

Download::Download()
    : QObject(nullptr)
{
    updateModelList();
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

void Download::updateModelList()
{
    QUrl jsonUrl("http://gpt4all.io/models/models.json");
    QNetworkRequest request(jsonUrl);
    QNetworkReply *jsonReply = m_networkManager.get(request);
    connect(jsonReply, &QNetworkReply::finished, this, &Download::handleJsonDownloadFinished);
}

void Download::downloadModel(const QString &modelFile)
{
    QNetworkRequest request("http://gpt4all.io/models/" + modelFile);
    QNetworkReply *modelReply = m_networkManager.get(request);
    connect(modelReply, &QNetworkReply::downloadProgress, this, &Download::handleDownloadProgress);
    connect(modelReply, &QNetworkReply::finished, this, &Download::handleModelDownloadFinished);
    m_activeDownloads.append(modelReply);
}

void Download::cancelDownload(const QString &modelFile)
{
    for (int i = 0; i < m_activeDownloads.size(); ++i) {
        QNetworkReply *modelReply = m_activeDownloads.at(i);
        QUrl url = modelReply->request().url();
        if (url.toString().endsWith(modelFile)) {
            // Disconnect the signals
            disconnect(modelReply, &QNetworkReply::downloadProgress, this, &Download::handleDownloadProgress);
            disconnect(modelReply, &QNetworkReply::finished, this, &Download::handleModelDownloadFinished);

            modelReply->abort(); // Abort the download
            modelReply->deleteLater(); // Schedule the reply for deletion
            m_activeDownloads.removeAll(modelReply);

            // Emit downloadFinished signal for cleanup
            emit downloadFinished(modelFile);
            break;
        }
    }
}

void Download::handleJsonDownloadFinished()
{
#if 0
    QByteArray jsonData = QString(""
    "["
    "  {"
    "    \"md5sum\": \"61d48a82cb188cceb14ebb8082bfec37\","
    "    \"filename\": \"ggml-gpt4all-j-v1.1-breezy.bin\""
    "  },"
    "  {"
    "    \"md5sum\": \"879344aaa9d62fdccbda0be7a09e7976\","
    "    \"filename\": \"ggml-gpt4all-j-v1.2-jazzy.bin\","
    "    \"isDefault\": \"true\""
    "  },"
    "  {"
    "    \"md5sum\": \"5b5a3f9b858d33b29b52b89692415595\","
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
        QByteArray modelMd5sum = obj["md5sum"].toString().toLatin1().constData();
        bool isDefault = obj.contains("isDefault") && obj["isDefault"] == QString("true");

        QString filePath = QCoreApplication::applicationDirPath() + QDir::separator() + modelFilename;
        QFileInfo info(filePath);
        ModelInfo modelInfo;
        modelInfo.filename = modelFilename;
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
//    qDebug() << "handleDownloadProgress" << bytesReceived << bytesTotal << modelFilename;
    emit downloadProgress(bytesReceived, bytesTotal, modelFilename);
}

bool operator==(const ModelInfo& lhs, const ModelInfo& rhs) {
    return lhs.filename == rhs.filename && lhs.md5sum == rhs.md5sum;
}

void Download::handleModelDownloadFinished()
{
    QNetworkReply *modelReply = qobject_cast<QNetworkReply *>(sender());
    if (!modelReply)
        return;

    QString modelFilename = modelReply->url().fileName();
//    qDebug() << "handleModelDownloadFinished" << modelFilename;
    m_activeDownloads.removeAll(modelReply);

    if (modelReply->error()) {
        qWarning() << "ERROR: downloading:" << modelReply->errorString();
        modelReply->deleteLater();
        emit downloadFinished(modelFilename);
        return;
    }

    QByteArray modelData = modelReply->readAll();
    if (!m_modelMap.contains(modelFilename)) {
        qWarning() << "ERROR: Cannot find in download map:" << modelFilename;
        modelReply->deleteLater();
        emit downloadFinished(modelFilename);
        return;
    }

    ModelInfo info = m_modelMap.value(modelFilename);
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(modelData);
    if (hash.result().toHex() != info.md5sum) {
        qWarning() << "ERROR: Download error MD5SUM did not match:"
            << hash.result().toHex()
            << "!=" << info.md5sum << "for" << modelFilename;
        modelReply->deleteLater();
        emit downloadFinished(modelFilename);
        return;
    }

    // Save the model file to disk
    QFile file(QCoreApplication::applicationDirPath() + QDir::separator() + modelFilename);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(modelData);
        file.close();
    }

    modelReply->deleteLater();
    emit downloadFinished(modelFilename);

    info.installed = true;
    m_modelMap.insert(modelFilename, info);
    emit modelListChanged();
}
