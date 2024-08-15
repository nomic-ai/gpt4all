#include "localdocssearch.h"
#include "database.h"
#include "localdocs.h"
#include "mysettings.h"

#include <QCoreApplication>
#include <QDebug>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>

using namespace Qt::Literals::StringLiterals;

QString LocalDocsSearch::run(const QJsonObject &parameters, qint64 timeout)
{
    // Reset the error state
    m_error = ToolEnums::Error::NoError;
    m_errorString = QString();

    QList<QString> collections;
    QJsonArray collectionsArray = parameters["collections"].toArray();
    for (int i = 0; i < collectionsArray.size(); ++i)
        collections.append(collectionsArray[i].toString());
    const QString text = parameters["query"].toString();
    const int count = MySettings::globalInstance()->localDocsRetrievalSize();
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

QJsonObject LocalDocsSearch::paramSchema() const
{
    static const QString localParamSchema = R"({
        "collections": {
          "type": "array",
          "items": {
            "type": "string"
          },
          "description": "The collections to search",
          "required": true,
          "modelGenerated": false,
          "userConfigured": false
        },
        "query": {
          "type": "string",
          "description": "The query to search",
          "required": true
        },
        "count": {
          "type": "integer",
          "description": "The number of excerpts to return",
          "required": true,
          "modelGenerated": false
        }
      })";

    static const QJsonDocument localJsonDoc = QJsonDocument::fromJson(localParamSchema.toUtf8());
    Q_ASSERT(!localJsonDoc.isNull() && localJsonDoc.isObject());
    return localJsonDoc.object();
}

QJsonObject LocalDocsSearch::exampleParams() const
{
    static const QString example = R"({
        "query": "the 44th president of the United States"
      })";
    static const QJsonDocument exampleDoc = QJsonDocument::fromJson(example.toUtf8());
    Q_ASSERT(!exampleDoc.isNull() && exampleDoc.isObject());
    return exampleDoc.object();
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
