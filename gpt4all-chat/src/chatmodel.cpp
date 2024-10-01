#include "chatmodel.h"

ChatModelIterator::ChatModelIterator(ChatModel* model, int index)
    : m_model(model), m_index(index)
{
}

ChatItem &ChatModelIterator::operator*() const
{
    Q_ASSERT(m_model);
    return m_model->m_chatItems[m_index];
}

ChatModelIterator &ChatModelIterator::operator++()
{
    if (m_model && m_index < m_model->m_chatItems.size())
        ++m_index;
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
    return m_index < other.m_index;
}