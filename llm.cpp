#include "llm.h"
#include "config.h"
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

#if defined(__x86_64__) || defined(__i386__)
    if (QString(GPT4ALL_AVX_ONLY) == "OFF") {
        const bool avx(__builtin_cpu_supports("avx"));
        const bool avx2(__builtin_cpu_supports("avx2"));
        const bool fma(__builtin_cpu_supports("fma"));
        m_compatHardware = avx && avx2 && fma;
        qDebug() << "m_compatHardware" << m_compatHardware;
        emit compatHardwareChanged();
    }
#endif
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
