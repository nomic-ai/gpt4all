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
    Q_PROPERTY(bool bestGPTJ MEMBER bestGPTJ)
    Q_PROPERTY(bool bestLlama MEMBER bestLlama)
    Q_PROPERTY(bool bestMPT MEMBER bestMPT)
    Q_PROPERTY(bool isChatGPT MEMBER isChatGPT)
    Q_PROPERTY(QString description MEMBER description)
    Q_PROPERTY(QString requires MEMBER requires)

public:
    QString filename;
    QString filesize;
    QByteArray md5sum;
    bool calcHash = false;
    bool installed = false;
    bool isDefault = false;
    bool bestGPTJ = false;
    bool bestLlama = false;
    bool bestMPT = false;
    bool isChatGPT = false;
    QString description;
    QString requires;
};
Q_DECLARE_METATYPE(ModelInfo)

struct ReleaseInfo {
    Q_GADGET
    Q_PROPERTY(QString version MEMBER version)
    Q_PROPERTY(QString notes MEMBER notes)
    Q_PROPERTY(QString contributors MEMBER contributors)

public:
    QString version;
    QString notes;
    QString contributors;
};

class HashAndSaveFile : public QObject
{
    Q_OBJECT
public:
    HashAndSaveFile();

public Q_SLOTS:
    void hashAndSave(const QString &hash, const QString &saveFilePath,
        QFile *tempFile, QNetworkReply *modelReply);

Q_SIGNALS:
    void hashAndSaveFinished(bool success,
        QFile *tempFile, QNetworkReply *modelReply);

private:
    QThread m_hashAndSaveThread;
};

class Download : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<ModelInfo> modelList READ modelList NOTIFY modelListChanged)
    Q_PROPERTY(bool hasNewerRelease READ hasNewerRelease NOTIFY hasNewerReleaseChanged)
    Q_PROPERTY(ReleaseInfo releaseInfo READ releaseInfo NOTIFY releaseInfoChanged)
    Q_PROPERTY(QString downloadLocalModelsPath READ downloadLocalModelsPath
                   WRITE setDownloadLocalModelsPath
                   NOTIFY downloadLocalModelsPathChanged)

public:
    static Download *globalInstance();

    QList<ModelInfo> modelList() const;
    ReleaseInfo releaseInfo() const;
    bool hasNewerRelease() const;
    Q_INVOKABLE void updateModelList();
    Q_INVOKABLE void updateReleaseNotes();
    Q_INVOKABLE void downloadModel(const QString &modelFile);
    Q_INVOKABLE void cancelDownload(const QString &modelFile);
    Q_INVOKABLE void installModel(const QString &modelFile, const QString &apiKey);
    Q_INVOKABLE void removeModel(const QString &modelFile);
    Q_INVOKABLE QString defaultLocalModelsPath() const;
    Q_INVOKABLE QString downloadLocalModelsPath() const;
    Q_INVOKABLE void setDownloadLocalModelsPath(const QString &modelPath);
    Q_INVOKABLE bool isFirstStart() const;

private Q_SLOTS:
    void handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
    void handleModelsJsonDownloadFinished();
    void handleReleaseJsonDownloadFinished();
    void handleErrorOccurred(QNetworkReply::NetworkError code);
    void handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void handleModelDownloadFinished();
    void handleHashAndSaveFinished(bool success,
        QFile *tempFile, QNetworkReply *modelReply);
    void handleReadyRead();

Q_SIGNALS:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal, const QString &modelFile);
    void downloadFinished(const QString &modelFile);
    void modelListChanged();
    void releaseInfoChanged();
    void hasNewerReleaseChanged();
    void downloadLocalModelsPathChanged();
    void requestHashAndSave(const QString &hash, const QString &saveFilePath,
        QFile *tempFile, QNetworkReply *modelReply);

private:
    void parseModelsJsonFile(const QByteArray &jsonData);
    void parseReleaseJsonFile(const QByteArray &jsonData);
    QString incompleteDownloadPath(const QString &modelFile);

    HashAndSaveFile *m_hashAndSave;
    QMap<QString, ModelInfo> m_modelMap;
    QMap<QString, ReleaseInfo> m_releaseMap;
    QNetworkAccessManager m_networkManager;
    QMap<QNetworkReply*, QFile*> m_activeDownloads;
    QString m_downloadLocalModelsPath;
    QDateTime m_startTime;

private:
    explicit Download();
    ~Download() {}
    friend class MyDownload;
};

#endif // DOWNLOAD_H
