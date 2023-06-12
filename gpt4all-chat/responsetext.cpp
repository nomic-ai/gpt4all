#include "responsetext.h"

#include <QTextCharFormat>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QFontMetricsF>
#include <QTextTableCell>
#include <QGuiApplication>
#include <QClipboard>

enum Language {
    None,
    Python,
    Cpp,
    Bash
};

static QColor keywordColor      = "#2e95d3"; // blue
static QColor functionColor     = "#f22c3d"; // red
static QColor functionCallColor = "#e9950c"; // orange
static QColor commentColor      = "#808080"; // gray
static QColor stringColor       = "#00a37d"; // green
static QColor numberColor       = "#df3079"; // fuchsia
static QColor preprocessorColor = keywordColor;

static Language stringToLanguage(const QString &language)
{
    if (language == "python")
        return Python;
    if (language == "cpp")
        return Cpp;
    if (language == "c++")
        return Cpp;
    if (language == "c")
        return Cpp;
    if (language == "bash")
        return Bash;
    return None;
}

struct HighlightingRule {
    QRegularExpression pattern;
    QTextCharFormat format;
};

static QVector<HighlightingRule> pythonHighlightingRules()
{
    static QVector<HighlightingRule> highlightingRules;
    if (highlightingRules.isEmpty()) {

        HighlightingRule rule;

        QTextCharFormat functionCallFormat;
        functionCallFormat.setForeground(functionCallColor);
        rule.pattern = QRegularExpression("\\b(\\w+)\\s*(?=\\()");
        rule.format = functionCallFormat;
        highlightingRules.append(rule);

        QTextCharFormat functionFormat;
        functionFormat.setForeground(functionColor);
        rule.pattern = QRegularExpression("\\bdef\\s+(\\w+)\\b");
        rule.format = functionFormat;
        highlightingRules.append(rule);

        QTextCharFormat numberFormat;
        numberFormat.setForeground(numberColor);
        rule.pattern = QRegularExpression("\\b[0-9]*\\.?[0-9]+\\b");
        rule.format = numberFormat;
        highlightingRules.append(rule);

        QTextCharFormat keywordFormat;
        keywordFormat.setForeground(keywordColor);
        QStringList keywordPatterns = {
            "\\bdef\\b", "\\bclass\\b", "\\bif\\b", "\\belse\\b", "\\belif\\b",
            "\\bwhile\\b", "\\bfor\\b", "\\breturn\\b", "\\bprint\\b", "\\bimport\\b",
            "\\bfrom\\b", "\\bas\\b", "\\btry\\b", "\\bexcept\\b", "\\braise\\b",
            "\\bwith\\b", "\\bfinally\\b", "\\bcontinue\\b", "\\bbreak\\b", "\\bpass\\b"
        };

        for (const QString &pattern : keywordPatterns) {
            rule.pattern = QRegularExpression(pattern);
            rule.format = keywordFormat;
            highlightingRules.append(rule);
        }

        QTextCharFormat stringFormat;
        stringFormat.setForeground(stringColor);
        rule.pattern = QRegularExpression("\".*?\"");
        rule.format = stringFormat;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\'.*?\'");
        rule.format = stringFormat;
        highlightingRules.append(rule);

        QTextCharFormat commentFormat;
        commentFormat.setForeground(commentColor);
        rule.pattern = QRegularExpression("#[^\n]*");
        rule.format = commentFormat;
        highlightingRules.append(rule);

    }
    return highlightingRules;
}

