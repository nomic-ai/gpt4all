#include "chatlistmodel.h"
#include "download.h"
#include "llm.h"

#include <QFile>
#include <QDataStream>

#define CHAT_FORMAT_MAGIC 0xF5D553CC
#define CHAT_FORMAT_VERSION 2

ChatListModel::ChatListModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_newChat(nullptr)
    , m_dummyChat(nullptr)
    , m_serverChat(nullptr)
    , m_currentChat(nullptr)
    , m_shouldSaveChats(false)
{
    addDummyChat();

    ChatsRestoreThread *thread = new ChatsRestoreThread;
    connect(thread, &ChatsRestoreThread::chatRestored, this, &ChatListModel::restoreChat);
    connect(thread, &ChatsRestoreThread::finished, this, &ChatListModel::chatsRestoredFinished);
    connect(thread, &ChatsRestoreThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

bool ChatListModel::shouldSaveChats() const
{
    return m_shouldSaveChats;
}

void ChatListModel::setShouldSaveChats(bool b)
{
    if (m_shouldSaveChats == b)
        return;
    m_shouldSaveChats = b;
    emit shouldSaveChatsChanged();
}

bool ChatListModel::shouldSaveChatGPTChats() const
{
    return m_shouldSaveChatGPTChats;
}

void ChatListModel::setShouldSaveChatGPTChats(bool b)
{
    if (m_shouldSaveChatGPTChats == b)
        return;
    m_shouldSaveChatGPTChats = b;
    emit shouldSaveChatGPTChatsChanged();
}

void ChatListModel::removeChatFile(Chat *chat) const
{
    Q_ASSERT(chat != m_serverChat);
    const QString savePath = Download::globalInstance()->downloadLocalModelsPath();
    QFile file(savePath + "/gpt4all-" + chat->id() + ".chat");
    if (!file.exists())
        return;
    bool success = file.remove();
    if (!success)
        qWarning() << "ERROR: Couldn't remove chat file:" << file.fileName();
}

void ChatListModel::saveChats() const
{
    QElapsedTimer timer;
    timer.start();
    const QString savePath = Download::globalInstance()->downloadLocalModelsPath();
    for (Chat *chat : m_chats) {
        if (chat == m_serverChat)
            continue;
        const bool isChatGPT = chat->modelName().startsWith("chatgpt-");
        if (!isChatGPT && !m_shouldSaveChats)
            continue;
        if (isChatGPT && !m_shouldSaveChatGPTChats)
            continue;
        QString fileName = "gpt4all-" + chat->id() + ".chat";
        QFile file(savePath + "/" + fileName);
        bool success = file.open(QIODevice::WriteOnly);
        if (!success) {
            qWarning() << "ERROR: Couldn't save chat to file:" << file.fileName();
            continue;
        }
        QDataStream out(&file);

        out << (quint32)CHAT_FORMAT_MAGIC;
        out << (qint32)CHAT_FORMAT_VERSION;
        out.setVersion(QDataStream::Qt_6_2);

        qDebug() << "serializing chat" << fileName;
        if (!chat->serialize(out, CHAT_FORMAT_VERSION)) {
            qWarning() << "ERROR: Couldn't serialize chat to file:" << file.fileName();
            file.remove();
        }
        file.close();
    }
    qint64 elapsedTime = timer.elapsed();
    qDebug() << "serializing chats took:" << elapsedTime << "ms";
}

void ChatsRestoreThread::run()
{
    QElapsedTimer timer;
    timer.start();
    struct FileInfo {
        bool oldFile;
        qint64 creationDate;
        QString file;
    };
    QList<FileInfo> files;
    {
        // Look for any files in the original spot which was the settings config directory
        QSettings settings;
        QFileInfo settingsInfo(settings.fileName());
        QString settingsPath = settingsInfo.absolutePath();
        QDir dir(settingsPath);
        dir.setNameFilters(QStringList() << "gpt4all-*.chat");
        QStringList fileNames = dir.entryList();
        for (QString f : fileNames) {
            QString filePath = settingsPath + "/" + f;
            QFile file(filePath);
            bool success = file.open(QIODevice::ReadOnly);
            if (!success) {
                qWarning() << "ERROR: Couldn't restore chat from file:" << file.fileName();
                continue;
            }
            QDataStream in(&file);
            FileInfo info;
            info.oldFile = true;
            info.file = filePath;
            in >> info.creationDate;
            files.append(info);
            file.close();
        }
    }
    {
        const QString savePath = Download::globalInstance()->downloadLocalModelsPath();
        QDir dir(savePath);
        dir.setNameFilters(QStringList() << "gpt4all-*.chat");
        QStringList fileNames = dir.entryList();
        for (QString f : fileNames) {
            QString filePath = savePath + "/" + f;
            QFile file(filePath);
            bool success = file.open(QIODevice::ReadOnly);
            if (!success) {
                qWarning() << "ERROR: Couldn't restore chat from file:" << file.fileName();
                continue;
            }
            QDataStream in(&file);
            // Read and check the header
            quint32 magic;
            in >> magic;
            if (magic != CHAT_FORMAT_MAGIC) {
                qWarning() << "ERROR: Chat file has bad magic:" << file.fileName();
                continue;
            }

            // Read the version
            qint32 version;
            in >> version;
            if (version < 1) {
                qWarning() << "ERROR: Chat file has non supported version:" << file.fileName();
                continue;
            }

            if (version <= 1)
                in.setVersion(QDataStream::Qt_6_2);

            FileInfo info;
            info.oldFile = false;
            info.file = filePath;
            in >> info.creationDate;
            files.append(info);
            file.close();
        }
    }
    std::sort(files.begin(), files.end(), [](const FileInfo &a, const FileInfo &b) {
        return a.creationDate > b.creationDate;
    });

    for (FileInfo &f : files) {
            QFile file(f.file);
            bool success = file.open(QIODevice::ReadOnly);
            if (!success) {
                qWarning() << "ERROR: Couldn't restore chat from file:" << file.fileName();
                continue;
            }
            QDataStream in(&file);

            qint32 version = 0;
            if (!f.oldFile) {
                // Read and check the header
                quint32 magic;
                in >> magic;
                if (magic != CHAT_FORMAT_MAGIC) {
                    qWarning() << "ERROR: Chat file has bad magic:" << file.fileName();
                    continue;
                }

                // Read the version
                in >> version;
                if (version < 1) {
                    qWarning() << "ERROR: Chat file has non supported version:" << file.fileName();
                    continue;
                }

                if (version <= 1)
                    in.setVersion(QDataStream::Qt_6_2);
            }

            qDebug() << "deserializing chat" << f.file;

            Chat *chat = new Chat;
            chat->moveToThread(qApp->thread());
            if (!chat->deserialize(in, version)) {
                qWarning() << "ERROR: Couldn't deserialize chat from file:" << file.fileName();
                file.remove();
            } else {
                emit chatRestored(chat);
            }
            if (f.oldFile)
               file.remove(); // No longer storing in this directory
            file.close();
    }

    qint64 elapsedTime = timer.elapsed();
    qDebug() << "deserializing chats took:" << elapsedTime << "ms";
}

void ChatListModel::restoreChat(Chat *chat)
{
    chat->setParent(this);
    connect(chat, &Chat::nameChanged, this, &ChatListModel::nameChanged);
    connect(chat, &Chat::modelLoadingError, this, &ChatListModel::handleModelLoadingError);

    if (m_dummyChat) {
        beginResetModel();
        m_chats = QList<Chat*>({chat});
        setCurrentChat(chat);
        delete m_dummyChat;
        m_dummyChat = nullptr;
        endResetModel();
    } else {
        beginInsertRows(QModelIndex(), m_chats.size(), m_chats.size());
        m_chats.append(chat);
        endInsertRows();
    }
}

void ChatListModel::chatsRestoredFinished()
{
    if (m_dummyChat) {
        beginResetModel();
        Chat *dummy = m_dummyChat;
        m_dummyChat = nullptr;
        m_chats.clear();
        addChat();
        delete dummy;
        endResetModel();
    }

    if (m_chats.isEmpty())
        addChat();

    addServerChat();
}

void ChatListModel::handleServerEnabledChanged()
{
    if (LLM::globalInstance()->serverEnabled() || m_serverChat != m_currentChat)
        return;

    Chat *nextChat = get(0);
    Q_ASSERT(nextChat && nextChat != m_serverChat);
    setCurrentChat(nextChat);
}