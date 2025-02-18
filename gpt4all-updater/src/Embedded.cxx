#include "Embedded.h"
#include "utils.h"

#include <QByteArray>
#include <QDir>

using namespace gpt4all::embedded;
using namespace Qt::Literals::StringLiterals;

EmbeddedInstaller::EmbeddedInstaller()
    : start(&data_start_gpt4all_installer),
      end(&data_stop_gpt4all_installer),
      size(gpt4all_installer_size) {}

void EmbeddedInstaller::extractAndDecode()
{
    QByteArray installerDat64(this->start, this->size);
    QByteArray installerDat = QByteArray::fromBase64(installerDat64);
    this->data = installerDat;
}

void EmbeddedInstaller::installInstaller()
{
    QString mountPoint = "/Volumes/gpt4all-installer-darwin";
    QString appBundlePath = QDir::tempPath() + "gpt4all-installer-darwin.app";
    QString installerStr = QDir::tempPath() + "gpt4all-installer.dmg";
    QFile installerPath(installerStr);
    bool success = installerPath.open(QIODevice::WriteOnly | QIODevice::Append);
    qWarning() << "Opening installer file for writing:" << installerPath.fileName();
    if (!success) {
        const QString error
            = u"ERROR: Could not open temp file: %1 %2"_s.arg(installerPath.fileName());
        qWarning() << error;
    }
    installerPath.write(this->data, this->size);
    installerPath.close();
    mountAndExtract(mountPoint, installerStr, appBundlePath);
}
