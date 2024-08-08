#include "toolmodel.h"

#include <QGlobalStatic>
#include <QGuiApplication>
#include <QJsonDocument>

#include "bravesearch.h"
#include "localdocssearch.h"

class MyToolModel: public ToolModel { };
Q_GLOBAL_STATIC(MyToolModel, toolModelInstance)
ToolModel *ToolModel::globalInstance()
{
    return toolModelInstance();
}

ToolModel::ToolModel()
    : QAbstractListModel(nullptr) {

    QCoreApplication::instance()->installEventFilter(this);

    Tool* localDocsSearch = new LocalDocsSearch;
    localDocsSearch->name = tr("LocalDocs search");
    localDocsSearch->description = tr("Search the local docs");
    localDocsSearch->function = "localdocs_search";
    localDocsSearch->isBuiltin = true;
    localDocsSearch->excerpts = true;
    localDocsSearch->forceUsage = true; // FIXME: persistent setting
    localDocsSearch->isEnabled = true; // FIXME: persistent setting

    QString localParamSchema = R"({
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

    QJsonDocument localJsonDoc = QJsonDocument::fromJson(localParamSchema.toUtf8());
    Q_ASSERT(!localJsonDoc.isNull() && localJsonDoc.isObject());
    localDocsSearch->paramSchema = localJsonDoc.object();
    m_tools.append(localDocsSearch);
    m_toolMap.insert(localDocsSearch->function, localDocsSearch);

    Tool *braveSearch = new BraveSearch;
    braveSearch->name = tr("Brave web search");
    braveSearch->description = tr("Search the web using brave.com");
    braveSearch->function = "brave_search";
    braveSearch->isBuiltin = true;
    braveSearch->excerpts = true;
    braveSearch->forceUsage = false; // FIXME: persistent setting
    braveSearch->isEnabled = false; // FIXME: persistent setting

    QString braveParamSchema = R"({
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

    QJsonDocument braveJsonDoc = QJsonDocument::fromJson(braveParamSchema.toUtf8());
    Q_ASSERT(!braveJsonDoc.isNull() && braveJsonDoc.isObject());
    braveSearch->paramSchema = braveJsonDoc.object();
    m_tools.append(braveSearch);
    m_toolMap.insert(braveSearch->function, braveSearch);
}

bool ToolModel::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == QCoreApplication::instance() && ev->type() == QEvent::LanguageChange)
        emit dataChanged(index(0, 0), index(m_tools.size() - 1, 0));
    return false;
}
