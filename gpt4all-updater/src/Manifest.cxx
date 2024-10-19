#include "Manifest.h"

using namespace gpt4all::manifest;
using namespace Qt::Literals::StringLiterals;

ManifestFile *ManifestFile::parseManifest(QIODevice *manifestInput)
{
    ManifestFile * manifest = new ManifestFile();
    manifest->xml.setDevice(manifestInput);
    if(manifest->xml.readNextStartElement()) {
        if (manifest->xml.name() == "gpt4all-updater"_L1){
            if(manifest->xml.readNextStartElement()) {
               manifest->parsePkgDescription();
            }
        }
    }
    return manifest;
}

void ManifestFile::parsePkgDescription()
{

    Q_ASSERT(xml.isStartElement() && xml.name() == "pkg-description"_L1);

    while(xml.readNextStartElement()){
        if(xml.name() == "name"_L1){
            this->name = xml.readElementText();
        }
        else if(xml.name() == "version"_L1){
            qsizetype suffix;
            this->release_ver = QVersionNumber::fromString(xml.readElementText(), &suffix);
        }
        else if(xml.name() == "notes"_L1) {
            this->notes = xml.readElementText();
        }
        else if(xml.name() == "authors"_L1) {
            this->authors = xml.readElementText().split(",");
        }
        else if(xml.name() == "date"_L1) {
            this->release_date = QDate::fromString(xml.readElementText(), Qt::ISODate);
        }
        else if(xml.name() == "release-type"_L1) {
            if ( xml.readElementText() == "release"_L1 ) {
                this->config = releaseType::RELEASE;
            }
            else {
                this->config = releaseType::DEBUG;
            }
        }
        else if(xml.name() == "last-compatible-version"_L1) {
            qsizetype suffix;
            this->last_supported_version = QVersionNumber::fromString(xml.readElementText(), &suffix);
        }
        else if(xml.name() == "entity"_L1) {
            this->entity = xml.readElementText();
        }
        else if(xml.name() == "component-list"_L1) {
            this->component_list = xml.readElementText().split(",");
        }
        else if(xml.name() == "pkg-manifest"_L1) {
            this->parsePkgManifest();
        }
        else {
            // Should probably throw here, but I'd rather implement schema validation
            xml.skipCurrentElement();
        }
    }
}

void ManifestFile::parsePkgManifest()
{
    this->pkg = Package::parsePackage(xml);
}

QUrl & ManifestFile::getInstallerEndpoint()
{
    return this->pkg.installer_endpoint;
}

QString & ManifestFile::getExpectedHash()
{
    return this->pkg.checksum_sha256;
}
