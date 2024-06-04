#ifndef RESPONSETEXT_H
#define RESPONSETEXT_H

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

class ContextLinkInterface : public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

public:
    explicit ContextLinkInterface(QObject *parent) : QObject(parent) {}
    void drawObject(QPainter *painter, const QRectF &rect, QTextDocument *doc, int posInDocument,
                    const QTextFormat &format) override;

    QSizeF intrinsicSize(QTextDocument *doc, int posInDocument, const QTextFormat &format) override;
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
    Q_INVOKABLE void setHeaderColor(const QColor &c) { m_headerColor = c; }

    Q_INVOKABLE QString getLinkAtPosition(int position) const;
    Q_INVOKABLE bool tryCopyAtPosition(int position) const;

Q_SIGNALS:
    void textDocumentChanged();

private Q_SLOTS:
    void handleTextChanged();
    void handleContextLinks();
    void handleCodeBlocks();

private:
    QQuickTextDocument *m_textDocument;
    SyntaxHighlighter *m_syntaxHighlighter;
    QVector<ContextLink> m_links;
    QVector<CodeCopy> m_copies;
    QColor m_linkColor;
    QColor m_headerColor;
    bool m_isProcessingText = false;
    ContextLinkInterface *m_contextLink;
};

#endif // RESPONSETEXT_H
