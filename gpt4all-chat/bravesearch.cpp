#include "bravesearch.h"
#include "mysettings.h"

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

BraveSearch::BraveSearch()
    : Tool(), m_error(ToolEnums::Error::NoError)
{
    connect(MySettings::globalInstance(), &MySettings::webSearchUsageModeChanged,
        this, &Tool::usageModeChanged);
}

QString BraveSearch::run(const QJsonObject &parameters, qint64 timeout)
{
    const QString apiKey = MySettings::globalInstance()->braveSearchAPIKey();
    const QString query = parameters["query"].toString();
    const int count = MySettings::globalInstance()->webSearchRetrievalSize();
    QThread workerThread;
    BraveAPIWorker worker;
    worker.moveToThread(&workerThread);
    connect(&worker, &BraveAPIWorker::finished, &workerThread, &QThread::quit, Qt::DirectConnection);
    connect(&workerThread, &QThread::started, [&worker, apiKey, query, count]() {
        worker.request(apiKey, query, count);
    });
    workerThread.start();
    bool timedOut = !workerThread.wait(timeout);
    if (timedOut) {
        m_error = ToolEnums::Error::TimeoutError;
        m_errorString = tr("ERROR: brave search timeout");
    } else {
        m_error = worker.error();
        m_errorString = worker.errorString();
    }
    workerThread.quit();
    workerThread.wait();
    return worker.response();
}

QJsonObject BraveSearch::paramSchema() const
{
    static const QString braveParamSchema = R"({
        "apiKey": {
          "type": "string",
          "description": "The api key to use",
          "required": true,
          "modelGenerated": false,
          "userConfigured": true
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

    static const QJsonDocument braveJsonDoc = QJsonDocument::fromJson(braveParamSchema.toUtf8());
    Q_ASSERT(!braveJsonDoc.isNull() && braveJsonDoc.isObject());
    return braveJsonDoc.object();
}

QJsonObject BraveSearch::exampleParams() const
{
    static const QString example = R"({
        "query": "the 44th president of the United States"
      })";
    static const QJsonDocument exampleDoc = QJsonDocument::fromJson(example.toUtf8());
    Q_ASSERT(!exampleDoc.isNull() && exampleDoc.isObject());
    return exampleDoc.object();
}

ToolEnums::UsageMode BraveSearch::usageMode() const
{
    return MySettings::globalInstance()->webSearchUsageMode();
}

void BraveAPIWorker::request(const QString &apiKey, const QString &query, int count)
{
    // Documentation on the brave web search:
    // https://api.search.brave.com/app/documentation/web-search/get-started
    QUrl jsonUrl("https://api.search.brave.com/res/v1/web/search");

    // Documentation on the query options:
    //https://api.search.brave.com/app/documentation/web-search/query
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("q", query);
    urlQuery.addQueryItem("count", QString::number(count));
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

QString BraveAPIWorker::cleanBraveResponse(const QByteArray& jsonResponse)
{
    // This parses the response from brave and formats it in json that conforms to the de facto
    // standard in SourceExcerpts::fromJson(...)
    QJsonParseError err;
    QJsonDocument document = QJsonDocument::fromJson(jsonResponse, &err);
    if (err.error != QJsonParseError::NoError) {
        m_error = ToolEnums::Error::UnknownError;
        m_errorString = QString(tr("ERROR: brave search could not parse json response: %1")).arg(jsonResponse);
        return QString();
    }

    QString query;
    QJsonObject searchResponse = document.object();
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
            QStringList selectedKeys = {"type", "title", "url"};
            QJsonObject result;
            for (const auto& key : selectedKeys)
                if (resultObj.contains(key))
                    result.insert(key, resultObj[key]);

            if (resultObj.contains("page_age"))
                result.insert("date", resultObj["page_age"]);
            else
                result.insert("date", QDate::currentDate().toString());

            QJsonArray excerpts;
            if (resultObj.contains("extra_snippets")) {
                QJsonArray snippets = resultObj["extra_snippets"].toArray();
                for (int i = 0; i < snippets.size(); ++i) {
                    QString snippet = snippets[i].toString();
                    QJsonObject excerpt;
                    excerpt.insert("text", snippet);
                    excerpts.append(excerpt);
                }
                if (resultObj.contains("description"))
                    result.insert("description", resultObj["description"]);
            } else {
                QJsonObject excerpt;
                excerpt.insert("text", resultObj["description"]);
            }
            result.insert("excerpts", excerpts);
            cleanArray.append(QJsonValue(result));
        }
    }

    QJsonObject cleanResponse;
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
        m_response = cleanBraveResponse(jsonData);
    } else {
        QByteArray jsonData = jsonReply->readAll();
        jsonReply->deleteLater();
    }
    emit finished();
}

void BraveAPIWorker::handleErrorOccurred(QNetworkReply::NetworkError code)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    Q_ASSERT(reply);
    m_error = ToolEnums::Error::UnknownError;
    m_errorString = QString(tr("ERROR: brave search code: %1 response: %2")).arg(code).arg(reply->errorString());
    emit finished();
}
