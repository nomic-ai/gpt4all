#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QVariant>
#include <QTemporaryFile>
#include <QThread>

struct ModelInfo {
    Q_GADGET
    Q_PROPERTY(QString filename MEMBER filename)
    Q_PROPERTY(QString filesize MEMBER filesize)
    Q_PROPERTY(QByteArray md5sum MEMBER md5sum)
    Q_PROPERTY(bool calcHash MEMBER calcHash)
    Q_PROPERTY(bool installed MEMBER installed)
    Q_PROPERTY(bool isDefault MEMBER isDefault)

public:
    QString filename;
    QString filesize;
    QByteArray md5sum;
    bool calcHash = false;
    bool installed = false;
    bool isDefault = false;
};
Q_DECLARE_METATYPE(ModelInfo)

class HashAndSaveFile : public QObject
{
    Q_OBJECT
public:
    HashAndSaveFile();

public Q_SLOTS:
    void hashAndSave(const QString &hash, const QString &saveFilePath,
        QTemporaryFile *tempFile, QNetworkReply *modelReply);

Q_SIGNALS:
    void hashAndSaveFinished(bool success,
        QTemporaryFile *tempFile, QNetworkReply *modelReply);

private:
    QThread m_hashAndSaveThread;
};

class Download : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<ModelInfo> modelList READ modelList NOTIFY modelListChanged)
    Q_PROPERTY(QString downloadLocalModelsPath READ downloadLocalModelsPath
                   WRITE setDownloadLocalModelsPath
                   NOTIFY downloadLocalModelsPathChanged)

public:
    static Download *globalInstance();

    QList<ModelInfo> modelList() const;
    Q_INVOKABLE void updateModelList();
    Q_INVOKABLE void downloadModel(const QString &modelFile);
    Q_INVOKABLE void cancelDownload(const QString &modelFile);
    Q_INVOKABLE QString defaultLocalModelsPath() const;
    Q_INVOKABLE QString downloadLocalModelsPath() const;
    Q_INVOKABLE void setDownloadLocalModelsPath(const QString &modelPath);

private Q_SLOTS:
    void handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
    void handleJsonDownloadFinished();
    void handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void handleModelDownloadFinished();
    void handleHashAndSaveFinished(bool success,
        QTemporaryFile *tempFile, QNetworkReply *modelReply);
    void handleReadyRead();

Q_SIGNALS:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal, const QString &modelFile);
    void downloadFinished(const QString &modelFile);
    void modelListChanged();
    void downloadLocalModelsPathChanged();
    void requestHashAndSave(const QString &hash, const QString &saveFilePath,
        QTemporaryFile *tempFile, QNetworkReply *modelReply);

private:
    void parseJsonFile(const QByteArray &jsonData);

    HashAndSaveFile *m_hashAndSave;
    QMap<QString, ModelInfo> m_modelMap;
    QNetworkAccessManager m_networkManager;
    QMap<QNetworkReply*, QTemporaryFile*> m_activeDownloads;
    QString m_downloadLocalModelsPath;

private:
    explicit Download();
    ~Download() {}
    friend class MyDownload;
};

#endif // DOWNLOAD_H
