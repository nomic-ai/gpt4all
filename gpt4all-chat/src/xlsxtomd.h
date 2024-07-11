#ifndef XLSXTOMD_H
#define XLSXTOMD_H

#include <QObject>
#include <QString>
#include <QtGlobal>

class XLSXToMD
{
public:
    static QString toMarkdown(const QString &xlsxFilePath);
};

#endif // XLSXTOMD_H
