#include "chatlistmodel.h"

#include <QFile>
#include <QDataStream>

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
    QSettings settings;
    QFileInfo settingsInfo(settings.fileName());
    QString settingsPath = settingsInfo.absolutePath();
    QFile file(settingsPath + "/gpt4all-" + chat->id() + ".chat");
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

    QSettings settings;
    QFileInfo settingsInfo(settings.fileName());
    QString settingsPath = settingsInfo.absolutePath();
    for (Chat *chat : m_chats) {
        QString fileName = "gpt4all-" + chat->id() + ".chat";
        QFile file(settingsPath + "/" + fileName);
        bool success = file.open(QIODevice::WriteOnly);
        if (!success) {
            qWarning() << "ERROR: Couldn't save chat to file:" << file.fileName();
            continue;
        }
        QDataStream out(&file);
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
    QSettings settings;
    QFileInfo settingsInfo(settings.fileName());
    QString settingsPath = settingsInfo.absolutePath();
    QDir dir(settingsPath);
    dir.setNameFilters(QStringList() << "gpt4all-*.chat");
    QStringList fileNames = dir.entryList();
    beginResetModel();
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
        file.close();
    }
    std::sort(m_chats.begin(), m_chats.end(), [](const Chat* a, const Chat* b) {
        return a->creationDate() > b->creationDate();
    });
    endResetModel();
}
