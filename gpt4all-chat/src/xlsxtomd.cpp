#include "xlsxtomd.h"

#include <xlsxabstractsheet.h>
#include <xlsxcell.h>
#include <xlsxcellrange.h>
#include <xlsxdocument.h>
#include <xlsxformat.h>
#include <xlsxworksheet.h>

#include <QDateTime>
#include <QDebug>
#include <QList>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QStringView>
#include <QVariant>
#include <QtGlobal>
#include <QtLogging>

#include <memory>

using namespace Qt::Literals::StringLiterals;


static QString formatCellText(const QXlsx::Cell *cell)
{
    if (!cell) return QString();

    QVariant value = cell->value();
    QXlsx::Format format = cell->format();
    QString cellText;

    // Determine the cell type based on format
    if (cell->isDateTime()) {
        // Handle DateTime
        QDateTime dateTime = cell->dateTime().toDateTime();
        cellText = dateTime.isValid() ? dateTime.toString(QStringView(u"yyyy-MM-dd")) : value.toString();
    } else {
        cellText = value.toString();
    }

    if (cellText.isEmpty())
        return QString();

    // Escape special characters
    static QRegularExpression special(
        QStringLiteral(
            R"(()([\\`*_[\]<>()!|])|)"    // special characters
            R"(^(\s*)(#+(?:\s|$))|)"      // headings
            R"(^(\s*[0-9])(\.(?:\s|$))|)" // ordered lists ("1. a")
            R"(^(\s*)([+-](?:\s|$)))"     // unordered lists ("- a")
        ),
        QRegularExpression::MultilineOption
    );
    cellText.replace(special, uR"(\1\\2)"_s);
    cellText.replace(u'&', "&amp;"_L1);
    cellText.replace(u'<', "&lt;"_L1);
    cellText.replace(u'>', "&gt;"_L1);

    // Apply Markdown formatting based on font styles
    if (format.fontUnderline())
        cellText = u"_%1_"_s.arg(cellText);
    if (format.fontBold())
        cellText = u"**%1**"_s.arg(cellText);
    if (format.fontItalic())
        cellText = u"*%1*"_s.arg(cellText);
    if (format.fontStrikeOut())
        cellText = u"~~%1~~"_s.arg(cellText);

    return cellText;
}

static QString getCellValue(QXlsx::Worksheet *sheet, int row, int col)
{
    if (!sheet)
        return QString();

    // Attempt to retrieve the cell directly
    std::shared_ptr<QXlsx::Cell> cell = sheet->cellAt(row, col);

    // If the cell is part of a merged range and not directly available
    if (!cell) {
        for (const QXlsx::CellRange &range : sheet->mergedCells()) {
            if (row >= range.firstRow() && row <= range.lastRow() &&
                col >= range.firstColumn() && col <= range.lastColumn()) {
                cell = sheet->cellAt(range.firstRow(), range.firstColumn());
                break;
            }
        }
    }

    // Format and return the cell text if available
    if (cell)
        return formatCellText(cell.get());

    // Return empty string if cell is not found
    return QString();
}

QString XLSXToMD::toMarkdown(QIODevice *xlsxDevice)
{
    // Load the Excel document
    QXlsx::Document xlsx(xlsxDevice);
    if (!xlsx.load()) {
        qCritical() << "Failed to load the Excel from device";
        return QString();
    }

    QString markdown;

    // Retrieve all sheet names
    QStringList sheetNames = xlsx.sheetNames();
    if (sheetNames.isEmpty()) {
        qWarning() << "No sheets found in the Excel document.";
        return QString();
    }

    // Iterate through each worksheet by name
    for (const QString &sheetName : sheetNames) {
        QXlsx::Worksheet *sheet = dynamic_cast<QXlsx::Worksheet *>(xlsx.sheet(sheetName));
        if (!sheet) {
            qWarning() << "Failed to load sheet:" << sheetName;
            continue;
        }

        markdown += u"### %1\n\n"_s.arg(sheetName);

        // Determine the used range
        QXlsx::CellRange range = sheet->dimension();
        int firstRow = range.firstRow();
        int lastRow = range.lastRow();
        int firstCol = range.firstColumn();
        int lastCol = range.lastColumn();

        if (firstRow > lastRow || firstCol > lastCol) {
            qWarning() << "Sheet" << sheetName << "is empty.";
            markdown += QStringView(u"*No data available.*\n\n");
            continue;
        }

        auto appendRow = [&markdown](auto &list) { markdown += u"|%1|\n"_s.arg(list.join(u'|')); };

        // Empty header
        static QString header(u' ');
        static QString separator(u'-');
        QStringList headers;
        QStringList separators;
        for (int col = firstCol; col <= lastCol; ++col) {
            headers << header;
            separators << separator;
        }
        appendRow(headers);
        appendRow(separators);

        // Iterate through data rows
        for (int row = firstRow; row <= lastRow; ++row) {
            QStringList rowData;
            for (int col = firstCol; col <= lastCol; ++col) {
                QString cellText = getCellValue(sheet, row, col);
                rowData << (cellText.isEmpty() ? u" "_s : cellText);
            }
            appendRow(rowData);
        }

        markdown += u'\n'; // Add an empty line between sheets
    }
    return markdown;
}
