#include "toolcallparser.h"

#include <QDebug>
#include <QtGlobal>
#include <QtLogging>

#include <cstddef>

static const QString ToolCallStart = ToolCallConstants::CodeInterpreterTag;
static const QString ToolCallEnd = ToolCallConstants::CodeInterpreterEndTag;

ToolCallParser::ToolCallParser()
{
    reset();
}

void ToolCallParser::reset()
{
    // Resets the search state, but not the buffer or global state
    resetSearchState();

    // These are global states maintained between update calls
    m_buffer.clear();
    m_hasSplit = false;
}

void ToolCallParser::resetSearchState()
{
    m_expected = ToolCallStart.at(0);
    m_expectedIndex = 0;
    m_state = ToolEnums::ParseState::None;
    m_toolCall.clear();
    m_endTagBuffer.clear();
    m_startIndex = -1;
}

// This method is called with an arbitrary string and a current state. This method should take the
// current state into account and then parse through the update character by character to arrive at
// the new state.
void ToolCallParser::update(const QString &update)
{
    Q_ASSERT(m_state != ToolEnums::ParseState::Complete);
    if (m_state == ToolEnums::ParseState::Complete) {
        qWarning() << "ERROR: ToolCallParser::update already found a complete toolcall!";
        return;
    }

    m_buffer.append(update);

    for (size_t i = m_buffer.size() - update.size(); i < m_buffer.size(); ++i) {
        const QChar c = m_buffer[i];
        const bool foundMatch = m_expected.isNull() || c == m_expected;
        if (!foundMatch) {
            resetSearchState();
            continue;
        }

        switch (m_state) {
        case ToolEnums::ParseState::None:
            {
                m_expectedIndex = 1;
                m_expected = ToolCallStart.at(1);
                m_state = ToolEnums::ParseState::InStart;
                m_startIndex = i;
                break;
            }
        case ToolEnums::ParseState::InStart:
            {
                if (m_expectedIndex == ToolCallStart.size() - 1) {
                    m_expectedIndex = 0;
                    m_expected = QChar();
                    m_state = ToolEnums::ParseState::Partial;
                } else {
                    ++m_expectedIndex;
                    m_expected = ToolCallStart.at(m_expectedIndex);
                }
                break;
            }
        case ToolEnums::ParseState::Partial:
            {
                m_toolCall.append(c);
                m_endTagBuffer.append(c);
                if (m_endTagBuffer.size() > ToolCallEnd.size())
                    m_endTagBuffer.remove(0, 1);
                if (m_endTagBuffer == ToolCallEnd) {
                    m_toolCall.chop(ToolCallEnd.size());
                    m_state = ToolEnums::ParseState::Complete;
                    m_endTagBuffer.clear();
                }
            }
        case ToolEnums::ParseState::Complete:
            {
                // Already complete, do nothing further
                break;
            }
        }
    }
}

QPair<QString, QString> ToolCallParser::split()
{
    Q_ASSERT(m_state == ToolEnums::ParseState::Partial
        || m_state == ToolEnums::ParseState::Complete);

    Q_ASSERT(m_startIndex >= 0);
    m_hasSplit = true;
    const QString beforeToolCall = m_buffer.left(m_startIndex);
    m_buffer = m_buffer.mid(m_startIndex);
    m_startIndex = 0;
    return { beforeToolCall, m_buffer };
}
