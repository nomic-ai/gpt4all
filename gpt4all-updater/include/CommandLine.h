#pragma once

#include <QCommandLineParser>


namespace gpt4all {
namespace command {
    
enum CommandType {
    UPDATE,
    MODIFY,
    DOWNGRADE,
    UNINSTALL
};

class CommandLine : public QObject
{
public:
    CommandLine();
    ~CommandLine();
    void parse(QCoreApplication &app);
    CommandType command();
private:
    CommandType type;
    QCommandLineParser * parser;
};

}
}

