#include "localdocssearch.h"
#include "database.h"
#include "localdocs.h"

#include <QCoreApplication>
#include <QDebug>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QThread>

using namespace Qt::Literals::StringLiterals;

QString LocalDocsSearch::run(const QJsonObject &parameters, qint64 timeout)
{
    QList<QString> collections;
    QJsonArray collectionsArray = parameters["collections"].toArray();
    for (int i = 0; i < collectionsArray.size(); ++i)
        collections.append(collectionsArray[i].toString());
    const QString text = parameters["text"].toString();
    const int count = parameters["count"].toInt();
    QThread workerThread;
    LocalDocsWorker worker;
    worker.moveToThread(&workerThread);
    connect(&worker, &LocalDocsWorker::finished, &workerThread, &QThread::quit, Qt::DirectConnection);
    connect(&workerThread, &QThread::started, [&worker, collections, text, count]() {
        worker.request(collections, text, count);
    });
    workerThread.start();
    bool timedOut = !workerThread.wait(timeout);
    if (timedOut) {
        m_error = ToolEnums::Error::TimeoutError;
        m_errorString = tr("ERROR: localdocs timeout");
    }

    workerThread.wait(timeout);
    workerThread.quit();
    workerThread.wait();
    return worker.response();
}

LocalDocsWorker::LocalDocsWorker()
    : QObject(nullptr)
{
    // The following are blocking operations and will block the calling thread
    connect(this, &LocalDocsWorker::requestRetrieveFromDB, LocalDocs::globalInstance()->database(),
        &Database::retrieveFromDB, Qt::BlockingQueuedConnection);
}

void LocalDocsWorker::request(const QList<QString> &collections, const QString &text, int count)
{
    QString jsonResult;
    emit requestRetrieveFromDB(collections, text, count, jsonResult); // blocks
    m_response = jsonResult;
    emit finished();
}
