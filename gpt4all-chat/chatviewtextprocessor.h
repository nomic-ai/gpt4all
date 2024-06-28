#ifndef CHATVIEWTEXTPROCESSOR_H
#define CHATVIEWTEXTPROCESSOR_H

#include <QColor>
#include <QObject>
#include <QQmlEngine>
#include <QQuickTextDocument> // IWYU pragma: keep
#include <QRectF>
#include <QSizeF>
#include <QString>
#include <QSyntaxHighlighter>
#include <QTextObjectInterface>
#include <QVector>

class QPainter;
class QTextDocument;
class QTextFormat;

class SyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    SyntaxHighlighter(QObject *parent);
    ~SyntaxHighlighter();
    void highlightBlock(const QString &text) override;
};

struct ContextLink {
    int startPos = -1;
    int endPos = -1;
    QString text;
    QString href;
};

struct CodeCopy {
    int startPos = -1;
    int endPos = -1;
    QString text;
};

class ChatViewTextProcessor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickTextDocument* textDocument READ textDocument WRITE setTextDocument NOTIFY textDocumentChanged())
    Q_PROPERTY(bool shouldProcessText READ shouldProcessText WRITE setShouldProcessText NOTIFY shouldProcessTextChanged())
    QML_ELEMENT
public:
    explicit ChatViewTextProcessor(QObject *parent = nullptr);

    QQuickTextDocument* textDocument() const;
    void setTextDocument(QQuickTextDocument* textDocument);

    Q_INVOKABLE void setLinkColor(const QColor &c) { m_linkColor = c; }
    Q_INVOKABLE void setHeaderColor(const QColor &c) { m_headerColor = c; }

    Q_INVOKABLE bool tryCopyAtPosition(int position) const;

    bool shouldProcessText() const;
    void setShouldProcessText(bool b);

Q_SIGNALS:
    void textDocumentChanged();
    void shouldProcessTextChanged();

private Q_SLOTS:
    void handleTextChanged();
    void handleCodeBlocks();
    void handleMarkdown();

private:
    QQuickTextDocument *m_textDocument;
    SyntaxHighlighter *m_syntaxHighlighter;
    QVector<ContextLink> m_links;
    QVector<CodeCopy> m_copies;
    QColor m_linkColor;
    QColor m_headerColor;
    bool m_shouldProcessText = false;
    bool m_isProcessingText = false;
};

#endif // CHATVIEWTEXTPROCESSOR_H
