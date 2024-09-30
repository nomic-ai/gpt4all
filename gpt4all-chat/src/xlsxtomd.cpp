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
#include <QString>
#include <QStringList>
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
    if (format.isDateTimeFormat()) {
        // Handle DateTime
        QDateTime dateTime = value.toDateTime();
        cellText = dateTime.isValid() ? dateTime.toString("yyyy-MM-dd") : value.toString();
    } else {
        cellText = value.toString();
    }

    if (cellText.isEmpty())
        return u" "_s;

    // Apply Markdown and HTML formatting based on font styles
    QString formattedText = cellText;

    if (format.fontUnderline())
        formattedText = u"_%1_"_s.arg(formattedText);
    if (format.fontBold())
        formattedText = u"**%1**"_s.arg(formattedText);
    if (format.fontItalic())
        formattedText = u"*%1*"_s.arg(formattedText);
    if (format.fontStrikeOut())
        formattedText = u"~~%1~~"_s.arg(formattedText);

    // Escape pipe characters to prevent Markdown table issues
    formattedText.replace("|", "\\|");

    return formattedText;
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

        markdown += u"## %1\n\n"_s.arg(sheetName);

        // Determine the used range
        QXlsx::CellRange range = sheet->dimension();
        int firstRow = range.firstRow();
        int lastRow = range.lastRow();
        int firstCol = range.firstColumn();
        int lastCol = range.lastColumn();

        if (firstRow > lastRow || firstCol > lastCol) {
            qWarning() << "Sheet" << sheetName << "is empty.";
            markdown += "*No data available.*\n\n";
            continue;
        }

        // Separator row (no header)
        QStringList separators;
        for (int col = firstCol; col <= lastCol; ++col)
            separators << u"---"_s;
        markdown += u"|%1|\n"_s.arg(separators.join(u'|'));

        // Iterate through data rows
        for (int row = 0; row <= lastRow; ++row) {
            QStringList rowData;
            for (int col = firstCol; col <= lastCol; ++col) {
                QString cellText = getCellValue(sheet, row, col);
                rowData << cellText;
            }

            QString dataRow = "|" + rowData.join("|") + "|";
            markdown += dataRow + "\n";
        }

        markdown += "\n"; // Add an empty line between sheets
    }
    return markdown;
}
