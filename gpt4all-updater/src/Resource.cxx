#include "Resource.h"

using namespace gpt4all::resource


int WinInstallerResources::extractAndInstall(QFile *installerPath)
{
    HMODULE hModule = NULL;
    HRSRC resourceInfo = FindResourceA(hModule, "gpt4allInstaller", "CUSTOMDATA");
    if (!resourceInfo)
    {
        fprintf(stderr, "Could not find resource\n");
        return -1;
    }

    HGLOBAL resource = LoadResource(hModule, resourceInfo);
    if (!resource)
    {
        qWarning()  << "Could not load resource";
        return -1;
    }

    DWORD resourceSize = SizeofResource(hModule, resourceInfo);
    bool success = installerPath->open(QIODevice::WriteOnly | QIODevice::Append);
    qWarning() << "Opening installer file for writing:" << installerPath->fileName();
    if (!success) {
        const QString error
            = u"ERROR: Could not open temp file: %1 %2"_s.arg(installerPath->fileName());
        qWarning() << error;
        return nullptr;
    }
    const char* installerdat = (const char*)resource;
    installerPath.write(installerdat);
    installerPath.close();
}