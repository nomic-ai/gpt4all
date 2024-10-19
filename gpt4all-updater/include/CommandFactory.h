#pragma once

#include "Command.h"
#include "CommandLine.h"



class CommandFactory : public QObject
{
public:
    std::shared_ptr<gpt4all::command::Command> GetCommand(gpt4all::command::CommandType type);
};