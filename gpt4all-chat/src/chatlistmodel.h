#ifndef CHATLISTMODEL_H
#define CHATLISTMODEL_H

#include "chat.h"
#include "chatllm.h"
#include "chatmodel.h"

#include <QAbstractListModel>
#include <QByteArray>
#include <QDebug>
#include <QHash>
#include <QList>
#include <QMutex>
#include <QObject>
#include <QThread>
#include <QVariant>
#include <QVector>
#include <Qt>
#include <QtGlobal>
#include <QtLogging>

#include <memory>


class ChatsRestoreThread : public QThread
{
    Q_OBJECT
public:
    void run() override;

Q_SIGNALS:
    void chatRestored(Chat *chat);
};

class ChatSaver : public QObject
{
    Q_OBJECT
public:
    explicit ChatSaver();
    ~ChatSaver() override;

Q_SIGNALS:
    void saveChatsFinished();

public Q_SLOTS:
    void saveChats(const QVector<Chat*> &chats);

private:
    QThread m_thread;
    QMutex  m_mutex;
};

class ChatListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(Chat *currentChat READ currentChat WRITE setCurrentChat NOTIFY currentChatChanged)

public:
    static ChatListModel *globalInstance();

    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        SectionRole
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
            case SectionRole: {
                if (item == m_serverChat)
                    return QString();
                const QDate date = QDate::currentDate();
                const QDate itemDate = item->creationDate().date();
                if (date == itemDate)
                    return tr("TODAY");
                else if (itemDate >= date.addDays(-7))
                    return tr("THIS WEEK");
                else if (itemDate >= date.addMonths(-1))
                    return tr("THIS MONTH");
                else if (itemDate >= date.addMonths(-6))
                    return tr("LAST SIX MONTHS");
                else if (itemDate.year() == date.year())
                    return tr("THIS YEAR");
                else if (itemDate.year() == date.year() - 1)
                    return tr("LAST YEAR");
                else
                    return QString::number(itemDate.year());
            }
        }

        return QVariant();
    }

    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> roles;
        roles[IdRole] = "id";
        roles[NameRole] = "name";
        roles[SectionRole] = "section";
        return roles;
    }

    bool shouldSaveChats() const;
    void setShouldSaveChats(bool b);

    bool shouldSaveChatGPTChats() const;
    void setShouldSaveChatGPTChats(bool b);

    Q_INVOKABLE void loadChats();

    Q_INVOKABLE void addChat()
    {
        // Select the existing new chat if we already have one
        if (m_newChat) {
            setCurrentChat(m_newChat);
            return;
        }

        // Create a new chat pointer and connect it to determine when it is populated
        m_newChat = new Chat(this);
        connect(m_newChat->chatModel(), &ChatModel::countChanged,
            this, &ChatListModel::newChatCountChanged);
        connect(m_newChat, &Chat::nameChanged,
            this, &ChatListModel::nameChanged);

        beginInsertRows(QModelIndex(), 0, 0);
        m_chats.prepend(m_newChat);
        endInsertRows();
        emit countChanged();
        setCurrentChat(m_newChat);
    }

    Q_INVOKABLE void addServerChat()
    {
        // Create a new dummy chat pointer and don't connect it
        if (m_serverChat)
            return;

        m_serverChat = new Chat(Chat::server_tag, this);
        beginInsertRows(QModelIndex(), m_chats.size(), m_chats.size());
        m_chats.append(m_serverChat);
        endInsertRows();
        emit countChanged();
    }

    Q_INVOKABLE void removeChat(Chat* chat)
    {
        Q_ASSERT(chat != m_serverChat);
        if (!m_chats.contains(chat)) {
            qWarning() << "WARNING: Removing chat failed with id" << chat->id();
            return;
        }

        removeChatFile(chat);

        if (chat == m_newChat) {
            m_newChat->disconnect(this);
            m_newChat = nullptr;
        }

        chat->markForDeletion();

        const int index = m_chats.indexOf(chat);
        if (m_chats.count() < 3 /*m_serverChat included*/) {
            addChat();
        } else {
            int nextIndex;
            if (index == m_chats.count() - 2 /*m_serverChat is last*/)
                nextIndex = index - 1;
            else
                nextIndex = index + 1;
            Chat *nextChat = get(nextIndex);
            Q_ASSERT(nextChat);
            setCurrentChat(nextChat);
        }

        const int newIndex = m_chats.indexOf(chat);
        beginRemoveRows(QModelIndex(), newIndex, newIndex);
        m_chats.removeAll(chat);
        endRemoveRows();
        chat->unloadAndDeleteLater();
    }

    Chat *currentChat() const
    {
        return m_currentChat;
    }

    void setCurrentChat(Chat *chat)
    {
        if (!m_chats.contains(chat)) {
            qWarning() << "ERROR: Setting current chat failed with id" << chat->id();
            return;
        }

        if (m_currentChat && m_currentChat != m_serverChat)
            m_currentChat->unloadModel();
        m_currentChat = chat;
        emit currentChatChanged();
        if (!m_currentChat->isModelLoaded() && m_currentChat != m_serverChat)
            m_currentChat->trySwitchContextOfLoadedModel();
    }

    Q_INVOKABLE Chat* get(int index)
    {
        if (index < 0 || index >= m_chats.size()) return nullptr;
        return m_chats.at(index);
    }

    int count() const { return m_chats.size(); }

    // stop ChatLLM threads for clean shutdown
    void destroyChats()
    {
        for (auto *chat: m_chats) { chat->destroy(); }
        ChatLLM::destroyStore();
    }

    void removeChatFile(Chat *chat) const;
    Q_INVOKABLE void saveChats();
    Q_INVOKABLE void saveChatsForQuit();
    void restoreChat(Chat *chat);
    void chatsRestoredFinished();

public Q_SLOTS:
    void handleServerEnabledChanged();

Q_SIGNALS:
    void countChanged();
    void currentChatChanged();
    void requestSaveChats(const QVector<Chat*> &);
    void saveChatsFinished();

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private Q_SLOTS:
    // Used with QCoreApplication::aboutToQuit. Does not require an event loop.
    void saveChatsSync();

    void newChatCountChanged()
    {
        Q_ASSERT(m_newChat && m_newChat->chatModel()->count());
        m_newChat->chatModel()->disconnect(this);
        m_newChat = nullptr;
    }

    void nameChanged()
    {
        Chat *chat = qobject_cast<Chat *>(sender());
        if (!chat)
            return;

        int row = m_chats.indexOf(chat);
        if (row < 0 || row >= m_chats.size())
            return;

        QModelIndex index = createIndex(row, 0);
        emit dataChanged(index, index, {NameRole});
    }

    void printChats()
    {
        for (auto c : m_chats) {
            qDebug() << c->name()
                << (c == m_currentChat ? "currentChat: true" : "currentChat: false")
                << (c == m_newChat ? "newChat: true" : "newChat: false");
        }
    }

private:
    QVector<Chat *> getChatsToSave() const;

private:
    Chat* m_newChat = nullptr;
    Chat* m_serverChat = nullptr;
    Chat* m_currentChat = nullptr;
    QList<Chat*> m_chats;
    std::unique_ptr<ChatSaver> m_chatSaver;
    bool m_startedFinalSave = false;

private:
    explicit ChatListModel();
    ~ChatListModel() {}
    friend class MyChatListModel;
};

#endif // CHATITEMMODEL_H
