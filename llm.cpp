#include "llm.h"
#include "download.h"
#include "network.h"

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
    , m_chatListModel(new ChatListModel(this))
{
    // Should be in the same thread
    connect(Download::globalInstance(), &Download::modelListChanged,
        this, &LLM::modelListChanged, Qt::DirectConnection);
    connect(m_chatListModel, &ChatListModel::connectChat,
        this, &LLM::connectChat, Qt::DirectConnection);
    connect(m_chatListModel, &ChatListModel::disconnectChat,
        this, &LLM::disconnectChat, Qt::DirectConnection);

    if (!m_chatListModel->count())
        m_chatListModel->addChat();
}

QList<QString> LLM::modelList() const
{
    Q_ASSERT(m_chatListModel->currentChat());
    const Chat *currentChat = m_chatListModel->currentChat();
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
                if (name == currentChat->modelName())
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
                if (name == currentChat->modelName())
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
    Q_ASSERT(m_chatListModel->currentChat());
    return m_chatListModel->currentChat()->isRecalc();
}

void LLM::connectChat(Chat *chat)
{
    // Should be in the same thread
    connect(chat, &Chat::modelNameChanged, this, &LLM::modelListChanged, Qt::DirectConnection);
    connect(chat, &Chat::recalcChanged, this, &LLM::recalcChanged, Qt::DirectConnection);
    connect(chat, &Chat::responseChanged, this, &LLM::responseChanged, Qt::DirectConnection);
}

void LLM::disconnectChat(Chat *chat)
{
    disconnect(chat);
}
