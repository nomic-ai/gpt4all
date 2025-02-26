#include "Command.h"

using namespace gpt4all::command;

bool Command::execute() {
    if (!this->installer->exists()) {
        qDebug() << "Couldn't find the installer, there may have been an issue extracting or downloading the installer";
        return false;
    }
    return QProcess::startDetached(this->installer->fileName(), this->arguments);
}
