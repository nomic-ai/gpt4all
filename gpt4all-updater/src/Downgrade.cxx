#include "Downgrade.h"

using namespace gpt4all::downgrade;

Downgrade::Downgrade(QFile &installer)
{
    this->installer = &installer;
    this->arguments = {"-c", "rm"};
}