static QVector<HighlightingRule> cppHighlightingRules()
{
    static QVector<HighlightingRule> highlightingRules;
    if (highlightingRules.isEmpty()) {

        HighlightingRule rule;

        QTextCharFormat functionCallFormat;
        functionCallFormat.setForeground(functionCallColor);
        rule.pattern = QRegularExpression("\\b(\\w+)\\s*(?=\\()");
        rule.format = functionCallFormat;
        highlightingRules.append(rule);

        QTextCharFormat functionFormat;
        functionFormat.setForeground(functionColor);
        rule.pattern = QRegularExpression("\\b[a-zA-Z_][a-zA-Z0-9_]*\\s+(\\w+)\\s*\\(");
        rule.format = functionFormat;
        highlightingRules.append(rule);

        QTextCharFormat numberFormat;
        numberFormat.setForeground(numberColor);
        rule.pattern = QRegularExpression("\\b[0-9]*\\.?[0-9]+\\b");
        rule.format = numberFormat;
        highlightingRules.append(rule);

        QTextCharFormat keywordFormat;
        keywordFormat.setForeground(keywordColor);
        QStringList keywordPatterns = {
            "\\bauto\\b", "\\bbool\\b", "\\bbreak\\b", "\\bcase\\b", "\\bcatch\\b",
            "\\bchar\\b", "\\bclass\\b", "\\bconst\\b", "\\bconstexpr\\b", "\\bcontinue\\b",
            "\\bdefault\\b", "\\bdelete\\b", "\\bdo\\b", "\\bdouble\\b", "\\belse\\b",
            "\\belifdef\\b", "\\belifndef\\b", "\\bembed\\b", "\\benum\\b", "\\bexplicit\\b",
            "\\bextern\\b", "\\bfalse\\b", "\\bfloat\\b", "\\bfor\\b", "\\bfriend\\b", "\\bgoto\\b",
            "\\bif\\b", "\\binline\\b", "\\bint\\b", "\\blong\\b", "\\bmutable\\b", "\\bnamespace\\b",
            "\\bnew\\b", "\\bnoexcept\\b", "\\bnullptr\\b", "\\boperator\\b", "\\boverride\\b",
            "\\bprivate\\b", "\\bprotected\\b", "\\bpublic\\b", "\\bregister\\b", "\\breinterpret_cast\\b",
            "\\breturn\\b", "\\bshort\\b", "\\bsigned\\b", "\\bsizeof\\b", "\\bstatic\\b", "\\bstatic_assert\\b",
            "\\bstatic_cast\\b", "\\bstruct\\b", "\\bswitch\\b", "\\btemplate\\b", "\\bthis\\b",
            "\\bthrow\\b", "\\btrue\\b", "\\btry\\b", "\\btypedef\\b", "\\btypeid\\b", "\\btypename\\b",
            "\\bunion\\b", "\\bunsigned\\b", "\\busing\\b", "\\bvirtual\\b", "\\bvoid\\b",
            "\\bvolatile\\b", "\\bwchar_t\\b", "\\bwhile\\b"
        };

        for (const QString &pattern : keywordPatterns) {
            rule.pattern = QRegularExpression(pattern);
            rule.format = keywordFormat;
            highlightingRules.append(rule);
        }

        QTextCharFormat stringFormat;
        stringFormat.setForeground(stringColor);
        rule.pattern = QRegularExpression("\".*?\"");
        rule.format = stringFormat;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\'.*?\'");
        rule.format = stringFormat;
        highlightingRules.append(rule);

        QTextCharFormat commentFormat;
        commentFormat.setForeground(commentColor);
        rule.pattern = QRegularExpression("//[^\n]*");
        rule.format = commentFormat;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("/\\*.*?\\*/");
        rule.format = commentFormat;
        highlightingRules.append(rule);

        QTextCharFormat preprocessorFormat;
        preprocessorFormat.setForeground(preprocessorColor);
        rule.pattern = QRegularExpression("#(?:include|define|undef|ifdef|ifndef|if|else|elif|endif|error|pragma)\\b.*");
        rule.format = preprocessorFormat;
        highlightingRules.append(rule);
    }
    return highlightingRules;
}

static QVector<HighlightingRule> bashHighlightingRules()
{
    static QVector<HighlightingRule> highlightingRules;
    if (highlightingRules.isEmpty()) {
        // FIXME
    }
    return highlightingRules;
}

SyntaxHighlighter::SyntaxHighlighter(QObject *parent)
    : QSyntaxHighlighter(parent)
{
}

SyntaxHighlighter::~SyntaxHighlighter()
{
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    QTextBlock block = this->currentBlock();

    QVector<HighlightingRule> rules;
    if (block.userState() == Python)
        rules = pythonHighlightingRules();
    else if (block.userState() == Cpp)
        rules = cppHighlightingRules();
    else if (block.userState() == Bash)
        rules = bashHighlightingRules();

    for (const HighlightingRule &rule : qAsConst(rules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            int startIndex = match.capturedStart();
            int length = match.capturedLength();
            setFormat(startIndex, length, rule.format);
        }
    }
}

ResponseText::ResponseText(QObject *parent)
    : QObject{parent}
    , m_textDocument(nullptr)
    , m_syntaxHighlighter(new SyntaxHighlighter(this))
    , m_isProcessingText(false)
{
}

QQuickTextDocument* ResponseText::textDocument() const
{
    return m_textDocument;
}

