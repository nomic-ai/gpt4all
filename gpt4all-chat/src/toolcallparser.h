#ifndef TOOLCALLPARSER_H
#define TOOLCALLPARSER_H

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

namespace ToolEnums { enum class ParseState; }

using namespace Qt::Literals::StringLiterals;


class ToolCallParser
{
public:
    ToolCallParser();
    ToolCallParser(const QStringList &tagNames);

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

    static QString makeStartTag(const QString &name) { return u"<%1>"_s .arg(name); }
    static QString makeEndTag  (const QString &name) { return u"</%1>"_s.arg(name); }

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

namespace ToolCallConstants
{
    // NB: the parsing code assumes the first char of the various tags differ

    inline const QString CodeInterpreterFunction = u"javascript_interpret"_s;
    inline const QString CodeInterpreterStartTag = ToolCallParser::makeStartTag(CodeInterpreterFunction);
    inline const QString CodeInterpreterEndTag   = ToolCallParser::makeEndTag  (CodeInterpreterFunction);
    inline const QString CodeInterpreterPrefix   = u"%1\n```javascript\n"_s.arg(CodeInterpreterStartTag);
    inline const QString CodeInterpreterSuffix   = u"```\n%1"_s            .arg(CodeInterpreterEndTag  );

    inline const QString ThinkTagName  = u"think"_s;
    inline const QString ThinkStartTag = ToolCallParser::makeStartTag(ThinkTagName);
    inline const QString ThinkEndTag   = ToolCallParser::makeEndTag  (ThinkTagName);

    inline const QStringList AllTagNames { CodeInterpreterFunction, ThinkTagName };
}

#endif // TOOLCALLPARSER_H
