#ifndef TOOLMODEL_H
#define TOOLMODEL_H

#include "tool.h"

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QList>
#include <QString>
#include <QVariant>
#include <QtGlobal>

class ToolModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    static ToolModel *globalInstance();

    enum Roles {
        NameRole = Qt::UserRole + 1,
        DescriptionRole,
        FunctionRole,
        ParametersRole,
        SymbolicFormatRole,
        ExamplePromptRole,
        ExampleCallRole,
        ExampleReplyRole,
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent)
        return m_tools.size();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_tools.size())
            return QVariant();

        const Tool *item = m_tools.at(index.row());
        switch (role) {
            case NameRole:
                return item->name();
            case DescriptionRole:
                return item->description();
            case FunctionRole:
                return item->function();
            case ParametersRole:
                return QVariant::fromValue(item->parameters());
            case SymbolicFormatRole:
                return item->symbolicFormat();
            case ExamplePromptRole:
                return item->examplePrompt();
            case ExampleCallRole:
                return item->exampleCall();
            case ExampleReplyRole:
                return item->exampleReply();
        }

        return QVariant();
    }

    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> roles;
        roles[NameRole] = "name";
        roles[DescriptionRole] = "description";
        roles[FunctionRole] = "function";
        roles[ParametersRole] = "parameters";
        roles[SymbolicFormatRole] = "symbolicFormat";
        roles[ExamplePromptRole] = "examplePrompt";
        roles[ExampleCallRole] = "exampleCall";
        roles[ExampleReplyRole] = "exampleReply";
        return roles;
    }

    Q_INVOKABLE Tool* get(int index) const
    {
        if (index < 0 || index >= m_tools.size()) return nullptr;
        return m_tools.at(index);
    }

    Q_INVOKABLE Tool *get(const QString &id) const
    {
        if (!m_toolMap.contains(id)) return nullptr;
        return m_toolMap.value(id);
    }

    int count() const { return m_tools.size(); }

Q_SIGNALS:
    void countChanged();
    void valueChanged(int index, const QString &value);

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    explicit ToolModel();
    ~ToolModel() {}
    friend class MyToolModel;
    QList<Tool*> m_tools;
    QHash<QString, Tool*> m_toolMap;
};

#endif // TOOLMODEL_H
