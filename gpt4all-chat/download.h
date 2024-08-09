#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QHash>
#include <QList>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QSslError>
#include <QString>
#include <QThread>
#include <QtGlobal>

class QByteArray;

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
    void hashAndSave(const QString &hash, QCryptographicHash::Algorithm a, const QString &saveFilePath,
        QFile *tempFile, QNetworkReply *modelReply);

Q_SIGNALS:
    void hashAndSaveFinished(bool success, const QString &error,
        QFile *tempFile, QNetworkReply *modelReply);

private:
    QThread m_hashAndSaveThread;
};

class Download : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasNewerRelease READ hasNewerRelease NOTIFY hasNewerReleaseChanged)
    Q_PROPERTY(ReleaseInfo releaseInfo READ releaseInfo NOTIFY releaseInfoChanged)
    Q_PROPERTY(QString latestNews READ latestNews NOTIFY latestNewsChanged)

public:
    static Download *globalInstance();

    static std::strong_ordering compareAppVersions(const QString &a, const QString &b);
    ReleaseInfo releaseInfo() const;
    bool hasNewerRelease() const;
    QString latestNews() const { return m_latestNews; }
    Q_INVOKABLE void downloadModel(const QString &modelFile);
    Q_INVOKABLE void cancelDownload(const QString &modelFile);
    Q_INVOKABLE void installModel(const QString &modelFile, const QString &apiKey);
    Q_INVOKABLE void installCompatibleModel(const QString &modelName, const QString &apiKey, const QString &baseUrl);
    Q_INVOKABLE void removeModel(const QString &modelFile);
    Q_INVOKABLE bool isFirstStart(bool writeVersion = false) const;

public Q_SLOTS:
    void updateLatestNews();
    void updateReleaseNotes();

private Q_SLOTS:
    void handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
    void handleReleaseJsonDownloadFinished();
    void handleLatestNewsDownloadFinished();
    void handleErrorOccurred(QNetworkReply::NetworkError code);
    void handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void handleModelDownloadFinished();
    void handleHashAndSaveFinished(bool success, const QString &error,
        QFile *tempFile, QNetworkReply *modelReply);
    void handleReadyRead();

Q_SIGNALS:
    void releaseInfoChanged();
    void hasNewerReleaseChanged();
    void requestHashAndSave(const QString &hash, QCryptographicHash::Algorithm a, const QString &saveFilePath,
        QFile *tempFile, QNetworkReply *modelReply);
    void latestNewsChanged();
    void toastMessage(const QString &message);

private:
    void parseReleaseJsonFile(const QByteArray &jsonData);
    QString incompleteDownloadPath(const QString &modelFile);
    bool hasRetry(const QString &filename) const;
    bool shouldRetry(const QString &filename);
    void clearRetry(const QString &filename);

    HashAndSaveFile *m_hashAndSave;
    QMap<QString, ReleaseInfo> m_releaseMap;
    QString m_latestNews;
    QNetworkAccessManager m_networkManager;
    QMap<QNetworkReply*, QFile*> m_activeDownloads;
    QHash<QString, int> m_activeRetries;
    QDateTime m_startTime;

private:
    explicit Download();
    ~Download() {}
    friend class MyDownload;
};

#endif // DOWNLOAD_H
