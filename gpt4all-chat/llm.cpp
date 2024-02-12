#include "llm.h"
#include "../gpt4all-backend/sysinfo.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QResource>
#include <QSettings>
#include <QUrl>
#include <QNetworkInformation>
#include <fstream>

#ifndef GPT4ALL_OFFLINE_INSTALLER
#include "network.h"
#endif

class MyLLM: public LLM { };
Q_GLOBAL_STATIC(MyLLM, llmInstance)
LLM *LLM::globalInstance()
{
    return llmInstance();
}

LLM::LLM()
    : QObject{nullptr}
    , m_compatHardware(true)
{
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

    QNetworkInformation::loadDefaultBackend();
    connect(QNetworkInformation::instance(), &QNetworkInformation::reachabilityChanged,
        this, &LLM::isNetworkOnlineChanged);
}

bool LLM::hasSettingsAccess() const
{
    QSettings settings;
    settings.sync();
    return settings.status() == QSettings::NoError;
}

bool LLM::checkForUpdates() const
{
    #ifdef GPT4ALL_OFFLINE_INSTALLER
    #pragma message "offline installer build will not check for updates!"
    return QDesktopServices::openUrl(QUrl("https://gpt4all.io/"));
    #else
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
    #endif
}

bool LLM::directoryExists(const QString &path)
{
    const QUrl url(path);
    const QString localFilePath = url.isLocalFile() ? url.toLocalFile() : path;
    const QFileInfo info(localFilePath);
    return info.exists() && info.isDir();
}

bool LLM::fileExists(const QString &path)
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

bool LLM::isNetworkOnline() const
{
    if (!QNetworkInformation::instance())
        return false;

    return QNetworkInformation::instance()->reachability() == QNetworkInformation::Reachability::Online;
}
