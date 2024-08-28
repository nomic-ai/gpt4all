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

struct CodeColors {
    Q_GADGET
    Q_PROPERTY(QColor defaultColor MEMBER defaultColor)
    Q_PROPERTY(QColor keywordColor MEMBER keywordColor)
    Q_PROPERTY(QColor functionColor MEMBER functionColor)
    Q_PROPERTY(QColor functionCallColor MEMBER functionCallColor)
    Q_PROPERTY(QColor commentColor MEMBER commentColor)
    Q_PROPERTY(QColor stringColor MEMBER stringColor)
    Q_PROPERTY(QColor numberColor MEMBER numberColor)
    Q_PROPERTY(QColor headerColor MEMBER headerColor)
    Q_PROPERTY(QColor backgroundColor MEMBER backgroundColor)

public:
    QColor defaultColor;
    QColor keywordColor;
    QColor functionColor;
    QColor functionCallColor;
    QColor commentColor;
    QColor stringColor;
    QColor numberColor;
    QColor headerColor;
    QColor backgroundColor;

    QColor preprocessorColor = keywordColor;
    QColor typeColor = numberColor;
    QColor arrowColor = functionColor;
    QColor commandColor = functionCallColor;
    QColor variableColor = numberColor;
    QColor keyColor = functionColor;
    QColor valueColor = stringColor;
    QColor parameterColor = stringColor;
    QColor attributeNameColor = numberColor;
    QColor attributeValueColor = stringColor;
    QColor specialCharacterColor = functionColor;
    QColor doctypeColor = commentColor;
};

Q_DECLARE_METATYPE(CodeColors)

class SyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    SyntaxHighlighter(QObject *parent);
    ~SyntaxHighlighter();
    void highlightBlock(const QString &text) override;

    CodeColors codeColors() const { return m_codeColors; }
    void setCodeColors(const CodeColors &colors) { m_codeColors = colors; }

private:
    CodeColors m_codeColors;
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
    Q_PROPERTY(qreal fontPixelSize READ fontPixelSize WRITE setFontPixelSize NOTIFY fontPixelSizeChanged())
    Q_PROPERTY(CodeColors codeColors READ codeColors WRITE setCodeColors NOTIFY codeColorsChanged())
    QML_ELEMENT
public:
    explicit ChatViewTextProcessor(QObject *parent = nullptr);

    QQuickTextDocument* textDocument() const;
    void setTextDocument(QQuickTextDocument* textDocument);

    Q_INVOKABLE void setValue(const QString &value);
    Q_INVOKABLE bool tryCopyAtPosition(int position) const;

    bool shouldProcessText() const;
    void setShouldProcessText(bool b);

    qreal fontPixelSize() const;
    void setFontPixelSize(qreal b);

    CodeColors codeColors() const;
    void setCodeColors(const CodeColors &colors);

Q_SIGNALS:
    void textDocumentChanged();
    void shouldProcessTextChanged();
    void fontPixelSizeChanged();
    void codeColorsChanged();

private Q_SLOTS:
    void handleTextChanged();
    void handleCodeBlocks();
    void handleMarkdown();

private:
    QQuickTextDocument *m_quickTextDocument;
    SyntaxHighlighter *m_syntaxHighlighter;
    QVector<ContextLink> m_links;
    QVector<CodeCopy> m_copies;
    bool m_shouldProcessText = false;
    qreal m_fontPixelSize;
};

#endif // CHATVIEWTEXTPROCESSOR_H
