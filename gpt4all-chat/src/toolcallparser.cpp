#include "toolcallparser.h"

#include <QDebug>
#include <QtGlobal>
#include <QtLogging>

#include <cstddef>

ToolCallParser::ToolCallParser()
{
    m_possibleStartTags << ToolCallConstants::CodeInterpreterTag.toUtf8()
                        << ToolCallConstants::ThinkTag.toUtf8();
    m_possibleEndTags   << ToolCallConstants::CodeInterpreterEndTag.toUtf8()
                        << ToolCallConstants::ThinkEndTag.toUtf8();
    reset();
}

void ToolCallParser::reset()
{
    // Resets the search state, but not the buffer or global state
    resetSearchState();

    // These are global states maintained between update calls
    m_buffers.clear();
    m_buffers << QByteArray();
}

void ToolCallParser::resetSearchState()
{
    m_expected = {'<'};
    m_expectedIndex = 0;
    m_state = ToolEnums::ParseState::None;

    m_toolCall.clear();
    m_startTagBuffer.clear();
    m_endTagBuffer.clear();

    m_currentTagIndex = -1;
    m_startIndex = -1;
    m_endIndex = -1;
}

bool ToolCallParser::isExpected(char c) const
{
    return m_expected.isEmpty() || m_expected.contains(c);
}

void ToolCallParser::setExpected(const QList<QByteArray> &tags)
{
    m_expected.clear();
    for (const auto &tag : tags) {
        Q_ASSERT(tag.size() > m_expectedIndex);
        m_expected << tag.at(m_expectedIndex);
    }
}

QByteArray ToolCallParser::startTag() const
{
    if (m_currentTagIndex < 0)
        return {};
    return m_possibleStartTags.at(m_currentTagIndex);
}

QByteArray ToolCallParser::endTag() const
{
    if (m_currentTagIndex < 0)
        return {};
    return m_possibleEndTags.at(m_currentTagIndex);
}

QByteArray &ToolCallParser::currentBuffer()
{
    return m_buffers.last();
}

// This method is called with an arbitrary string and a current state. This method should take the
// current state into account and then parse through the update character by character to arrive at
// the new state.
void ToolCallParser::update(const QByteArray &update)
{
    currentBuffer().append(update);

    for (size_t i = currentBuffer().size() - update.size(); i < currentBuffer().size(); ++i) {
        const char c = currentBuffer()[i];
        const bool foundMatch = isExpected(c);
        if (!foundMatch) {
            resetSearchState();
            continue;
        }

        switch (m_state) {
        case ToolEnums::ParseState::None:
            {
                m_expectedIndex = 1;
                setExpected(m_possibleStartTags);
                m_state = ToolEnums::ParseState::InTagChoice;
                m_startIndex = i;
                break;
            }
        case ToolEnums::ParseState::InTagChoice:
            {
                for (int i = 0; i < m_possibleStartTags.size(); ++i) {
                    const auto &tag = m_possibleStartTags.at(i);
                    if (c == tag.at(1)) m_currentTagIndex = i;
                }
                if (m_currentTagIndex >= 0) {
                    m_expectedIndex = 2;
                    setExpected({m_possibleStartTags.at(m_currentTagIndex)});
                    m_state = ToolEnums::ParseState::InStart;
                } else
                    resetSearchState();
                break;
            }
        case ToolEnums::ParseState::InStart:
            {
                m_startTagBuffer.append(c);

                const auto startTag = this->startTag();
                Q_ASSERT(!startTag.isEmpty());
                if (m_expectedIndex == startTag.size() - 1) {
                    m_expectedIndex = 0;
                    setExpected({});
                    m_state = ToolEnums::ParseState::Partial;
                } else {
                    ++m_expectedIndex;
                    Q_ASSERT(m_currentTagIndex >= 0);
                    setExpected({startTag});
                }
                break;
            }
        case ToolEnums::ParseState::Partial:
            {
                Q_ASSERT(m_currentTagIndex >= 0);
                const auto endTag = this->endTag();
                Q_ASSERT(!endTag.isEmpty());
                m_toolCall.append(c);
                m_endTagBuffer.append(c);
                if (m_endTagBuffer.size() > endTag.size())
                    m_endTagBuffer.remove(0, 1);
                if (m_endTagBuffer == endTag) {
                    m_endIndex = i + 1;
                    m_toolCall.chop(endTag.size());
                    m_state = ToolEnums::ParseState::Complete;
                    m_endTagBuffer.clear();
                }
                break;
            }
        case ToolEnums::ParseState::Complete:
            {
                // Already complete, do nothing further
                break;
            }
        }
    }
}

bool ToolCallParser::splitIfPossible()
{
    // The first split happens when we're in a partial state
    if (m_buffers.size() < 2 && m_state == ToolEnums::ParseState::Partial) {
        Q_ASSERT(m_startIndex >= 0);
        const auto beforeToolCall = currentBuffer().left(m_startIndex);
        const auto toolCall       = currentBuffer().mid (m_startIndex);
        m_buffers = { beforeToolCall, toolCall };
        return true;
    }

    // The second split happens when we're in the complete state
    if (m_buffers.size() < 3 && m_state == ToolEnums::ParseState::Complete) {
        Q_ASSERT(m_endIndex >= 0);
        const auto &beforeToolCall = m_buffers.first();
        const auto toolCall        = currentBuffer().left(m_endIndex);
        const auto afterToolCall   = currentBuffer().mid (m_endIndex);
        m_buffers = { beforeToolCall, toolCall, afterToolCall };
        return true;
    }

    return false;
}

QStringList ToolCallParser::buffers() const
{
    QStringList result;
    result.reserve(m_buffers.size());
    for (const auto &buffer : m_buffers)
        result << QString::fromUtf8(buffer);
    return result;
}
