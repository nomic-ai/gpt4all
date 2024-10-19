#pragma once

#include "Command.h"


namespace gpt4all {
namespace modify {

class Modify : public gpt4all::command::Command
{
public:
    Modify(QFile &installer);
};
}
}