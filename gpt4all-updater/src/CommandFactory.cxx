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
    gpt4all::stage::Gpt4AllState::getInstance().driveOffline();
#else
    gpt4all::download::Download::instance()->driveFetchAndInstall();
#endif
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
    return active_command;
}
