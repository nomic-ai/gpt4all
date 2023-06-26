#include "llm.h"
#include "config.h"
#include "chatlistmodel.h"
#include "../gpt4all-backend/llmodel.h"
#include "../gpt4all-backend/sysinfo.h"
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
    , m_threadCount(std::min(4, (int32_t) std::thread::hardware_concurrency()))
    , m_serverEnabled(false)
    , m_compatHardware(true)
{
    QString llmodelSearchPaths = QCoreApplication::applicationDirPath();
    const QString libDir = QCoreApplication::applicationDirPath() + "/../lib/";
    if (directoryExists(libDir))
        llmodelSearchPaths += ";" + libDir;
#if defined(Q_OS_MAC)
    const QString binDir = QCoreApplication::applicationDirPath() + "/../../../";
    if (directoryExists(binDir))
        llmodelSearchPaths += ";" + binDir;
    const QString frameworksDir = QCoreApplication::applicationDirPath() + "/../Frameworks/";
    if (directoryExists(frameworksDir))
        llmodelSearchPaths += ";" + frameworksDir;
#endif
    LLModel::setImplementationsSearchPath(llmodelSearchPaths.toStdString());
    connect(this, &LLM::serverEnabledChanged,
        ChatListModel::globalInstance(), &ChatListModel::handleServerEnabledChanged);

#if defined(__x86_64__)
    #ifndef _MSC_VER
        const bool minimal(__builtin_cpu_supports("avx"));
    #else
        int cpuInfo[4];
        __cpuid(cpuInfo, 1);
        const bool minimal(cpuInfo[2] & (1 << 28));
    #endif
#else
    const bool minimal = true; // Don't know how to handle non-x86_64
#endif

    m_compatHardware = minimal;
    emit compatHardwareChanged();
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

bool LLM::directoryExists(const QString &path) const
{
    const QUrl url(path);
    const QString localFilePath = url.isLocalFile() ? url.toLocalFile() : path;
    const QFileInfo info(localFilePath);
    return info.exists() && info.isDir();
}

bool LLM::fileExists(const QString &path) const
{
    const QUrl url(path);
    const QString localFilePath = url.isLocalFile() ? url.toLocalFile() : path;
    const QFileInfo info(localFilePath);
    return info.exists() && info.isFile();
}

qint64 LLM::systemTotalRAMInGB() const
{
    return getSystemTotalRAMInGB();
}

QString LLM::systemTotalRAMInGBString() const
{
    return QString::fromStdString(getSystemTotalRAMInGBString());
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

bool LLM::serverEnabled() const
{
    return m_serverEnabled;
}

void LLM::setServerEnabled(bool enabled)
{
    if (m_serverEnabled == enabled)
        return;
    m_serverEnabled = enabled;
    emit serverEnabledChanged();
}
