#pragma once

#include "Manifest.h"
#include "State.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QHash>
#include <QList>
#include <QMap>
#include <QObject>
#include <QtGlobal>
#include <QVersionNumber>
#include <QSslError>
#include <QThread>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QCoreApplication>

namespace gpt4all {
namespace download {

class HashFile : public QObject
{
public:
    HashFile() : QObject(nullptr) {}
    void hashAndInstall(const QString &expected, QCryptographicHash::Algorithm algo, QFile *temp, const QString &file, QNetworkReply *reply);
};

class Download : public QObject
{
public:
    static Download *instance();
    Q_INVOKABLE void driveFetchAndInstall();
    Q_INVOKABLE void downloadManifest();
    Q_INVOKABLE void downloadInstaller();
    Q_INVOKABLE void cancelDownload();
    Q_INVOKABLE void installInstaller(QString &expected, QFile *temp, QNetworkReply *installerResponse);

private Q_SLOTS:
    void handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
    void handleErrorOccurred(QNetworkReply::NetworkError code);
    void handleInstallerDownloadFinished();
    void handleReadyRead();

private:
    explicit Download();
    ~Download();
    QNetworkReply * downloadInMemory(QUrl &url);
    QNetworkReply * downloadLargeFile(QUrl &url);
    QIODevice * handleManifestRequestResponse(QNetworkReply * reply);
    gpt4all::manifest::ManifestFile *manifest;
    QNetworkAccessManager m_networkManager;
    QMap<QNetworkReply*, QFile*> download_tracking;
    HashFile *saver;
    QDateTime m_startTime;
    QString platform_ext;
    QFile *downloadPath;
    friend class Downloader;
};

}
}
