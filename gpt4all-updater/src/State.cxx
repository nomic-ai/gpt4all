#include "State.h"

#if defined(Q_OS_WINDOWS)
    #include "Resource.h"
#elif defined(Q_OS_DARWIN)
    #include "Embedded.h"
#endif


using namespace gpt4all::state;

Gpt4AllState & Gpt4AllState::getInstance()
{
    static Gpt4AllState instance;
    return instance;
}

#ifdef OFFLINE
void Gpt4AllState::driveOffline()
{
    // if(this->checkForExistingInstall())
    //     this->removeCurrentInstallation();
#if defined(Q_OS_WINDOWS)
    this->installer = new QString(QDir::tempPath() + "gpt4all-installer.exe");
    gpt4all::resource::WinInstallerResources::extractAndInstall(installer);
#elif defined(Q_OS_DARWIN)
    QString installer(QDir::tempPath() + "gpt4all-installer.dmg");
    QString appBundlePath = QDir::tempPath() + "gpt4all-installer-darwin.app";
    this->installer = new QFile(appBundlePath + "/Contents/MacOS/gpt4all-installer-darwin");
    gpt4all::embedded::EmbeddedInstaller embed;
    embed.extractAndDecode();
    embed.installInstaller();
#endif
}
#endif

QVersionNumber &Gpt4AllState::getCurrentGpt4AllVersion()
{
    return this->currentGpt4AllVersion;
}

QFile &Gpt4AllState::getCurrentGpt4AllConfig()
{
    return this->currentGpt4AllConfig;
}

QDir &Gpt4AllState::getCurrentGpt4AllInstallRoot()
{
    return this->currentGpt4AllInstallPath;
}

QFile & Gpt4AllState::getInstaller()
{
    return *this->installer;
}

void Gpt4AllState::setInstaller(QFile *installer)
{
    this->installer = installer;
}

void Gpt4AllState::removeCurrentInstallation() {
    if (this->currentGpt4AllInstallPath.exists())
        this->currentGpt4AllInstallPath.removeRecursively();
}

bool Gpt4AllState::checkForExistingInstall()
{
#if defined(Q_OS_DARWIN)
    QDir potentialInstallDir = QDir(QCoreApplication::applicationDirPath())
                                    .filePath("maintenancetool.app/Contents/MacOS/maintenancetool");
    QStringList potentialInstallPaths = {"/Applications/gpt4all"};
#elif defined(Q_OS_WINDOWS)
    QDir potentialInstallDir = QDir(QCoreApplication::applicationDirPath()).filePath("maintenancetool.exe");
    QStringList potentialInstallPaths = {QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)
                                        + "\\gpt4all", QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                                        + "\\gpt4all"};
#endif
    if(potentialInstallDir.exists()){
        this->currentGpt4AllInstallPath = QDir(QCoreApplication::applicationDirPath());
        return true;
    }
    foreach( const QString &str, potentialInstallPaths) {
        QDir potentialPath(str);
        if(potentialPath.exists())
            this->currentGpt4AllInstallPath = potentialPath;
            return true;
    }
    return false;
}


bool Gpt4AllState::getExistingInstaller()
{
    if(this->currentGpt4AllInstallPath.exists()){
#if defined(Q_OS_DARWIN)
        this->installer = new QFile(this->currentGpt4AllInstallPath.filePath("maintenancetool.app/Contents/MacOS/maintenancetool"));
#elif defined(Q_OS_WINDOWS)
        this->installer = new QFile(this->currentGpt4AllInstallPath.filePath("maintenancetool.exe"));
#endif
        return true;
    }
    return false;
}

