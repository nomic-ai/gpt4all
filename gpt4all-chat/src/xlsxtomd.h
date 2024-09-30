#ifndef XLSXTOMD_H
#define XLSXTOMD_H

class QString;

class XLSXToMD
{
public:
    static QString toMarkdown(const QString &xlsxFilePath);
};

#endif // XLSXTOMD_H
