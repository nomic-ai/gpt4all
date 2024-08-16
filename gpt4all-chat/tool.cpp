#include "tool.h"

#include <QJsonDocument>

QJsonObject filterModelGeneratedProperties(const QJsonObject &inputObject) {
    QJsonObject filteredObject;
    for (const QString &key : inputObject.keys()) {
        QJsonObject propertyObject = inputObject.value(key).toObject();
        if (!propertyObject.contains("modelGenerated") || propertyObject["modelGenerated"].toBool())
            filteredObject.insert(key, propertyObject);
    }
    return filteredObject;
}

jinja2::Value Tool::jinjaValue() const
{
    QJsonDocument doc(filterModelGeneratedProperties(paramSchema()));
    QString p(doc.toJson(QJsonDocument::Compact));

    QJsonDocument exampleDoc(exampleParams());
    QString e(exampleDoc.toJson(QJsonDocument::Compact));

    jinja2::ValuesMap params {
        { "name", name().toStdString() },
        { "description", description().toStdString() },
        { "function", function().toStdString() },
        { "paramSchema", p.toStdString() },
        { "exampleParams", e.toStdString() }
    };
    return params;
}
