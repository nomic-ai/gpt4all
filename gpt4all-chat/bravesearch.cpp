#include "bravesearch.h"

#include <QCoreApplication>
#include <QDebug>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QThread>
#include <QUrl>
#include <QUrlQuery>

using namespace Qt::Literals::StringLiterals;

QString BraveSearch::run(const QJsonObject &parameters, qint64 timeout)
{
    const QString apiKey = parameters["apiKey"].toString();
    const QString query = parameters["query"].toString();
    const int count = parameters["count"].toInt();
    QThread workerThread;
    BraveAPIWorker worker;
    worker.moveToThread(&workerThread);
    connect(&worker, &BraveAPIWorker::finished, &workerThread, &QThread::quit, Qt::DirectConnection);
    connect(&workerThread, &QThread::started, [&worker, apiKey, query, count]() {
        worker.request(apiKey, query, count);
    });
    workerThread.start();
    workerThread.wait(timeout);
    workerThread.quit();
    workerThread.wait();
    return worker.response();
}

void BraveAPIWorker::request(const QString &apiKey, const QString &query, int topK)
{
    m_topK = topK;

    // Documentation on the brave web search:
    // https://api.search.brave.com/app/documentation/web-search/get-started
    QUrl jsonUrl("https://api.search.brave.com/res/v1/web/search");

    // Documentation on the query options:
    //https://api.search.brave.com/app/documentation/web-search/query
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("q", query);
    urlQuery.addQueryItem("count", QString::number(topK));
    urlQuery.addQueryItem("result_filter", "web");
    urlQuery.addQueryItem("extra_snippets", "true");
    jsonUrl.setQuery(urlQuery);
    QNetworkRequest request(jsonUrl);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);
    request.setRawHeader("X-Subscription-Token", apiKey.toUtf8());
    request.setRawHeader("Accept", "application/json");
    m_networkManager = new QNetworkAccessManager(this);
    QNetworkReply *reply = m_networkManager->get(request);
    connect(qGuiApp, &QCoreApplication::aboutToQuit, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::finished, this, &BraveAPIWorker::handleFinished);
    connect(reply, &QNetworkReply::errorOccurred, this, &BraveAPIWorker::handleErrorOccurred);
}

static QString cleanBraveResponse(const QByteArray& jsonResponse, qsizetype topK = 1)
{
    // This parses the response from brave and formats it in json that conforms to the de facto
    // standard in SourceExcerpts::fromJson(...)
    QJsonParseError err;
    QJsonDocument document = QJsonDocument::fromJson(jsonResponse, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "ERROR: Couldn't parse brave response: " << jsonResponse << err.errorString();
        return QString();
    }

    QString query;
    QJsonObject searchResponse = document.object();
    QJsonObject cleanResponse;
    QJsonArray cleanArray;

    if (searchResponse.contains("query")) {
        QJsonObject queryObj = searchResponse["query"].toObject();
        if (queryObj.contains("original"))
            query = queryObj["original"].toString();
    }

    if (searchResponse.contains("mixed")) {
        QJsonObject mixedResults = searchResponse["mixed"].toObject();
        QJsonArray mainResults = mixedResults["main"].toArray();
        QJsonObject resultsObject = searchResponse["web"].toObject();
        QJsonArray resultsArray = resultsObject["results"].toArray();

        for (int i = 0; i < std::min(mainResults.size(), resultsArray.size()); ++i) {
            QJsonObject m = mainResults[i].toObject();
            QString r_type = m["type"].toString();
            Q_ASSERT(r_type == "web");
            const int idx = m["index"].toInt();

            QJsonObject resultObj = resultsArray[idx].toObject();
            QStringList selectedKeys = {"type", "title", "url", "description"};
            QJsonObject result;
            for (const auto& key : selectedKeys)
                if (resultObj.contains(key))
                    result.insert(key, resultObj[key]);

            if (resultObj.contains("page_age"))
                result.insert("date", resultObj["page_age"]);

            QJsonArray excerpts;
            if (resultObj.contains("extra_snippets")) {
                QJsonArray snippets = resultObj["extra_snippets"].toArray();
                for (int i = 0; i < snippets.size(); ++i) {
                    QString snippet = snippets[i].toString();
                    QJsonObject excerpt;
                    excerpt.insert("text", snippet);
                    excerpts.append(excerpt);
                }
            }
            result.insert("excerpts", excerpts);
            cleanArray.append(QJsonValue(result));
        }
    }

    cleanResponse.insert("query", query);
    cleanResponse.insert("results", cleanArray);
    QJsonDocument cleanedDoc(cleanResponse);
//    qDebug().noquote() << document.toJson(QJsonDocument::Indented);
//    qDebug().noquote() << cleanedDoc.toJson(QJsonDocument::Indented);
    return cleanedDoc.toJson(QJsonDocument::Compact);
}

void BraveAPIWorker::handleFinished()
{
    QNetworkReply *jsonReply = qobject_cast<QNetworkReply *>(sender());
    Q_ASSERT(jsonReply);

    if (jsonReply->error() == QNetworkReply::NoError && jsonReply->isFinished()) {
        QByteArray jsonData = jsonReply->readAll();
        jsonReply->deleteLater();
        m_response = cleanBraveResponse(jsonData, m_topK);
    } else {
        QByteArray jsonData = jsonReply->readAll();
        qWarning() << "ERROR: Could not search brave" << jsonReply->error() << jsonReply->errorString() << jsonData;
        jsonReply->deleteLater();
    }
}

void BraveAPIWorker::handleErrorOccurred(QNetworkReply::NetworkError code)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    Q_ASSERT(reply);
    qWarning().noquote() << "ERROR: BraveAPIWorker::handleErrorOccurred got HTTP Error" << code << "response:"
                         << reply->errorString();
    emit finished();
}
