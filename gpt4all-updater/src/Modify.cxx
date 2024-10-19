#include "Modify.h"

using namespace gpt4all::modify;

Modify::Modify(QFile &installer)
{
    this->installer = &installer;
    this->arguments = {"-c", "ch"};
}