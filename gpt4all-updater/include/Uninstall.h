#pragma once

#include "Command.h"


namespace gpt4all {
namespace uninstall {

class Uninstall : public gpt4all::command::Command
{
public:
    Uninstall(QFile &uninstaller);
};

}
}