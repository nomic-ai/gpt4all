#include "chatlistmodel.h"
#include "download.h"

#include <QFile>
#include <QDataStream>

#define CHAT_FORMAT_MAGIC 0xF5D553CC
#define CHAT_FORMAT_VERSION 100

ChatListModel::ChatListModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_newChat(nullptr)
    , m_dummyChat(nullptr)
    , m_currentChat(nullptr)
    , m_shouldSaveChats(false)
{
    addDummyChat();

    ChatsRestoreThread *thread = new ChatsRestoreThread;
    connect(thread, &ChatsRestoreThread::chatsRestored, this, &ChatListModel::restoreChats);
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

void ChatListModel::removeChatFile(Chat *chat) const
{
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
    if (!m_shouldSaveChats)
        return;

    QElapsedTimer timer;
    timer.start();
    const QString savePath = Download::globalInstance()->downloadLocalModelsPath();
    for (Chat *chat : m_chats) {
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
        out.setVersion(QDataStream::Qt_6_5);

        qDebug() << "serializing chat" << fileName;
        if (!chat->serialize(out)) {
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
    QList<Chat*> chats;
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
            Chat *chat = new Chat;
            chat->moveToThread(qApp->thread());
            if (!chat->deserialize(in)) {
                qWarning() << "ERROR: Couldn't deserialize chat from file:" << file.fileName();
                file.remove();
            } else {
                chats.append(chat);
            }
            qDebug() << "deserializing chat" << f;
            file.remove(); // No longer storing in this directory
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
            if (version < 100) {
                qWarning() << "ERROR: Chat file has non supported version:" << file.fileName();
                continue;
            }

            if (version <= 100)
                in.setVersion(QDataStream::Qt_6_5);

            Chat *chat = new Chat;
            chat->moveToThread(qApp->thread());
            if (!chat->deserialize(in)) {
                qWarning() << "ERROR: Couldn't deserialize chat from file:" << file.fileName();
                file.remove();
            } else {
                chats.append(chat);
            }
            qDebug() << "deserializing chat" << f;
            file.close();
        }
    }
    std::sort(chats.begin(), chats.end(), [](const Chat* a, const Chat* b) {
        return a->creationDate() > b->creationDate();
    });
    qint64 elapsedTime = timer.elapsed();
    qDebug() << "deserializing chats took:" << elapsedTime << "ms";

    emit chatsRestored(chats);
}

void ChatListModel::restoreChats(const QList<Chat*> &chats)
{
    for (Chat* chat : chats) {
        chat->setParent(this);
        connect(chat, &Chat::nameChanged, this, &ChatListModel::nameChanged);
    }

    beginResetModel();

    // Setup the new chats
    m_chats = chats;

    if (!m_chats.isEmpty()) {
        Chat *firstChat = m_chats.first();
        if (firstChat->chatModel()->count() < 2)
            setNewChat(firstChat);
        else
            setCurrentChat(firstChat);
    } else
        addChat();

    // Clean up the dummy
    delete m_dummyChat;
    m_dummyChat = nullptr;

    endResetModel();
}
