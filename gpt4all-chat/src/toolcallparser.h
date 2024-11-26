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
}

class ToolCallParser
{
public:
    ToolCallParser();
    void reset();
    void update(const QString &update);
    QString buffer() const { return m_buffer; }
    QString toolCall() const { return m_toolCall; }
    int startIndex() const { return m_startIndex; }
    ToolEnums::ParseState state() const { return m_state; }

    // Splits
    QPair<QString, QString> split();
    bool hasSplit() const { return m_hasSplit; }

private:
    void resetSearchState();

    QChar m_expected;
    int m_expectedIndex;
    ToolEnums::ParseState m_state;
    QString m_buffer;
    QString m_toolCall;
    QString m_endTagBuffer;
    int m_startIndex;
    bool m_hasSplit;
};

#endif // TOOLCALLPARSER_H
