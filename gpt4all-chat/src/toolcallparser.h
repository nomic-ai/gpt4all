#ifndef TOOLCALLPARSER_H
#define TOOLCALLPARSER_H

#include "tool.h"

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

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
    void update(const QByteArray &update);
    QString toolCall() const { return QString::fromUtf8(m_toolCall); }
    int startIndex() const { return m_startIndex; }
    ToolEnums::ParseState state() const { return m_state; }
    QByteArray startTag() const;
    QByteArray endTag() const;

    bool splitIfPossible();
    QStringList buffers() const;
    int numberOfBuffers() const { return m_buffers.size(); }

private:
    QByteArray &currentBuffer();
    void resetSearchState();
    bool isExpected(char c) const;
    void setExpected(const QList<QByteArray> &tags);

    QList<QByteArray> m_possibleStartTags;
    QList<QByteArray> m_possibleEndTags;
    QByteArray m_startTagBuffer;
    QByteArray m_endTagBuffer;
    int m_currentTagIndex;

    QList<char> m_expected;
    int m_expectedIndex;
    ToolEnums::ParseState m_state;
    QList<QByteArray> m_buffers;
    QByteArray m_toolCall;
    int m_startIndex;
    int m_endIndex;
};

#endif // TOOLCALLPARSER_H
