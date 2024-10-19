#include "Package.h"


using namespace Qt::Literals::StringLiterals;
using namespace gpt4all::manifest;


Package Package::parsePackage(QXmlStreamReader &xml)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == "pkg-manifest"_L1);
    Package p;
    while(xml.readNextStartElement()) {
        if(xml.name() == "installer-uri"_L1) {
            p.installer_endpoint = QUrl(xml.readElementText());
        }
        else if(xml.name() == "sha256"_L1) {
            p.checksum_sha256 = xml.readElementText();
        }
        else if(xml.name() == "args"_L1) {
            p.installer_args = xml.readElementText().split(" ");
        }
        else if(xml.name() == "signed"_L1) {
            p.is_signed = xml.readElementText() == "true";
        }
        else {
            xml.skipCurrentElement();
        }
    }
    return p;
}
