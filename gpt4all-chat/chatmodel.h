#ifndef CHATMODEL_H
#define CHATMODEL_H

#include <QAbstractListModel>
#include <QtQml>
#include <QDataStream>

struct ChatItem
{
    Q_GADGET
    Q_PROPERTY(int id MEMBER id)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString value MEMBER value)
    Q_PROPERTY(QString prompt MEMBER prompt)
    Q_PROPERTY(QString newResponse MEMBER newResponse)
    Q_PROPERTY(bool currentResponse MEMBER currentResponse)
    Q_PROPERTY(bool stopped MEMBER stopped)
    Q_PROPERTY(bool thumbsUpState MEMBER thumbsUpState)
    Q_PROPERTY(bool thumbsDownState MEMBER thumbsDownState)
    Q_PROPERTY(QString references MEMBER references)
    Q_PROPERTY(QList<QString> referencesContext MEMBER referencesContext)

public:
    int id = 0;
    QString name;
    QString value;
    QString prompt;
    QString newResponse;
    QString references;
    QList<QString> referencesContext;
    bool currentResponse = false;
    bool stopped = false;
    bool thumbsUpState = false;
    bool thumbsDownState = false;
};
Q_DECLARE_METATYPE(ChatItem)

class ChatModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit ChatModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        ValueRole,
        PromptRole,
        NewResponseRole,
        CurrentResponseRole,
        StoppedRole,
        ThumbsUpStateRole,
        ThumbsDownStateRole,
        ReferencesRole,
        ReferencesContextRole
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent)
        return m_chatItems.size();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_chatItems.size())
            return QVariant();

        const ChatItem &item = m_chatItems.at(index.row());
        switch (role) {
            case IdRole:
                return item.id;
            case NameRole:
                return item.name;
            case ValueRole:
                return item.value;
            case PromptRole:
                return item.prompt;
            case NewResponseRole:
                return item.newResponse;
            case CurrentResponseRole:
                return item.currentResponse;
            case StoppedRole:
                return item.stopped;
            case ThumbsUpStateRole:
                return item.thumbsUpState;
            case ThumbsDownStateRole:
                return item.thumbsDownState;
            case ReferencesRole:
                return item.references;
            case ReferencesContextRole:
                return item.referencesContext;
        }

        return QVariant();
    }

    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> roles;
        roles[IdRole] = "id";
        roles[NameRole] = "name";
        roles[ValueRole] = "value";
        roles[PromptRole] = "prompt";
        roles[NewResponseRole] = "newResponse";
        roles[CurrentResponseRole] = "currentResponse";
        roles[StoppedRole] = "stopped";
        roles[ThumbsUpStateRole] = "thumbsUpState";
        roles[ThumbsDownStateRole] = "thumbsDownState";
        roles[ReferencesRole] = "references";
        roles[ReferencesContextRole] = "referencesContext";
        return roles;
    }

    void appendPrompt(const QString &name, const QString &value)
    {
        ChatItem item;
        item.name = name;
        item.value = value;
        beginInsertRows(QModelIndex(), m_chatItems.size(), m_chatItems.size());
        m_chatItems.append(item);
        endInsertRows();
        emit countChanged();
    }

    void appendResponse(const QString &name, const QString &prompt)
    {
        ChatItem item;
        item.id = m_chatItems.count(); // This is only relevant for responses
        item.name = name;
        item.prompt = prompt;
        item.currentResponse = true;
        beginInsertRows(QModelIndex(), m_chatItems.size(), m_chatItems.size());
        m_chatItems.append(item);
        endInsertRows();
        emit countChanged();
    }

    Q_INVOKABLE void clear()
    {
        if (m_chatItems.isEmpty()) return;

        beginResetModel();
        m_chatItems.clear();
        endResetModel();
        emit countChanged();
    }

    Q_INVOKABLE ChatItem get(int index)
    {
        if (index < 0 || index >= m_chatItems.size()) return ChatItem();
        return m_chatItems.at(index);
    }

    Q_INVOKABLE void updateCurrentResponse(int index, bool b)
    {
        if (index < 0 || index >= m_chatItems.size()) return;

        ChatItem &item = m_chatItems[index];
        if (item.currentResponse != b) {
            item.currentResponse = b;
            emit dataChanged(createIndex(index, 0), createIndex(index, 0), {CurrentResponseRole});
        }
    }

    Q_INVOKABLE void updateStopped(int index, bool b)
    {
        if (index < 0 || index >= m_chatItems.size()) return;

        ChatItem &item = m_chatItems[index];
        if (item.stopped != b) {
            item.stopped = b;
            emit dataChanged(createIndex(index, 0), createIndex(index, 0), {StoppedRole});
        }
    }

    Q_INVOKABLE void updateValue(int index, const QString &value)
    {
        if (index < 0 || index >= m_chatItems.size()) return;

        ChatItem &item = m_chatItems[index];
        if (item.value != value) {
            item.value = value;
            emit dataChanged(createIndex(index, 0), createIndex(index, 0), {ValueRole});
        }
    }

    Q_INVOKABLE void updateReferences(int index, const QString &references, const QList<QString> &referencesContext)
    {
        if (index < 0 || index >= m_chatItems.size()) return;

        ChatItem &item = m_chatItems[index];
        if (item.references != references) {
            item.references = references;
            emit dataChanged(createIndex(index, 0), createIndex(index, 0), {ReferencesRole});
        }
        if (item.referencesContext != referencesContext) {
            item.referencesContext = referencesContext;
            emit dataChanged(createIndex(index, 0), createIndex(index, 0), {ReferencesContextRole});
        }
    }

    Q_INVOKABLE void updateThumbsUpState(int index, bool b)
    {
        if (index < 0 || index >= m_chatItems.size()) return;

        ChatItem &item = m_chatItems[index];
        if (item.thumbsUpState != b) {
            item.thumbsUpState = b;
            emit dataChanged(createIndex(index, 0), createIndex(index, 0), {ThumbsUpStateRole});
        }
    }

    Q_INVOKABLE void updateThumbsDownState(int index, bool b)
    {
        if (index < 0 || index >= m_chatItems.size()) return;

        ChatItem &item = m_chatItems[index];
        if (item.thumbsDownState != b) {
            item.thumbsDownState = b;
            emit dataChanged(createIndex(index, 0), createIndex(index, 0), {ThumbsDownStateRole});
        }
    }

    Q_INVOKABLE void updateNewResponse(int index, const QString &newResponse)
    {
        if (index < 0 || index >= m_chatItems.size()) return;

        ChatItem &item = m_chatItems[index];
        if (item.newResponse != newResponse) {
            item.newResponse = newResponse;
            emit dataChanged(createIndex(index, 0), createIndex(index, 0), {NewResponseRole});
        }
    }

    int count() const { return m_chatItems.size(); }

    bool serialize(QDataStream &stream, int version) const
    {
        stream << count();
        for (const auto &c : m_chatItems) {
            stream << c.id;
            stream << c.name;
            stream << c.value;
            stream << c.prompt;
            stream << c.newResponse;
            stream << c.currentResponse;
            stream << c.stopped;
            stream << c.thumbsUpState;
            stream << c.thumbsDownState;
            if (version > 2) {
                stream << c.references;
                stream << c.referencesContext;
            }
        }
        return stream.status() == QDataStream::Ok;
    }

    bool deserialize(QDataStream &stream, int version)
    {
        int size;
        stream >> size;
        for (int i = 0; i < size; ++i) {
            ChatItem c;
            stream >> c.id;
            stream >> c.name;
            stream >> c.value;
            stream >> c.prompt;
            stream >> c.newResponse;
            stream >> c.currentResponse;
            stream >> c.stopped;
            stream >> c.thumbsUpState;
            stream >> c.thumbsDownState;
            if (version > 2) {
                stream >> c.references;
                stream >> c.referencesContext;
            }
            beginInsertRows(QModelIndex(), m_chatItems.size(), m_chatItems.size());
            m_chatItems.append(c);
            endInsertRows();
        }
        emit countChanged();
        return stream.status() == QDataStream::Ok;
    }

    QVector<QPair<QString, QString>> text() const
    {
        QVector<QPair<QString, QString>> result;
        for (const auto &c : m_chatItems)
            result << qMakePair(c.name, c.value);
        return result;
    }

Q_SIGNALS:
    void countChanged();

private:

    QList<ChatItem> m_chatItems;
};

#endif // CHATMODEL_H
