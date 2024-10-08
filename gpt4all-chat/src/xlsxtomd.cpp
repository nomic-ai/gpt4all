#include "xlsxtomd.h"

#include <xlsxabstractsheet.h>
#include <xlsxcell.h>
#include <xlsxcellrange.h>
#include <xlsxdocument.h>
#include <xlsxformat.h>
#include <xlsxworksheet.h>

#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QList>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QStringView>
#include <QTextStream>
#include <QVariant>
#include <QtGlobal>
#include <QtLogging>

#include <memory>

using namespace Qt::Literals::StringLiterals;

static QString escapeMarkdown(const QString &text)
{
    QString escaped = text;
    static QRegularExpression specialChars(
        QStringLiteral(R"([\\`*_{}[\]()#+\-.!])")
    );
    escaped.replace(specialChars, uR"(\\\0)"_s);
    return escaped;
}

static QString applyMarkdownFormatting(const QString &text, const QXlsx::Format &format)
{
    QString formattedText = text;

    // Apply strike-through first
    if (format.fontStrikeOut())
        formattedText = u"~~%1~~"_s.arg(formattedText);

    // Bold and italic can be nested
    bool isBold = format.fontBold();
    bool isItalic = format.fontItalic();
    if (isBold && isItalic) {
        formattedText = u"***%1***"_s.arg(formattedText);
    } else if (isBold) {
        formattedText = u"**%1**"_s.arg(formattedText);
    } else if (isItalic) {
        formattedText = u"*%1*"_s.arg(formattedText);
    }

    // Underline using HTML tags as Markdown does not support underline
    if (format.fontUnderline())
        formattedText = u"<u>%1</u>"_s.arg(formattedText);

    return formattedText;
}

static QString formatCellText(const QXlsx::Cell *cell)
{
    if (!cell) return QString();

    QVariant value = cell->value();
    QXlsx::Format format = cell->format();
    QString cellText;

    // Handle different data types
    if (cell->isDateTime()) {
        // Handle DateTime
        QDateTime dateTime = cell->dateTime().toDateTime();
        cellText = dateTime.isValid() ? dateTime.toString(u"yyyy-MM-dd") : value.toString();
    } else if (value.type() == QVariant::Int || value.type() == QVariant::Double || value.type() == QVariant::LongLong) {
        cellText = value.toString();
    } else if (value.type() == QVariant::String) {
        cellText = value.toString();
    } else {
        // Handle non-standard cell types
        cellText = value.isValid() ? value.toString() : QString();
    }

    if (cellText.isEmpty())
        return QString();

    // Handle hyperlinks
    if (cell->hyperlink().isValid()) {
        QString url = cell->hyperlink().target;
        cellText = u"[%1](%2)"_s.arg(cellText, url);
    }

    // Escape special Markdown characters
    cellText = escapeMarkdown(cellText);

    // Apply Markdown formatting based on font styles
    cellText = applyMarkdownFormatting(cellText, format);

    return cellText;
}

static QMap<QString, const QXlsx::Cell *> buildMergedCellsMapping(QXlsx::Worksheet *sheet)
{
    QMap<QString, const QXlsx::Cell *> mergedCellsMapping;

    for (const QXlsx::CellRange &range : sheet->mergedCells()) {
        const QXlsx::Cell *topLeftCell = sheet->cellAt(range.firstRow(), range.firstColumn());
        if (!topLeftCell) continue;
        for (int row = range.firstRow(); row <= range.lastRow(); ++row) {
            for (int col = range.firstColumn(); col <= range.lastColumn(); ++col) {
                QString coord = QXlsx::CellReference(row, col).toString();
                mergedCellsMapping[coord] = topLeftCell;
            }
        }
    }

    return mergedCellsMapping;
}

static QString getCellValue(QXlsx::Worksheet *sheet, int row, int col, const QMap<QString, const QXlsx::Cell *> &mergedCellsMapping)
{
    if (!sheet)
        return QString();

    QString coord = QXlsx::CellReference(row, col).toString();
    const QXlsx::Cell *cell = sheet->cellAt(row, col);

    // Check if the cell is part of a merged range
    if (!cell && mergedCellsMapping.contains(coord)) {
        cell = mergedCellsMapping.value(coord);
    }

    // Format and return the cell text if available
    if (cell)
        return formatCellText(cell);

    // Return empty string if cell is not found
    return QString();
}

