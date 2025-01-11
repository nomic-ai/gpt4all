#pragma once
#include <QCoreApplication>
#include <QProcess>
#include <QStringList>
#include <QFile>

namespace gpt4all {
namespace command{

class Command
{
public:
    virtual bool execute();
    QStringList arguments;
    QFile* installer;
};
}
}