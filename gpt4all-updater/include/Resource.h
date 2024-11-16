#pragma once

#include <stdio.h>
#include <windows.h>

#include <QCoreApplication>
#include <QFile>

namespace gpt4all {
namespace resource {


class WinInstallerResources : public QObject
{
public:
    static int extractAndInstall(QFile *installerPath);
};
}
}