#include "State.h"

#if defined(Q_OS_WINDOWS)
    #include "Resource.h"
#elif defined(Q_OS_DARWIN)
    #include "Embeded.h"
#endif


using namespace gpt4all::state;

Gpt4AllState & Gpt4AllState::getInstance()
{
    static Gpt4AllState instance;
    return instance;
}

void Gpt4AllState::driveOffline()
{
    if(this->checkForExistingInstall())
        this->removeCurrentInstallation();
#if defined(Q_OS_WINDOWS)
    gpt4all::resource::WinInstallerResources::extractAndInstall();
#elif defined(Q_OS_DARWIN)
    gpt4all::embedded::EmbeddedInstaller embed;
    embed.extractAndDecode();
#endif
}

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

void Gpt4AllState::setInstaller(QFile &installer_)
{
    this->installer = &installer_;
}

void Gpt4AllState::removeCurrentInstallation() {
    if (!this->currentGpt4AllInstallPath.exists())
        return
    this->currentGpt4AllInstallPath.removeRecursively();
}

bool Gpt4AllState::checkForExistingInstall()
{
#if defined(Q_OS_DARWIN)
    QStringList potentialInstallPaths = {"/Applications/gpt4all"};
#elif defined(Q_OS_WINDOWS)
    QStringList potentialInstallPaths = {"C:\\Program Files\\gpt4all"};
#endif
    foreach( const QString &str, potentialInstallPaths) {
        QDir potentialPath(str);
        if(potentialPath.exists())
            this->currentGpt4AllInstallPath = potentialPath;
    }
}
