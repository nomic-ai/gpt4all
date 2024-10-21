#include "Embeded.h"

#include <QByteArray>

using namespace gpt4all::embedded;

EmbeddedInstaller::EmbeddedInstaller()
    : start(&data_start_gpt4all_installer),
      end(&data_stop_gpt4all_installer),
      size(gpt4all_installer_size) {}

EmbeddedInstaller::extractAndDecode()
{
    QByteArray installerDat64(this->start, this->size);
    QByteArray installerDat = QByteArray::fromBase64(installerDat64);
    this->data(installerDat);
}

EmbeddedInstaller::installInstaller(QFile *installerFilepath)
{
    bool success = installerPath->open(QIODevice::WriteOnly | QIODevice::Append);
    qWarning() << "Opening installer file for writing:" << installerPath->fileName();
    if (!success) {
        const QString error
            = u"ERROR: Could not open temp file: %1 %2"_s.arg(installerPath->fileName());
        qWarning() << error;
        return nullptr;
    }
    installerFilepath->write(this->data, this->size);
    installerFilePath.close();
    gpt4all::state::Gpt4AllState::getInstance().setInstaller(installerPath);
}
