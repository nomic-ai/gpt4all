#pragma once

#include "Package.h"

#include <QVersionNumber>
#include <QDate>
#include <QFile>
#include <QXmlStreamReader>
#include <QString>
#include <QStringList>

namespace gpt4all{
namespace manifest {

enum releaseType{
    RELEASE,
    DEBUG
};

class ManifestFile {
public:
    static ManifestFile * parseManifest(QIODevice *manifestInput);
    QUrl & getInstallerEndpoint();
    QString & getExpectedHash();
private:
    ManifestFile() {}
    QString name;
    QVersionNumber release_ver;
    QString notes;
    QStringList authors;
    QDate release_date;
    releaseType config;
    QVersionNumber last_supported_version;
    QString entity;
    QStringList component_list;
    Package pkg;
    void parsePkgDescription();
    void parsePkgManifest();
    QXmlStreamReader xml;
};




}
}