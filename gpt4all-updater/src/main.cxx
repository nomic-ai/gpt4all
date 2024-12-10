#include "CommandLine.h"
#include "CommandFactory.h"


int main(int argc, char ** argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName("nomic.ai");
    QCoreApplication::setOrganizationDomain("gpt4all.io");
    QCoreApplication::setApplicationName("gpt4all-auto-updater");
    QCoreApplication::setApplicationVersion("0.0.1");
    gpt4all::command::CommandLine cli;
    cli.parse(app);
    CommandFactory cmd;
    gpt4all::command::CommandType command_type(cli.command());
    std::shared_ptr<gpt4all::command::Command> installer_command = cmd.GetCommand(command_type);
    if(installer_command)
        installer_command->execute();
    return 0;
}