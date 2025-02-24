#pragma once

#include <QDir>
#include <QVersionNumber>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QCoreApplication>

namespace gpt4all {
namespace state {

class Gpt4AllState : public QObject {
public:
    virtual ~Gpt4AllState() = default;
    static Gpt4AllState & getInstance();
    QVersionNumber &getCurrentGpt4AllVersion();
    QFile &getCurrentGpt4AllConfig();
    QDir &getCurrentGpt4AllInstallRoot();
    bool checkForExistingInstall();
    QFile &getInstaller();
    void setInstaller(QFile *installer);
    void removeCurrentInstallation();
    bool getExistingInstaller();
#ifdef OFFLINE
    void driveOffline();
#endif

private:
    explicit Gpt4AllState(QObject *parent = nullptr) {}
    QFile *installer;
    QFile currentGpt4AllConfig;
    QDir currentGpt4AllInstallPath;
    QVersionNumber currentGpt4AllVersion;
};
}
}

