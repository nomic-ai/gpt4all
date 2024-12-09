#include "jinja_helpers.h"

#include "utils.h"

#include <fmt/format.h>

#include <QString>
#include <QUrl>

#include <memory>
#include <vector>

using namespace std::literals::string_view_literals;


JinjaResultInfo::~JinjaResultInfo() = default;

const JinjaFieldMap<ResultInfo> JinjaResultInfo::s_fields = {
    { "collection", [](auto &s) { return s.collection.toStdString(); } },
    { "path",       [](auto &s) { return s.path      .toStdString(); } },
    { "file",       [](auto &s) { return s.file      .toStdString(); } },
    { "title",      [](auto &s) { return s.title     .toStdString(); } },
    { "author",     [](auto &s) { return s.author    .toStdString(); } },
    { "date",       [](auto &s) { return s.date      .toStdString(); } },
    { "text",       [](auto &s) { return s.text      .toStdString(); } },
    { "page",       [](auto &s) { return s.page;                     } },
    { "file_uri",   [](auto &s) { return s.fileUri() .toStdString(); } },
};

JinjaPromptAttachment::~JinjaPromptAttachment() = default;

const JinjaFieldMap<PromptAttachment> JinjaPromptAttachment::s_fields = {
    { "url",               [](auto &s) { return s.url.toString()    .toStdString(); } },
    { "file",              [](auto &s) { return s.file()            .toStdString(); } },
    { "processed_content", [](auto &s) { return s.processedContent().toStdString(); } },
};

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
    static const std::unordered_set<std::string_view> userKeys
        { "role", "content", "sources", "prompt_attachments" };
    switch (m_item->type()) {
        using enum MessageItem::Type;
    case System:
    case Response:
    case ToolResponse:
        return baseKeys;
    case Prompt:
        return userKeys;
        break;
    }
    Q_UNREACHABLE();
}

bool operator==(const JinjaMessage &a, const JinjaMessage &b)
{
    if (a.m_item == b.m_item)
        return true;
    const auto &[ia, ib] = std::tie(*a.m_item, *b.m_item);
    auto type = ia.type();
    if (type != ib.type() || ia.content() != ib.content())
        return false;

    switch (type) {
        using enum MessageItem::Type;
    case System:
    case Response:
    case ToolResponse:
        return true;
    case Prompt:
        return ia.sources() == ib.sources() && ia.promptAttachments() == ib.promptAttachments();
        break;
    }
    Q_UNREACHABLE();
}

const JinjaFieldMap<JinjaMessage> JinjaMessage::s_fields = {
    { "role", [](auto &m) {
        switch (m.item().type()) {
            using enum MessageItem::Type;
            case System:   return "system"sv;
            case Prompt:   return "user"sv;
            case Response: return "assistant"sv;
            case ToolResponse: return "tool"sv;
                break;
        }
        Q_UNREACHABLE();
    } },
    { "content", [](auto &m) {
        if (m.version() == 0 && m.item().type() == MessageItem::Type::Prompt)
            return m.item().bakedPrompt().toStdString();
        return m.item().content().toStdString();
    } },
    { "sources", [](auto &m) {
        auto sources = m.item().sources() | views::transform([](auto &r) {
            return jinja2::GenericMap([map = std::make_shared<JinjaResultInfo>(r)] { return map.get(); });
        });
        return jinja2::ValuesList(sources.begin(), sources.end());
    } },
    { "prompt_attachments", [](auto &m) {
        auto attachments = m.item().promptAttachments() | views::transform([](auto &pa) {
            return jinja2::GenericMap([map = std::make_shared<JinjaPromptAttachment>(pa)] { return map.get(); });
        });
        return jinja2::ValuesList(attachments.begin(), attachments.end());
    } },
};
