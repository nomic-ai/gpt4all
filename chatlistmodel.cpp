#include "chatlistmodel.h"
#include "download.h"

#include <QFile>
#include <QDataStream>

#define CHAT_FORMAT_MAGIC 0xF5D553CC
#define CHAT_FORMAT_VERSION 100

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
}

void ChatListModel::restoreChats()
{
    beginResetModel();
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
            Chat *chat = new Chat(this);
            if (!chat->deserialize(in)) {
                qWarning() << "ERROR: Couldn't deserialize chat from file:" << file.fileName();
                file.remove();
            } else {
                connect(chat, &Chat::nameChanged, this, &ChatListModel::nameChanged);
                m_chats.append(chat);
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

            Chat *chat = new Chat(this);
            if (!chat->deserialize(in)) {
                qWarning() << "ERROR: Couldn't deserialize chat from file:" << file.fileName();
                file.remove();
            } else {
                connect(chat, &Chat::nameChanged, this, &ChatListModel::nameChanged);
                m_chats.append(chat);
            }
            qDebug() << "deserializing chat" << f;
            file.close();
        }
    }
    std::sort(m_chats.begin(), m_chats.end(), [](const Chat* a, const Chat* b) {
        return a->creationDate() > b->creationDate();
    });
    endResetModel();
}
