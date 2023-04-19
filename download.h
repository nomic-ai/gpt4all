#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include <QObject>
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QVariant>

struct ModelInfo {
    Q_GADGET
    Q_PROPERTY(QString filename MEMBER filename)
    Q_PROPERTY(QByteArray md5sum MEMBER md5sum)
    Q_PROPERTY(bool installed MEMBER installed)
    Q_PROPERTY(bool isDefault MEMBER isDefault)

public:
    QString filename;
    QByteArray md5sum;
    bool installed = false;
    bool isDefault = false;
};
Q_DECLARE_METATYPE(ModelInfo)

class Download : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<ModelInfo> modelList READ modelList NOTIFY modelListChanged)

public:
    static Download *globalInstance();

    QList<ModelInfo> modelList() const;
    Q_INVOKABLE void updateModelList();
    Q_INVOKABLE void downloadModel(const QString &modelFile);
    Q_INVOKABLE void cancelDownload(const QString &modelFile);

public Q_SLOTS:
    void handleJsonDownloadFinished();
    void handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void handleModelDownloadFinished();

Q_SIGNALS:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal, const QString &modelFile);
    void downloadFinished(const QString &modelFile);
    void modelListChanged();

private:
    void parseJsonFile(const QByteArray &jsonData);

    QMap<QString, ModelInfo> m_modelMap;
    QNetworkAccessManager m_networkManager;
    QVector<QNetworkReply*> m_activeDownloads;

private:
    explicit Download();
    ~Download() {}
    friend class MyDownload;
};

#endif // DOWNLOAD_H
