#include "chatmodel.h"

MessageItem::MessageItem(const ChatItem *item)
{
    switch (item->type()) {
    case ChatItem::Type::System:        m_type = MessageItem::Type::System; break;
    case ChatItem::Type::Prompt:        m_type = MessageItem::Type::System; break;
    case ChatItem::Type::Response:      m_type = MessageItem::Type::System; break;
    case ChatItem::Type::ToolResponse:  m_type = MessageItem::Type::System; break;
    case ChatItem::Type::Text:
    case ChatItem::Type::ToolCall:
        Q_UNREACHABLE();
        break;
    }

    m_content = item->flattenedContent().toUtf8();
}
