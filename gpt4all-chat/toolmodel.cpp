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
    connect(localDocsSearch, &Tool::usageModeChanged, this, &ToolModel::privacyScopeChanged);

    Tool *braveSearch = new BraveSearch;
    m_tools.append(braveSearch);
    m_toolMap.insert(braveSearch->function(), braveSearch);
    connect(braveSearch, &Tool::usageModeChanged, this, &ToolModel::privacyScopeChanged);
}

bool ToolModel::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == QCoreApplication::instance() && ev->type() == QEvent::LanguageChange)
        emit dataChanged(index(0, 0), index(m_tools.size() - 1, 0));
    return false;
}

ToolEnums::PrivacyScope ToolModel::privacyScope() const
{
    ToolEnums::PrivacyScope scope = ToolEnums::PrivacyScope::Local; // highest scope
    for (const Tool *t : m_tools)
        if (t->usageMode() != ToolEnums::UsageMode::Disabled)
            scope = std::min(scope, t->privacyScope());
    return scope;
}
