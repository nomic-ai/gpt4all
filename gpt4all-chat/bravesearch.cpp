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

QPair<QString, QList<SourceExcerpt>> BraveSearch::search(const QString &apiKey, const QString &query, int topK, unsigned long timeout)
{
    QThread workerThread;
    BraveAPIWorker worker;
    worker.moveToThread(&workerThread);
    connect(&worker, &BraveAPIWorker::finished, &workerThread, &QThread::quit, Qt::DirectConnection);
    connect(this, &BraveSearch::request, &worker, &BraveAPIWorker::request, Qt::QueuedConnection);
    workerThread.start();
    emit request(apiKey, query, topK);
    workerThread.wait(timeout);
    workerThread.quit();
    workerThread.wait();
    return worker.response();
}

void BraveAPIWorker::request(const QString &apiKey, const QString &query, int topK)
{
    m_topK = topK;
    QUrl jsonUrl("https://api.search.brave.com/res/v1/web/search");
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("q", query);
    jsonUrl.setQuery(urlQuery);
    QNetworkRequest request(jsonUrl);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);

    request.setRawHeader("X-Subscription-Token", apiKey.toUtf8());
//    request.setRawHeader("Accept-Encoding", "gzip");
    request.setRawHeader("Accept", "application/json");

    m_networkManager = new QNetworkAccessManager(this);
    QNetworkReply *reply = m_networkManager->get(request);
    connect(qGuiApp, &QCoreApplication::aboutToQuit, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::finished, this, &BraveAPIWorker::handleFinished);
    connect(reply, &QNetworkReply::errorOccurred, this, &BraveAPIWorker::handleErrorOccurred);
}

static QPair<QString, QList<SourceExcerpt>> cleanBraveResponse(const QByteArray& jsonResponse, qsizetype topK = 1)
{
    QJsonParseError err;
    QJsonDocument document = QJsonDocument::fromJson(jsonResponse, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "ERROR: Couldn't parse: " << jsonResponse << err.errorString();
        return QPair<QString, QList<SourceExcerpt>>();
    }

    QJsonObject searchResponse = document.object();
    QJsonObject cleanResponse;
    QString query;
    QJsonArray cleanArray;

    QList<SourceExcerpt> infos;

    if (searchResponse.contains("query")) {
        QJsonObject queryObj = searchResponse["query"].toObject();
        if (queryObj.contains("original")) {
            query = queryObj["original"].toString();
        }
    }

    if (searchResponse.contains("mixed")) {
        QJsonObject mixedResults = searchResponse["mixed"].toObject();
        QJsonArray mainResults = mixedResults["main"].toArray();

        for (int i = 0; i < std::min(mainResults.size(), topK); ++i) {
            QJsonObject m = mainResults[i].toObject();
            QString r_type = m["type"].toString();
            int idx = m["index"].toInt();
            QJsonObject resultsObject = searchResponse[r_type].toObject();
            QJsonArray resultsArray = resultsObject["results"].toArray();

            QJsonValue cleaned;
            SourceExcerpt info;
            if (r_type == "web") {
                // For web data - add a single output from the search
                QJsonObject resultObj = resultsArray[idx].toObject();
                QStringList selectedKeys = {"type", "title", "url", "description", "date", "extra_snippets"};
                QJsonObject cleanedObj;
                for (const auto& key : selectedKeys) {
                    if (resultObj.contains(key)) {
                        cleanedObj.insert(key, resultObj[key]);
                    }
                }

                info.date = resultObj["date"].toString();
                info.text = resultObj["description"].toString(); // fixme
                info.url = resultObj["url"].toString();
                QJsonObject meta_url = resultObj["meta_url"].toObject();
                info.favicon = meta_url["favicon"].toString();
                info.title = resultObj["title"].toString();

                cleaned = cleanedObj;
            } else if (r_type == "faq") {
                // For faq data - take a list of all the questions & answers
                QStringList selectedKeys = {"type", "question", "answer", "title", "url"};
                QJsonArray cleanedArray;
                for (const auto& q : resultsArray) {
                    QJsonObject qObj = q.toObject();
                    QJsonObject cleanedObj;
                    for (const auto& key : selectedKeys) {
                        if (qObj.contains(key)) {
                            cleanedObj.insert(key, qObj[key]);
                        }
                    }
                    cleanedArray.append(cleanedObj);
                }
                cleaned = cleanedArray;
            } else if (r_type == "infobox") {
                QJsonObject resultObj = resultsArray[idx].toObject();
                QStringList selectedKeys = {"type", "title", "url", "description", "long_desc"};
                QJsonObject cleanedObj;
                for (const auto& key : selectedKeys) {
                    if (resultObj.contains(key)) {
                        cleanedObj.insert(key, resultObj[key]);
                    }
                }
                cleaned = cleanedObj;
            } else if (r_type == "videos") {
                QStringList selectedKeys = {"type", "url", "title", "description", "date"};
                QJsonArray cleanedArray;
                for (const auto& q : resultsArray) {
                    QJsonObject qObj = q.toObject();
                    QJsonObject cleanedObj;
                    for (const auto& key : selectedKeys) {
                        if (qObj.contains(key)) {
                            cleanedObj.insert(key, qObj[key]);
                        }
                    }
                    cleanedArray.append(cleanedObj);
                }
                cleaned = cleanedArray;
            } else if (r_type == "locations") {
                QStringList selectedKeys = {"type", "title", "url", "description", "coordinates", "postal_address", "contact", "rating", "distance", "zoom_level"};
                QJsonArray cleanedArray;
                for (const auto& q : resultsArray) {
                    QJsonObject qObj = q.toObject();
                    QJsonObject cleanedObj;
                    for (const auto& key : selectedKeys) {
                        if (qObj.contains(key)) {
                            cleanedObj.insert(key, qObj[key]);
                        }
                    }
                    cleanedArray.append(cleanedObj);
                }
                cleaned = cleanedArray;
            } else if (r_type == "news") {
                QStringList selectedKeys = {"type", "title", "url", "description"};
                QJsonArray cleanedArray;
                for (const auto& q : resultsArray) {
                    QJsonObject qObj = q.toObject();
                    QJsonObject cleanedObj;
                    for (const auto& key : selectedKeys) {
                        if (qObj.contains(key)) {
                            cleanedObj.insert(key, qObj[key]);
                        }
                    }
                    cleanedArray.append(cleanedObj);
                }
                cleaned = cleanedArray;
            } else {
                cleaned = QJsonValue();
            }

            infos.append(info);
            cleanArray.append(cleaned);
        }
    }

    cleanResponse.insert("query", query);
    cleanResponse.insert("top_k", cleanArray);
    QJsonDocument cleanedDoc(cleanResponse);

//    qDebug().noquote() << document.toJson(QJsonDocument::Indented);
//    qDebug().noquote() << cleanedDoc.toJson(QJsonDocument::Indented);

    return qMakePair(cleanedDoc.toJson(QJsonDocument::Indented), infos);
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
