#pragma once

#include "Command.h"

namespace gpt4all {
namespace updater {

class Update : public gpt4all::command::Command
{
public:
    Update(QFile &installer);
};

}
}
