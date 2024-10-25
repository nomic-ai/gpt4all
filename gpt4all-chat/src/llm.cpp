#include "llm.h"

#include <gpt4all-backend/llmodel.h>
#include <gpt4all-backend/sysinfo.h>

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QGlobalStatic>
#include <QNetworkInformation>
#include <QProcess>
#include <QSettings>
#include <QUrl>
#include <QtLogging>

#ifdef GPT4ALL_OFFLINE_INSTALLER
#   include <QDesktopServices>
#else
#   include "network.h"
#endif

#ifdef Q_OS_MAC
#include "macosdock.h"
#endif

using namespace Qt::Literals::StringLiterals;

class MyLLM: public LLM { };
Q_GLOBAL_STATIC(MyLLM, llmInstance)
LLM *LLM::globalInstance()
{
    return llmInstance();
}

LLM::LLM()
    : QObject{nullptr}
    , m_compatHardware(LLModel::Implementation::hasSupportedCPU())
{
    QNetworkInformation::loadDefaultBackend();
    auto * netinfo = QNetworkInformation::instance();
    if (netinfo) {
        connect(netinfo, &QNetworkInformation::reachabilityChanged,
            this, &LLM::isNetworkOnlineChanged);
    }
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
#   pragma message(__FILE__ ": WARNING: offline installer build will not check for updates!")
    return QDesktopServices::openUrl(QUrl("https://github.com/nomic-ai/gpt4all/releases"));
#else
    Network::globalInstance()->trackEvent("check_for_updates");

#if defined(Q_OS_LINUX)
    QString tool = u"maintenancetool"_s;
#elif defined(Q_OS_WINDOWS)
    QString tool = u"maintenancetool.exe"_s;
#elif defined(Q_OS_DARWIN)
    QString tool = u"../../../maintenancetool.app/Contents/MacOS/maintenancetool"_s;
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
    auto * netinfo = QNetworkInformation::instance();
    return !netinfo || netinfo->reachability() == QNetworkInformation::Reachability::Online;
}

void LLM::showDockIcon() const
{
#ifdef Q_OS_MAC
    MacOSDock::showIcon();
#else
    qt_noop();
#endif
}

void LLM::hideDockIcon() const
{
#ifdef Q_OS_MAC
    MacOSDock::hideIcon();
#else
    qt_noop();
#endif
}
