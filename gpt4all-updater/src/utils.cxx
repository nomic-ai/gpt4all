#include "utils.h"

#include <QProcess>
#include <QDebug>

constexpr uint32_t hash(const char* data) noexcept
{
    uint32_t hash = 5381;

    for(const char *c = data; *c; ++c)
        hash = ((hash << 5) + hash) + (unsigned char) *c;

    return hash;
}

void mountAndExtract(const QString &mountPoint, const QString &saveFilePath, const QString &appBundlePath)
{
    // Mount the DMG
    QProcess mountProcess;
    mountProcess.start("hdiutil", QStringList() << "attach" << "-mountpoint" << mountPoint << saveFilePath);
    mountProcess.waitForFinished();

    if (mountProcess.exitCode() != 0) {
        qDebug() << "Error mounting DMG:" << mountProcess.readAllStandardError();
    }

    // Extract files
    QProcess extractProcess;
    extractProcess.start("cp", QStringList() << "-R" << mountPoint + "/gpt4all-installer-darwin.app" << appBundlePath);
    extractProcess.waitForFinished();

    if (extractProcess.exitCode() != 0) {
        qDebug() << "Error extracting files:" << extractProcess.readAllStandardError();
    }

    // Unmount the DMG
    QProcess unmountProcess;
    unmountProcess.start("hdiutil", QStringList() << "detach" << mountPoint);
    unmountProcess.waitForFinished();

    if (unmountProcess.exitCode() != 0) {
        qDebug() << "Error unmounting DMG:" << unmountProcess.readAllStandardError();
    }

    qDebug() << "DMG mounted, extracted, and unmounted successfully.";
}