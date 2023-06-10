#include "responsetext.h"

#include <QTextCharFormat>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QFontMetricsF>
#include <QTextTableCell>

SyntaxHighlighter::SyntaxHighlighter(QObject *parent)
    : QSyntaxHighlighter(parent)
{
}

SyntaxHighlighter::~SyntaxHighlighter()
{
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    for (const HighlightingRule &rule : qAsConst(m_highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
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

void ResponseText::handleTextChanged()
{
    if (!m_textDocument || m_isProcessingText)
        return;

    m_isProcessingText = true;
    QTextDocument* doc = m_textDocument->textDocument();
    QTextCursor cursor(doc);

    QTextCharFormat linkFormat;
    linkFormat.setForeground(m_linkColor);
    linkFormat.setFontUnderline(true);

    // Loop through the document looking for context links
    QRegularExpression re("\\[Context\\]\\((context://\\d+)\\)");
    QRegularExpressionMatchIterator i = re.globalMatch(doc->toPlainText());

    QList<QRegularExpressionMatch> matches;
    while (i.hasNext())
        matches.append(i.next());

    QVector<ContextLink> newLinks;

    // Calculate new positions and store them in newLinks
    int positionOffset = 0;
    for(const auto &match : matches) {
        ContextLink newLink;
        newLink.href = match.captured(1);
        newLink.text = "Context";
        newLink.startPos = match.capturedStart() - positionOffset;
        newLink.endPos = newLink.startPos + newLink.text.length();
        newLinks.append(newLink);
        positionOffset += match.capturedLength() - newLink.text.length();
    }

    // Replace the context links with the word "Context" in reverse order
    for(int index = matches.count() - 1; index >= 0; --index) {
        cursor.setPosition(matches.at(index).capturedStart());
        cursor.setPosition(matches.at(index).capturedEnd(), QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        cursor.setCharFormat(linkFormat);
        cursor.insertText(newLinks.at(index).text);
        cursor.setCharFormat(QTextCharFormat());
    }

    m_links = newLinks;
    m_isProcessingText = false;
}
