#ifndef TOOLCALLPARSER_H
#define TOOLCALLPARSER_H

#include "tool.h"

#include <QChar>
#include <QString>
#include <QPair>

namespace ToolCallConstants
{
    const QString CodeInterpreterFunction = R"(javascript_interpret)";
    const QString CodeInterpreterTag = R"(<)" + CodeInterpreterFunction + R"(>)";
    const QString CodeInterpreterEndTag = R"(</)" + CodeInterpreterFunction + R"(>)";
    const QString CodeInterpreterPrefix = CodeInterpreterTag + "\n```javascript\n";
    const QString CodeInterpreterSuffix = "```\n" + CodeInterpreterEndTag;

    // NB: the parsing code assumes the first char of the various tags differ
    const QString ThinkTag = QStringLiteral("<think>");
    const QString ThinkEndTag = QStringLiteral("</think>");
}

class ToolCallParser
{
public:
    ToolCallParser();
    void reset();
    void update(const QString &update);
    QString toolCall() const { return m_toolCall; }
    int startIndex() const { return m_startIndex; }
    ToolEnums::ParseState state() const { return m_state; }
    QString startTag() const;
    QString endTag() const;

    bool splitIfPossible();
    const QVector<QString> &buffers() const;
    int numberOfBuffers() const { return m_buffers.size(); }

private:
    QString &currentBuffer();
    void resetSearchState();
    bool isExpected(QChar c) const;
    void setExpected(const QStringList &tags);

    QStringList m_possibleStartTags;
    QStringList m_possibleEndTags;
    QString m_startTagBuffer;
    QString m_endTagBuffer;
    int m_currentTagIndex;

    QVector<QChar> m_expected;
    int m_expectedIndex;
    ToolEnums::ParseState m_state;
    QVector<QString> m_buffers;
    QString m_toolCall;
    int m_startIndex;
    int m_endIndex;
};

#endif // TOOLCALLPARSER_H
