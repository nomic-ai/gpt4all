#include "llm.h"
#include "download.h"
#include "network.h"
#include "llmodel/gptj.h"
#include "llmodel/llamamodel.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QResource>
#include <QSettings>
#include <fstream>

class MyLLM: public LLM { };
Q_GLOBAL_STATIC(MyLLM, llmInstance)
LLM *LLM::globalInstance()
{
    return llmInstance();
}

LLM::LLM()
    : QObject{nullptr}
{
    if (m_chats.isEmpty())
        addChat();
    connect(Download::globalInstance(), &Download::modelListChanged,
        this, &LLM::modelListChanged, Qt::QueuedConnection);
}

QList<QString> LLM::modelList() const
{
    Q_ASSERT(currentChat());
    // Build a model list from exepath and from the localpath
    QList<QString> list;

    QString exePath = QCoreApplication::applicationDirPath() + QDir::separator();
    QString localPath = Download::globalInstance()->downloadLocalModelsPath();

    {
        QDir dir(exePath);
        dir.setNameFilters(QStringList() << "ggml-*.bin");
        QStringList fileNames = dir.entryList();
        for (QString f : fileNames) {
            QString filePath = exePath + f;
            QFileInfo info(filePath);
            QString name = info.completeBaseName().remove(0, 5);
            if (info.exists()) {
                if (name == currentChat()->modelName())
                    list.prepend(name);
                else
                    list.append(name);
            }
        }
    }

    if (localPath != exePath) {
        QDir dir(localPath);
        dir.setNameFilters(QStringList() << "ggml-*.bin");
        QStringList fileNames = dir.entryList();
        for (QString f : fileNames) {
            QString filePath = localPath + f;
            QFileInfo info(filePath);
            QString name = info.completeBaseName().remove(0, 5);
            if (info.exists() && !list.contains(name)) { // don't allow duplicates
                if (name == currentChat()->modelName())
                    list.prepend(name);
                else
                    list.append(name);
            }
        }
    }

    if (list.isEmpty()) {
        if (exePath != localPath) {
            qWarning() << "ERROR: Could not find any applicable models in"
                       << exePath << "nor" << localPath;
        } else {
            qWarning() << "ERROR: Could not find any applicable models in"
                       << exePath;
        }
        return QList<QString>();
    }

    return list;
}

bool LLM::checkForUpdates() const
{
    Network::globalInstance()->sendCheckForUpdates();

#if defined(Q_OS_LINUX)
    QString tool("maintenancetool");
#elif defined(Q_OS_WINDOWS)
    QString tool("maintenancetool.exe");
#elif defined(Q_OS_DARWIN)
    QString tool("../../../maintenancetool.app/Contents/MacOS/maintenancetool");
#endif

    QString fileName = QCoreApplication::applicationDirPath()
        + "/../" + tool;
    if (!QFileInfo::exists(fileName)) {
        qDebug() << "Couldn't find tool at" << fileName << "so cannot check for updates!";
        return false;
    }

    return QProcess::startDetached(fileName);
}

bool LLM::isRecalc() const
{
    Q_ASSERT(currentChat());
    return currentChat()->isRecalc();
}

Chat *LLM::currentChat() const
{
    return chatFromId(m_currentChat);
}

QList<QString> LLM::chatList() const
{
    return m_chats.keys();
}

QString LLM::addChat()
{
    Chat *newChat = new Chat(this);
    m_chats.insert(newChat->id(), newChat);
    emit chatListChanged();
    setCurrentChatFromId(newChat->id());
    return newChat->id();
}

void LLM::removeChat(const QString &id)
{
    if (!m_chats.contains(id)) {
        qDebug() << "WARNING: Removing chat with id" << id;
        return;
    }

    const bool chatIsCurrent = id == m_currentChat;
    Chat *chat = m_chats.value(id);
    disconnectChat(chat);
    m_chats.remove(id);
    emit chatListChanged();
    delete chat;
    if (m_chats.isEmpty())
        addChat();
    else
        setCurrentChatFromId(chatList().first());
}

Chat *LLM::chatFromId(const QString &id) const
{
    if (!m_chats.contains(id)) {
        qDebug() << "WARNING: Getting chat from id" << id;
        return nullptr;
    }
    return m_chats.value(id);
}

void LLM::setCurrentChatFromId(const QString &id)
{
    if (!m_chats.contains(id)) {
        qDebug() << "ERROR: Setting current chat from id" << id;
        return;
    }

    // On load this can be empty as we add a new chat in ctor this method will be called
    if (!m_currentChat.isEmpty()) {
        Chat *curr = currentChat();
        Q_ASSERT(curr);
        disconnect(curr);
    }

    Chat *newCurr = m_chats.value(id);
    connectChat(newCurr);
    m_currentChat = id;
    emit currentChatChanged();
}

void LLM::connectChat(Chat *chat)
{
    connect(chat, &Chat::modelNameChanged,
        this, &LLM::modelListChanged, Qt::QueuedConnection);
    connect(chat, &Chat::recalcChanged,
        this, &LLM::recalcChanged, Qt::QueuedConnection);
    connect(chat, &Chat::responseChanged,
        this, &LLM::responseChanged, Qt::QueuedConnection);
}

void LLM::disconnectChat(Chat *chat)
{
    disconnect(chat);
}
