#include "sourceexcerpt.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

QString SourceExcerpt::toJson(const QList<SourceExcerpt> &sources)
{
    if (sources.isEmpty())
        return QString();

    QJsonArray resultsArray;
    for (const auto &source : sources) {
        QJsonObject sourceObj;
        sourceObj["date"] = source.date;
        sourceObj["collection"] = source.collection;
        sourceObj["path"] = source.path;
        sourceObj["file"] = source.file;
        sourceObj["url"] = source.url;
        sourceObj["favicon"] = source.favicon;
        sourceObj["title"] = source.title;
        sourceObj["author"] = source.author;
        sourceObj["description"] = source.description;

        QJsonArray excerptsArray;
        for (const auto &excerpt : source.excerpts) {
            QJsonObject excerptObj;
            excerptObj["text"] = excerpt.text;
            if (excerpt.page != -1)
                excerptObj["page"] = excerpt.page;
            if (excerpt.from != -1)
                excerptObj["from"] = excerpt.from;
            if (excerpt.to != -1)
                excerptObj["to"] = excerpt.to;
            excerptsArray.append(excerptObj);
        }
        sourceObj["excerpts"] = excerptsArray;

        resultsArray.append(sourceObj);
    }

    QJsonObject jsonObj;
    jsonObj["results"] = resultsArray;

    QJsonDocument doc(jsonObj);
    return doc.toJson(QJsonDocument::Compact);
}

QList<SourceExcerpt> SourceExcerpt::fromJson(const QString &json, QString &errorString)
{
    if (json.isEmpty())
        return QList<SourceExcerpt>();

    QJsonParseError err;
    QJsonDocument document = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) {
        errorString = err.errorString();
        return QList<SourceExcerpt>();
    }

    QJsonObject jsonObject = document.object();
    Q_ASSERT(jsonObject.contains("results"));
    if (!jsonObject.contains("results")) {
        errorString = "json does not contain results array";
        return QList<SourceExcerpt>();
    }

    QList<SourceExcerpt> excerpts;
    QJsonArray results = jsonObject["results"].toArray();
    for (int i = 0; i < results.size(); ++i) {
        QJsonObject result = results[i].toObject();
        if (!result.contains("date")) {
            errorString = "result does not contain required date field";
            return QList<SourceExcerpt>();
        }

        if (!result.contains("excerpts") || !result["excerpts"].isArray()) {
            errorString = "result does not contain required excerpts array";
            return QList<SourceExcerpt>();
        }

        QJsonArray textExcerpts = result["excerpts"].toArray();
        if (textExcerpts.isEmpty()) {
            errorString = "result excerpts array is empty";
            return QList<SourceExcerpt>();
        }

        SourceExcerpt source;
        source.date = result["date"].toString();
        if (result.contains("collection"))
            source.collection = result["collection"].toString();
        if (result.contains("path"))
            source.path = result["path"].toString();
        if (result.contains("file"))
            source.file = result["file"].toString();
        if (result.contains("url"))
            source.url = result["url"].toString();
        if (result.contains("favicon"))
            source.favicon = result["favicon"].toString();
        if (result.contains("title"))
            source.title = result["title"].toString();
        if (result.contains("author"))
            source.author = result["author"].toString();
        if (result.contains("description"))
            source.author = result["description"].toString();

        for (int i = 0; i < textExcerpts.size(); ++i) {
            if (!textExcerpts[i].isObject()) {
                errorString = "result excerpt is not an object";
                return QList<SourceExcerpt>();
            }
            QJsonObject excerptObj = textExcerpts[i].toObject();
            if (!excerptObj.contains("text")) {
                errorString = "result excerpt is does not have text field";
                return QList<SourceExcerpt>();
            }
            Excerpt excerpt;
            excerpt.text = excerptObj["text"].toString();
            if (excerptObj.contains("page"))
                excerpt.page = excerptObj["page"].toInt();
            if (excerptObj.contains("from"))
                excerpt.from = excerptObj["from"].toInt();
            if (excerptObj.contains("to"))
                excerpt.to = excerptObj["to"].toInt();
            source.excerpts.append(excerpt);
        }
        excerpts.append(source);
    }
    return excerpts;
}
