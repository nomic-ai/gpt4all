#ifndef RESPONSETEXT_H
#define RESPONSETEXT_H

#include <QObject>
#include <QQmlEngine>
#include <QQuickTextDocument>
#include <QSyntaxHighlighter>
#include <QRegularExpression>

struct HighlightingRule
{
    QRegularExpression pattern;
    QTextCharFormat format;
};

class SyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    SyntaxHighlighter(QObject *parent);
    ~SyntaxHighlighter();

    void highlightBlock(const QString &text) override;

private:
    QVector<HighlightingRule> m_highlightingRules;
};

struct ContextLink {
    int startPos = -1;
    int endPos = -1;
    QString text;
    QString href;
};

class ResponseText : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickTextDocument* textDocument READ textDocument WRITE setTextDocument NOTIFY textDocumentChanged())
    QML_ELEMENT
public:
    explicit ResponseText(QObject *parent = nullptr);

    QQuickTextDocument* textDocument() const;
    void setTextDocument(QQuickTextDocument* textDocument);

    Q_INVOKABLE void setLinkColor(const QColor &c) { m_linkColor = c; }
    Q_INVOKABLE QString getLinkAtPosition(int position) const;

Q_SIGNALS:
    void textDocumentChanged();

private Q_SLOTS:
    void handleTextChanged();

private:
    QQuickTextDocument *m_textDocument;
    SyntaxHighlighter *m_syntaxHighlighter;
    QVector<ContextLink> m_links;
    QColor m_linkColor;
    bool m_isProcessingText = false;
};

#endif // RESPONSETEXT_H
