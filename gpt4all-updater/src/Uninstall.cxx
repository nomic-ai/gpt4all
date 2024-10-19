#include "Uninstall.h"


using namespace gpt4all::uninstall;


Uninstall::Uninstall(QFile &uninstaller)
{
    this->installer = &uninstaller;
    this->arguments = {"-c", "pr"};
}