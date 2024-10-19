#pragma once

#include <string>
#include <vector>

#include <QCryptographicHash>
#include <QXmlStreamReader>
#include <QString>
#include <QStringList>
#include <QUrl>

namespace gpt4all {
namespace manifest {

class Package {
public:
    Package() {}
    static Package parsePackage(QXmlStreamReader &xml);

    QString checksum_sha256;
    bool is_signed;
    QUrl installer_endpoint;
    QUrl sbom_manifest;
    QStringList installer_args;
};

}

}