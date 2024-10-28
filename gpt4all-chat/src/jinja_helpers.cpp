#include "jinja_helpers.h"

#include "utils.h"

#include <fmt/format.h>

#include <QString>
#include <QUrl>
#include <QtGlobal>

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
    { "fileUri",    [](auto &s) { return s.fileUri() .toStdString(); } },
};

JinjaPromptAttachment::~JinjaPromptAttachment() = default;

const JinjaFieldMap<PromptAttachment> JinjaPromptAttachment::s_fields = {
    { "url",              [](auto &s) { return s.url.toString()    .toStdString(); } },
    { "file",             [](auto &s) { return s.file()            .toStdString(); } },
    { "processedContent", [](auto &s) { return s.processedContent().toStdString(); } },
};

std::vector<std::string> JinjaMessage::GetKeys() const
{
    std::vector<std::string> result;
    auto &keys = this->keys();
    result.reserve(keys.size());
    result.assign(keys.begin(), keys.end());
    return result;
}

auto JinjaMessage::role() const -> Role
{
    if (m_item->name == "Prompt: "_L1)   return Role::User;
    if (m_item->name == "Response: "_L1) return Role::Assistant;
    throw std::invalid_argument(fmt::format("unrecognized ChatItem name: {}", m_item->name));
}

auto JinjaMessage::keys() const -> const std::unordered_set<std::string_view> &
{
    static const std::unordered_set<std::string_view> userKeys
        { "role", "content", "sources", "prompt_attachments" };
    static const std::unordered_set<std::string_view> assistantKeys
        { "role", "content" };
    switch (role()) {
        case Role::User:      return userKeys;
        case Role::Assistant: return assistantKeys;
    }
    Q_UNREACHABLE();
}

const JinjaFieldMap<JinjaMessage> JinjaMessage::s_fields = {
    { "role", [](auto &m) {
        switch (m.role()) {
            case Role::User:      return "user"sv;
            case Role::Assistant: return "assistant"sv;
        }
        Q_UNREACHABLE();
    } },
    { "content", [](auto &m) { return m.item().value.toStdString(); } },
    { "sources", [](auto &m) {
        auto sources = m.item().sources | views::transform([](auto &r) {
            return jinja2::GenericMap([map = std::make_shared<JinjaResultInfo>(r)] { return map.get(); });
        });
        return jinja2::ValuesList(sources.begin(), sources.end());
    } },
    { "prompt_attachments", [](auto &m) {
        auto attachments = m.item().promptAttachments | views::transform([](auto &pa) {
            return jinja2::GenericMap([map = std::make_shared<JinjaPromptAttachment>(pa)] { return map.get(); });
        });
        return jinja2::ValuesList(attachments.begin(), attachments.end());
    } },
};
