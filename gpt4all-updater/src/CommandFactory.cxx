#include "CommandFactory.h"
#include "Downgrade.h"
#include "Modify.h"
#include "Uninstall.h"
#include "Update.h"
#include "State.h"
#include "Download.h"

using namespace gpt4all::command;

std::shared_ptr<Command> CommandFactory::GetCommand(CommandType type)
{
    std::shared_ptr<Command> active_command;
#ifdef OFFLINE
    // If we're offline, we're only installing or removing
    gpt4all::state::Gpt4AllState::getInstance().driveOffline();
    // This command always removes the current install
    // only return a command if updating
    if (type == CommandType::UPDATE)
    {
        active_command = std::make_shared<gpt4all::updater::Update>(gpt4all::state::Gpt4AllState::getInstance().getInstaller());
    }
    else {
        active_command = std::shared_ptr<gpt4all::command::Command>(nullptr);
    }
#else
    if (type == CommandType::UPDATE || type == CommandType::DOWNGRADE) {
        gpt4all::download::Download::instance()->driveFetchAndInstall();
    }
    else {
        if(!gpt4all::state::Gpt4AllState::getInstance().getExistingInstaller()){
            qWarning() << "Unable to execute requested command, requires existing installation of gpt4all";
            QCoreApplication.exit(1);
        }
    }
    QFile &installer = gpt4all::state::Gpt4AllState::getInstance().getInstaller();
    switch(type){
        case CommandType::UPDATE:
            active_command = std::make_shared<gpt4all::updater::Update>(installer);
        case CommandType::DOWNGRADE:
            active_command = std::make_shared<gpt4all::downgrade::Downgrade>(installer);
        case CommandType::MODIFY:
            active_command = std::make_shared<gpt4all::modify::Modify>(installer);
        case CommandType::UNINSTALL:
            active_command = std::make_shared<gpt4all::uninstall::Uninstall>(installer);
    }
#endif
    return active_command;
}
