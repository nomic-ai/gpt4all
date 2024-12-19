#include "toolmodel.h"

#include "codeinterpreter.h"

#include <QCoreApplication>
#include <QEvent>
#include <QGlobalStatic>

class MyToolModel: public ToolModel { };
Q_GLOBAL_STATIC(MyToolModel, toolModelInstance)
ToolModel *ToolModel::globalInstance()
{
    return toolModelInstance();
}

ToolModel::ToolModel()
    : QAbstractListModel(nullptr)
{
    QCoreApplication::instance()->installEventFilter(this);

    Tool* codeInterpreter = new CodeInterpreter;
    m_tools.append(codeInterpreter);
    m_toolMap.insert(codeInterpreter->function(), codeInterpreter);
}

bool ToolModel::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == QCoreApplication::instance() && ev->type() == QEvent::LanguageChange)
        emit dataChanged(index(0, 0), index(m_tools.size() - 1, 0));
    return false;
}
