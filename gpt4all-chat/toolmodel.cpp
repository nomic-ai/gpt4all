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
    m_tools.append(localDocsSearch);
    m_toolMap.insert(localDocsSearch->function(), localDocsSearch);

    Tool *braveSearch = new BraveSearch;
    m_tools.append(braveSearch);
    m_toolMap.insert(braveSearch->function(), braveSearch);
}

bool ToolModel::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == QCoreApplication::instance() && ev->type() == QEvent::LanguageChange)
        emit dataChanged(index(0, 0), index(m_tools.size() - 1, 0));
    return false;
}
