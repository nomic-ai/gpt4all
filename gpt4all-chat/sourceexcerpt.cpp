#include "sourceexcerpt.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

QList<SourceExcerpt> SourceExcerpt::fromJson(const QString &json, QString &errorString)
{
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
            source.collection = result["text"].toString();
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
            SourceExcerpt excerpt;
            excerpt.date        = source.date;
            excerpt.collection  = source.collection;
            excerpt.path        = source.path;
            excerpt.file        = source.file;
            excerpt.url         = source.url;
            excerpt.favicon     = source.favicon;
            excerpt.title       = source.title;
            excerpt.author      = source.author;
            if (!textExcerpts[i].isObject()) {
                errorString = "result excerpt is not an object";
                return QList<SourceExcerpt>();
            }
            QJsonObject excerptObj = textExcerpts[i].toObject();
            if (!excerptObj.contains("text")) {
                errorString = "result excerpt is does not have text field";
                return QList<SourceExcerpt>();
            }
            excerpt.text = excerptObj["text"].toString();
            if (excerptObj.contains("page"))
                excerpt.page = excerptObj["page"].toInt();
            if (excerptObj.contains("from"))
                excerpt.from = excerptObj["from"].toInt();
            if (excerptObj.contains("to"))
                excerpt.to = excerptObj["to"].toInt();
            excerpts.append(excerpt);
        }
    }
    return excerpts;
}
