#include "chatlistmodel.h"

#include "database.h" // IWYU pragma: keep
#include "mysettings.h"

#include <QDataStream>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QGlobalStatic>
#include <QGuiApplication>
#include <QIODevice>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <Qt>

#include <algorithm>
#include <memory>

static constexpr quint32 CHAT_FORMAT_MAGIC   = 0xF5D553CC;
static constexpr qint32  CHAT_FORMAT_VERSION = 12;

class MyChatListModel: public ChatListModel { };
Q_GLOBAL_STATIC(MyChatListModel, chatListModelInstance)
ChatListModel *ChatListModel::globalInstance()
{
    return chatListModelInstance();
}

ChatListModel::ChatListModel()
    : QAbstractListModel(nullptr) {

        QCoreApplication::instance()->installEventFilter(this);
}

bool ChatListModel::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == QCoreApplication::instance() && ev->type() == QEvent::LanguageChange)
        emit dataChanged(index(0, 0), index(m_chats.size() - 1, 0));
    return false;
}

void ChatListModel::loadChats()
{
    addChat();

    ChatsRestoreThread *thread = new ChatsRestoreThread;
    connect(thread, &ChatsRestoreThread::chatRestored, this, &ChatListModel::restoreChat, Qt::QueuedConnection);
    connect(thread, &ChatsRestoreThread::finished, this, &ChatListModel::chatsRestoredFinished, Qt::QueuedConnection);
    connect(thread, &ChatsRestoreThread::finished, thread, &QObject::deleteLater);
    thread->start();

    m_chatSaver = std::make_unique<ChatSaver>();
    connect(this, &ChatListModel::requestSaveChats, m_chatSaver.get(), &ChatSaver::saveChats, Qt::QueuedConnection);
    connect(m_chatSaver.get(), &ChatSaver::saveChatsFinished, this, &ChatListModel::saveChatsFinished, Qt::QueuedConnection);
    // save chats on application quit
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &ChatListModel::saveChatsSync);

    connect(MySettings::globalInstance(), &MySettings::serverChatChanged, this, &ChatListModel::handleServerEnabledChanged);
}

void ChatListModel::removeChatFile(Chat *chat) const
{
    Q_ASSERT(chat != m_serverChat);
    const QString savePath = MySettings::globalInstance()->modelPath();
    QFile file(savePath + "/gpt4all-" + chat->id() + ".chat");
    if (!file.exists())
        return;
    bool success = file.remove();
    if (!success)
        qWarning() << "ERROR: Couldn't remove chat file:" << file.fileName();
}

ChatSaver::ChatSaver()
    : QObject(nullptr)
{
    moveToThread(&m_thread);
    m_thread.start();
}

ChatSaver::~ChatSaver()
{
    m_thread.quit();
    m_thread.wait();
}

QVector<Chat *> ChatListModel::getChatsToSave() const
{
    QVector<Chat *> toSave;
    for (auto *chat : m_chats)
        if (chat != m_serverChat && !chat->isNewChat())
            toSave << chat;
    return toSave;
}

void ChatListModel::saveChats()
{
    auto toSave = getChatsToSave();
    if (toSave.isEmpty()) {
        emit saveChatsFinished();
        return;
    }

    emit requestSaveChats(toSave);
}

void ChatListModel::saveChatsForQuit()
{
    saveChats();
    m_startedFinalSave = true;
}

void ChatListModel::saveChatsSync()
{
    auto toSave = getChatsToSave();
    if (!m_startedFinalSave && !toSave.isEmpty())
        m_chatSaver->saveChats(toSave);
}

void ChatSaver::saveChats(const QVector<Chat *> &chats)
{
    // we can be called from the main thread instead of a worker thread at quit time, so take a lock
    QMutexLocker locker(&m_mutex);

    QElapsedTimer timer;
    timer.start();
    const QString savePath = MySettings::globalInstance()->modelPath();
    qsizetype nSavedChats = 0;
    for (Chat *chat : chats) {
        if (!chat->needsSave())
            continue;
        ++nSavedChats;

        QString fileName = "gpt4all-" + chat->id() + ".chat";
        QString filePath = savePath + "/" + fileName;
        QFile originalFile(filePath);
        QFile tempFile(filePath + ".tmp"); // Temporary file

        bool success = tempFile.open(QIODevice::WriteOnly);
        if (!success) {
            qWarning() << "ERROR: Couldn't save chat to temporary file:" << tempFile.fileName();
            continue;
        }
        QDataStream out(&tempFile);

        out << CHAT_FORMAT_MAGIC;
        out << CHAT_FORMAT_VERSION;
        out.setVersion(QDataStream::Qt_6_2);

        qDebug() << "serializing chat" << fileName;
        if (!chat->serialize(out, CHAT_FORMAT_VERSION)) {
            qWarning() << "ERROR: Couldn't serialize chat to file:" << tempFile.fileName();
            tempFile.remove();
            continue;
        }

        chat->setNeedsSave(false);
        if (originalFile.exists())
            originalFile.remove();
        tempFile.rename(filePath);
    }

    qint64 elapsedTime = timer.elapsed();
    qDebug() << "serializing chats took" << elapsedTime << "ms, saved" << nSavedChats << "/" << chats.size() << "chats";
    emit saveChatsFinished();
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
        for (const QString &f : fileNames) {
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
        const QString savePath = MySettings::globalInstance()->modelPath();
        QDir dir(savePath);
        dir.setNameFilters(QStringList() << "gpt4all-*.chat");
        QStringList fileNames = dir.entryList();
        for (const QString &f : fileNames) {
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
                qWarning() << "WARNING: Chat file version" << version << "is not supported:" << file.fileName();
                continue;
            }
            if (version > CHAT_FORMAT_VERSION) {
                qWarning().nospace() << "WARNING: Chat file is from a future version (have " << version << " want "
                                     << CHAT_FORMAT_VERSION << "): " << file.fileName();
                continue;
            }

            if (version < 2)
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

            if (version < 2)
                in.setVersion(QDataStream::Qt_6_2);
        }

        qDebug() << "deserializing chat" << f.file;

        auto chat = std::make_unique<Chat>();
        chat->moveToThread(qGuiApp->thread());
        bool ok = chat->deserialize(in, version);
        if (!ok) {
            qWarning() << "ERROR: Couldn't deserialize chat from file:" << file.fileName();
        } else if (!in.atEnd()) {
            qWarning().nospace() << "error loading chat from " << file.fileName() << ": extra data at end of file";
        } else {
            emit chatRestored(chat.release());
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

    beginInsertRows(QModelIndex(), m_chats.size(), m_chats.size());
    m_chats.append(chat);
    endInsertRows();
}

void ChatListModel::chatsRestoredFinished()
{
    addServerChat();
}

void ChatListModel::handleServerEnabledChanged()
{
    if (MySettings::globalInstance()->serverChat() || m_serverChat != m_currentChat)
        return;

    Chat *nextChat = get(0);
    Q_ASSERT(nextChat && nextChat != m_serverChat);
    setCurrentChat(nextChat);
}