static void processHeaders(QXlsx::Worksheet *sheet, int headerRow, int firstCol, int lastCol, const QMap<QString, const QXlsx::Cell *> &mergedCellsMapping, QStringList &headers, QStringList &alignments)
{
    for (int col = firstCol; col <= lastCol; ++col) {
        QString cellText = getCellValue(sheet, headerRow, col, mergedCellsMapping);
        if (cellText.isEmpty())
            cellText = u"Column %1"_s.arg(QXlsx::CellReference(1, col).columnName());

        headers << cellText;

        // Determine alignment
        const QXlsx::Cell *cell = sheet->cellAt(headerRow, col);
        if (!cell && mergedCellsMapping.contains(QXlsx::CellReference(headerRow, col).toString())) {
            cell = mergedCellsMapping.value(QXlsx::CellReference(headerRow, col).toString());
        }
        QString alignment = u"---"; // Default alignment is left
        if (cell) {
            QXlsx::Format format = cell->format();
            switch (format.horizontalAlignment()) {
            case QXlsx::Format::AlignHCenter:
            case QXlsx::Format::AlignHJustify:
                alignment = u":---:";
                break;
            case QXlsx::Format::AlignRight:
                alignment = u"---:";
                break;
            case QXlsx::Format::AlignLeft:
            default:
                alignment = u"---";
                break;
            }
        }
        alignments << alignment;
    }
}

QString XLSXToMD::toMarkdown(QIODevice *xlsxDevice, bool skipEmptySheets)
{
    // Validate device
    if (!xlsxDevice || !xlsxDevice->isOpen()) {
        qCritical() << "Invalid or unopened QIODevice provided.";
        return QString();
    }

    // Load the Excel document
    QXlsx::Document xlsx(xlsxDevice);
    if (!xlsx.load()) {
        qCritical() << "Failed to load the Excel document from device.";
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

        // Determine the used range
        QXlsx::CellRange range = sheet->dimension();
        int firstRow = range.firstRow();
        int lastRow = range.lastRow();
        int firstCol = range.firstColumn();
        int lastCol = range.lastColumn();

        if (firstRow > lastRow || firstCol > lastCol) {
            if (skipEmptySheets) {
                qInfo() << "Skipping empty sheet:" << sheetName;
                continue;
            } else {
                markdown += u"### %1\n\n"_s.arg(sheetName);
                markdown += u"*No data available.*\n\n";
                continue;
            }
        }

        markdown += u"### %1\n\n"_s.arg(sheetName);

        // Build merged cells mapping
        QMap<QString, const QXlsx::Cell *> mergedCellsMapping = buildMergedCellsMapping(sheet);

        // Process headers and alignments
        QStringList headers;
        QStringList alignments;
        processHeaders(sheet, firstRow, firstCol, lastCol, mergedCellsMapping, headers, alignments);

        auto appendRow = [&markdown](const QStringList &list) { markdown += u"|%1|\n"_s.arg(list.join(u'|')); };

        // Append headers and alignments
        appendRow(headers);
        appendRow(alignments);

        // Iterate through data rows
        for (int row = firstRow + 1; row <= lastRow; ++row) {
            QStringList rowData;
            bool isEmptyRow = true;
            for (int col = firstCol; col <= lastCol; ++col) {
                QString cellText = getCellValue(sheet, row, col, mergedCellsMapping);
                if (!cellText.trimmed().isEmpty())
                    isEmptyRow = false;
                rowData << (cellText.isEmpty() ? u" "_s : cellText);
            }
            // Skip empty rows
            if (!isEmptyRow)
                appendRow(rowData);
        }

        markdown += u'\n'; // Add an empty line between sheets
    }

    return markdown;
}

// Example usage of the modified function
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    if (argc < 2) {
        qCritical() << "Usage:" << argv[0] << "<input.xlsx> [output.md]";
        return -1;
    }

    QString inputFileName = argv[1];
    QString outputFileName = (argc > 2) ? argv[2] : inputFileName.section('.', 0, -2) + ".md";

    // Validate input file
    if (!inputFileName.endsWith(u".xlsx"_s, Qt::CaseInsensitive) && !inputFileName.endsWith(u".xlsm"_s, Qt::CaseInsensitive)) {
        qCritical() << "Input file must be an Excel file with .xlsx or .xlsm extension.";
        return -1;
    }

    QFile xlsxFile(inputFileName);
    if (!xlsxFile.open(QIODevice::ReadOnly)) {
        qCritical() << "Failed to open input file:" << inputFileName;
        return -1;
    }

    XLSXToMD converter;
    QString markdownContent = converter.toMarkdown(&xlsxFile, true); // skipEmptySheets = true
    xlsxFile.close();

    if (markdownContent.isEmpty()) {
        qCritical() << "No markdown content generated.";
        return -1;
    }

    // Write output with UTF-8 encoding
    QFile mdFile(outputFileName);
    if (!mdFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical() << "Failed to open output file:" << outputFileName;
        return -1;
    }

    QTextStream outStream(&mdFile);
    outStream.setCodec("UTF-8");
    outStream << markdownContent;
    mdFile.close();

    qInfo() << "Conversion complete. Check" << outputFileName;

    return 0;
}
