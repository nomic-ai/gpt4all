#ifndef CHATLISTMODEL_H
#define CHATLISTMODEL_H

#include <QAbstractListModel>
#include "chat.h"

class ChatListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(Chat *currentChat READ currentChat WRITE setCurrentChat NOTIFY currentChatChanged)

public:
    explicit ChatListModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
    }

    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent)
        return m_chats.size();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_chats.size())
            return QVariant();

        const Chat *item = m_chats.at(index.row());
        switch (role) {
            case IdRole:
                return item->id();
            case NameRole:
                return item->name();
        }

        return QVariant();
    }

    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> roles;
        roles[IdRole] = "id";
        roles[NameRole] = "name";
        return roles;
    }

    Q_INVOKABLE Chat* addChat()
    {
        Chat *newChat = new Chat(this);
        beginInsertRows(QModelIndex(), 0, 0);
        m_chats.prepend(newChat);
        endInsertRows();
        emit countChanged();
        setCurrentChat(newChat);
        return newChat;
    }

    Q_INVOKABLE void removeChat(Chat* chat)
    {
        if (!m_chats.contains(chat)) {
            qDebug() << "WARNING: Removing chat failed with id" << chat->id();
            return;
        }

        const bool chatIsCurrent = chat == m_currentChat;
        emit disconnectChat(chat);
        const int index = m_chats.indexOf(chat);
        if (m_chats.count() < 2) {
            addChat();
        } else {
            int nextIndex;
            if (index == m_chats.count() - 1)
                nextIndex = index - 1;
            else
                nextIndex = index + 1;
            Chat *nextChat = get(nextIndex);
            Q_ASSERT(nextChat);
            setCurrentChat(nextChat);
        }
        beginRemoveRows(QModelIndex(), index, index);
        m_chats.removeAll(chat);
        endRemoveRows();
        delete chat;
    }

    Chat *currentChat() const
    {
        return m_currentChat;
    }

    void setCurrentChat(Chat *chat)
    {
        if (!m_chats.contains(chat)) {
            qDebug() << "ERROR: Setting current chat failed with id" << chat->id();
            return;
        }

        if (m_currentChat) {
            if (m_currentChat->isModelLoaded())
                m_currentChat->unload();
            emit disconnect(m_currentChat);
        }

        emit connectChat(chat);
        m_currentChat = chat;
        if (!m_currentChat->isModelLoaded())
            m_currentChat->reload();
        emit currentChatChanged();
    }

    Q_INVOKABLE Chat* get(int index)
    {
        if (index < 0 || index >= m_chats.size()) return nullptr;
        return m_chats.at(index);
    }


    int count() const { return m_chats.size(); }

Q_SIGNALS:
    void countChanged();
    void connectChat(Chat*);
    void disconnectChat(Chat*);
    void currentChatChanged();

private:
    Chat* m_currentChat;
    QList<Chat*> m_chats;
};

#endif // CHATITEMMODEL_H
