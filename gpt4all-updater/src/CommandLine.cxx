#include "CommandLine.h"

#include "utils.h"


gpt4all::command::CommandLine::CommandLine()
{
    this->parser = new QCommandLineParser();
    this->parser->addHelpOption();
    this->parser->addVersionOption();
    this->parser->addPositionalArgument("command", QCoreApplication::translate("main", "Gpt4All installation modification command, must be one of: 'update', 'downgrade', 'uninstall', 'modify' (note: only upgrade and uninstall are available offline)"));
}

gpt4all::command::CommandLine::~CommandLine()
{
    free(this->parser);
}

void gpt4all::command::CommandLine::parse(QCoreApplication &app)
{
    this->parser->process(app);
}

constexpr uint32_t hash(const char* data) noexcept{
    uint32_t hash = 5381;

    for(const char *c = data; *c; ++c)
        hash = ((hash << 5) + hash) + (unsigned char) *c;

    return hash;
}

gpt4all::command::CommandType gpt4all::command::CommandLine::command()
{
    const QStringList args(this->parser->positionalArguments());
    if (args.empty()){
        this->parser->showHelp(1);
    }
    gpt4all::command::CommandType ret;
    QString command_arg = args.first();
    switch(hash(command_arg.toStdString().c_str())) {
        case hash("update"):
            ret = gpt4all::command::CommandType::UPDATE;
            break;
#ifndef OFFLINE
        case hash("modify"):
            ret = gpt4all::command::CommandType::MODIFY;
            break;
        case hash("downgrade"):
            ret = gpt4all::command::CommandType::DOWNGRADE;
            break;
#endif
        case hash("uninstall"):
            ret = gpt4all::command::CommandType::UNINSTALL;
            break;
    }
    if(!ret)
        qWarning() << "Invalid command option";
        QCoreApplication::exit(1);
    return ret;
}