void ResponseText::setTextDocument(QQuickTextDocument* textDocument)
{
    if (m_textDocument)
        disconnect(m_textDocument->textDocument(), &QTextDocument::contentsChanged, this, &ResponseText::handleTextChanged);

    m_textDocument = textDocument;
    m_syntaxHighlighter->setDocument(m_textDocument->textDocument());
    connect(m_textDocument->textDocument(), &QTextDocument::contentsChanged, this, &ResponseText::handleTextChanged);
    handleTextChanged();
}

QString ResponseText::getLinkAtPosition(int position) const
{
    int i = 0;
    for (const auto &link : m_links) {
        if (position >= link.startPos && position < link.endPos)
            return link.href;
    }
    return QString();
}

bool ResponseText::tryCopyAtPosition(int position) const
{
    for (const auto &copy : m_copies) {
        if (position >= copy.startPos && position < copy.endPos) {
            QClipboard *clipboard = QGuiApplication::clipboard();
            clipboard->setText(copy.text);
            return true;
        }
    }
    return false;
}

void ResponseText::handleTextChanged()
{
    if (!m_textDocument || m_isProcessingText)
        return;

    m_isProcessingText = true;
    QTextDocument* doc = m_textDocument->textDocument();
    handleContextLinks();
    handleCodeBlocks();
    m_isProcessingText = false;
}

