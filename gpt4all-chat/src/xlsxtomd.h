#ifndef XLSXTOMD_H
#define XLSXTOMD_H

class QIODevice;
class QString;

class XLSXToMD
{
public:
    static QString toMarkdown(QIODevice *xlsxDevice);
};

#endif // XLSXTOMD_H
