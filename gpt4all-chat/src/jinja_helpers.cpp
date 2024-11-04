#include "jinja_helpers.h"

#include "utils.h"

#include <fmt/format.h>

#include <QString>
#include <QUrl>
#include <QtGlobal>

#include <memory>
#include <vector>

using namespace std::literals::string_view_literals;

std::vector<std::string> JinjaMessage::GetKeys() const
{
    std::vector<std::string> result;
    auto &keys = this->keys();
    result.reserve(keys.size());
    result.assign(keys.begin(), keys.end());
    return result;
}

auto JinjaMessage::keys() const -> const std::unordered_set<std::string_view> &
{
    static const std::unordered_set<std::string_view> baseKeys
        { "role", "content" };
    return baseKeys;
}

bool operator==(const JinjaMessage &a, const JinjaMessage &b)
{
    if (a.m_item == b.m_item)
        return true;
    const auto &[ia, ib] = std::tie(*a.m_item, *b.m_item);
    return ia.type() == ib.type() && ia.value == ib.value;
}

const JinjaFieldMap<ChatItem> JinjaMessage::s_fields = {
    { "role", [](auto &i) {
        switch (i.type()) {
            using enum ChatItem::Type;
            case System:   return "system"sv;
            case Prompt:   return "user"sv;
            case Response: return "assistant"sv;
        }
        Q_UNREACHABLE();
    } },
    { "content", [](auto &i) { return i.value.toStdString(); } },
};
