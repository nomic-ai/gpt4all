#pragma once

#include "Command.h"


namespace gpt4all {
namespace downgrade {

class Downgrade : public gpt4all::command::Command
{
public:
    Downgrade(QFile &installer);
};


}
}