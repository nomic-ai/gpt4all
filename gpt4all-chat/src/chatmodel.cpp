#include "chatmodel.h"

ChatModelIterator::ChatModelIterator(ChatModel* model, QList<ChatItem>::iterator it)
    : m_model(model), m_it(it)
{
}

ChatItem &ChatModelIterator::operator*() const
{
    Q_ASSERT(m_model);
    return *m_it;
}

ChatModelIterator &ChatModelIterator::operator++()
{
    ++m_it;
    return *this;
}

ChatModelIterator ChatModelIterator::operator++(int)
{
    ChatModelIterator temp = *this;
    ++(*this);
    return temp;
}

bool ChatModelIterator::operator<(const ChatModelIterator& other) const
{
    return m_it < other.m_it;
}
