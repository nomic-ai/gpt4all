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
    , m_threadCount(std::min(4, (int32_t) std::thread::hardware_concurrency()))
{
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
        this, &LLM::aboutToQuit);

    m_chatListModel->restoreChats();
    if (m_chatListModel->count()) {
        Chat *firstChat = m_chatListModel->get(0);
        if (firstChat->chatModel()->count() < 2)
            m_chatListModel->setNewChat(firstChat);
        else
            m_chatListModel->setCurrentChat(firstChat);
    } else
        m_chatListModel->addChat();
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

int32_t LLM::threadCount() const
{
    return m_threadCount;
}

void LLM::setThreadCount(int32_t n_threads)
{
    if (n_threads <= 0)
        n_threads = std::min(4, (int32_t) std::thread::hardware_concurrency());
    m_threadCount = n_threads;
    emit threadCountChanged();
}

void LLM::aboutToQuit()
{
    m_chatListModel->saveChats();
}
