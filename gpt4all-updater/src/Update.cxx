#include "Update.h"


using namespace gpt4all::updater;


Update::Update(QFile &installer)
{
    this->installer = &installer;
    this->arguments = {"-c", "install"};
}