void ResponseText::handleContextLinks()
{
    QTextDocument* doc = m_textDocument->textDocument();
    QTextCursor cursor(doc);
    QTextCharFormat linkFormat;
    linkFormat.setForeground(m_linkColor);
    linkFormat.setFontUnderline(true);

    // Regex for context links
    static const QRegularExpression reLink("\\[Context\\]\\((context://\\d+)\\)");
    QRegularExpressionMatchIterator iLink = reLink.globalMatch(doc->toPlainText());

    QList<QRegularExpressionMatch> matchesLink;
    while (iLink.hasNext())
        matchesLink.append(iLink.next());

    QVector<ContextLink> newLinks;

    // Calculate new positions and store them in newLinks
    int positionOffset = 0;
    for(const auto &match : matchesLink) {
        ContextLink newLink;
        newLink.href = match.captured(1);
        newLink.text = "Context";
        newLink.startPos = match.capturedStart() - positionOffset;
        newLink.endPos = newLink.startPos + newLink.text.length();
        newLinks.append(newLink);
        positionOffset += match.capturedLength() - newLink.text.length();
    }

    // Replace the context links with the word "Context" in reverse order
    for(int index = matchesLink.count() - 1; index >= 0; --index) {
        cursor.setPosition(matchesLink.at(index).capturedStart());
        cursor.setPosition(matchesLink.at(index).capturedEnd(), QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        cursor.setCharFormat(linkFormat);
        cursor.insertText(newLinks.at(index).text);
        cursor.setCharFormat(QTextCharFormat());
    }

    m_links = newLinks;
}

void ResponseText::handleCodeBlocks()
{
    QTextDocument* doc = m_textDocument->textDocument();
    QTextCursor cursor(doc);

    QTextCharFormat textFormat;
    textFormat.setFontFamilies(QStringList() << "Monospace");
    textFormat.setForeground(QColor("white"));

    QTextFrameFormat frameFormatBase;
    frameFormatBase.setBackground(QColor("black"));

    QTextTableFormat tableFormat;
    tableFormat.setMargin(0);
    tableFormat.setPadding(0);
    tableFormat.setBorder(0);
    tableFormat.setBorderCollapse(true);
    QList<QTextLength> constraints;
    constraints << QTextLength(QTextLength::PercentageLength, 100);
    tableFormat.setColumnWidthConstraints(constraints);

    QTextTableFormat headerTableFormat;
    headerTableFormat.setBackground(m_headerColor);
    headerTableFormat.setPadding(0);
    headerTableFormat.setBorder(0);
    headerTableFormat.setBorderCollapse(true);
    headerTableFormat.setTopMargin(15);
    headerTableFormat.setBottomMargin(15);
    headerTableFormat.setLeftMargin(30);
    headerTableFormat.setRightMargin(30);
    QList<QTextLength> headerConstraints;
    headerConstraints << QTextLength(QTextLength::PercentageLength, 80);
    headerConstraints << QTextLength(QTextLength::PercentageLength, 20);
    headerTableFormat.setColumnWidthConstraints(headerConstraints);

    QTextTableFormat codeBlockTableFormat;
    codeBlockTableFormat.setBackground(QColor("black"));
    codeBlockTableFormat.setPadding(0);
    codeBlockTableFormat.setBorder(0);
    codeBlockTableFormat.setBorderCollapse(true);
    codeBlockTableFormat.setTopMargin(30);
    codeBlockTableFormat.setBottomMargin(30);
    codeBlockTableFormat.setLeftMargin(30);
    codeBlockTableFormat.setRightMargin(30);
    codeBlockTableFormat.setColumnWidthConstraints(constraints);

    QTextImageFormat copyImageFormat;
    copyImageFormat.setWidth(30);
    copyImageFormat.setHeight(30);
    copyImageFormat.setName("qrc:/gpt4all/icons/copy.svg");

    // Regex for code blocks
    static const QRegularExpression reCode("```(.*?)(```|$)", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator iCode = reCode.globalMatch(doc->toPlainText());

    static const QRegularExpression reWhitespace("^\\s*(\\w+)");

    QList<QRegularExpressionMatch> matchesCode;
    while (iCode.hasNext())
        matchesCode.append(iCode.next());

    QVector<CodeCopy> newCopies;

    for(int index = matchesCode.count() - 1; index >= 0; --index) {
        cursor.setPosition(matchesCode[index].capturedStart());
        cursor.setPosition(matchesCode[index].capturedEnd(), QTextCursor::KeepAnchor);
        cursor.removeSelectedText();

        QTextFrameFormat frameFormat = frameFormatBase;
        QString capturedText = matchesCode[index].captured(1);
        QString codeLanguage;

        QRegularExpressionMatch match = reWhitespace.match(capturedText);
        if (match.hasMatch()) {
            const QString firstWord = match.captured(1).trimmed();
            if (firstWord == "python"
                || firstWord == "cpp"
                || firstWord == "c++"
                || firstWord == "c"
                || firstWord == "bash") {
                codeLanguage = firstWord;
                capturedText.remove(0, match.captured(0).length());
            }
        }

        const QStringList lines = capturedText.split('\n');

        QTextFrame *mainFrame = cursor.currentFrame();
        cursor.setCharFormat(textFormat);

        QTextFrame *frame = cursor.insertFrame(frameFormat);
        QTextTable *table = cursor.insertTable(codeLanguage.isEmpty() ? 1 : 2, 1, tableFormat);

        if (!codeLanguage.isEmpty()) {
            QTextTableCell headerCell = table->cellAt(0, 0);
            QTextCursor headerCellCursor = headerCell.firstCursorPosition();
            QTextTable *headerTable = headerCellCursor.insertTable(1, 2, headerTableFormat);
            QTextTableCell header = headerTable->cellAt(0, 0);
            QTextCursor headerCursor = header.firstCursorPosition();
            headerCursor.insertText(codeLanguage);
            QTextTableCell copy = headerTable->cellAt(0, 1);
            QTextCursor copyCursor = copy.firstCursorPosition();
            int startPos = copyCursor.position();
            CodeCopy newCopy;
            newCopy.text = lines.join("\n");
            newCopy.startPos = copyCursor.position();
            newCopy.endPos = newCopy.startPos + 1;
            newCopies.append(newCopy);
            QTextBlockFormat blockFormat;
            blockFormat.setAlignment(Qt::AlignRight);
            copyCursor.setBlockFormat(blockFormat);
            copyCursor.insertImage(copyImageFormat, QTextFrameFormat::FloatRight);
        }

        QTextTableCell codeCell = table->cellAt(codeLanguage.isEmpty() ? 0 : 1, 0);
        QTextCursor codeCellCursor = codeCell.firstCursorPosition();
        QTextTable *codeTable = codeCellCursor.insertTable(1, 1, codeBlockTableFormat);
        QTextTableCell code = codeTable->cellAt(0, 0);
        QTextCursor codeCursor = code.firstCursorPosition();
        if (!codeLanguage.isEmpty()) {
            codeCursor.block().setUserState(stringToLanguage(codeLanguage));
            for (const QString &line : lines) {
                codeCursor.insertText(line);
                codeCursor.insertBlock();
                codeCursor.block().setUserState(stringToLanguage(codeLanguage));
            }
        } else {
            codeCursor.insertText(lines.join("\n"));
        }
        if (codeCursor.position() > 0) {
            codeCursor.setPosition(codeCursor.position() - 1);
            codeCursor.deleteChar();
        }

        cursor = mainFrame->lastCursorPosition();
        cursor.setCharFormat(QTextCharFormat());
    }

    m_copies = newCopies;
}
