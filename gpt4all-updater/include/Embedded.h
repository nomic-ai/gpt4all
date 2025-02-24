#pragma once

#include <iostream>
#include <fstream>
#include <sstream>

#include <QCoreApplication>
#include <QFile>
#include <QByteArray>

// Symbols indicating beginning and end of embedded installer
extern "C" {
    extern char data_start_gpt4all_installer, data_stop_gpt4all_installer;
    extern int gpt4all_installer_size;
}

namespace gpt4all {
namespace embedded {

class EmbeddedInstaller : public QObject
{
public:
    EmbeddedInstaller();
    void extractAndDecode();
    void installInstaller();

private:
    char * start;
    char * end;
    int size;
    QByteArray data;
};


}
}

