#include "jinja_helpers.h"

#include <QString>
#include <QUrl>

#include <ranges>
#include <string>
#include <utility>

namespace views  = std::views;
using json = nlohmann::ordered_json;


json::object_t JinjaResultInfo::AsJson() const
{
    return {
        { "collection", m_source->collection.toStdString() },
        { "path",       m_source->path      .toStdString() },
        { "file",       m_source->file      .toStdString() },
        { "title",      m_source->title     .toStdString() },
        { "author",     m_source->author    .toStdString() },
        { "date",       m_source->date      .toStdString() },
        { "text",       m_source->text      .toStdString() },
        { "page",       m_source->page                     },
        { "file_uri",   m_source->fileUri() .toStdString() },
    };
}

json::object_t JinjaPromptAttachment::AsJson() const
{
    return {
        { "url",               m_attachment->url.toString()    .toStdString() },
        { "file",              m_attachment->file()            .toStdString() },
        { "processed_content", m_attachment->processedContent().toStdString() },
    };
}

json::object_t JinjaMessage::AsJson() const
{
    json::object_t obj;
    {
        json::string_t role;
        switch (m_item->type()) {
            using enum MessageItem::Type;
            case System:       role = "system";    break;
            case Prompt:       role = "user";      break;
            case Response:     role = "assistant"; break;
            case ToolResponse: role = "tool";      break;
        }
        obj.emplace_back("role", std::move(role));
    }
    {
        QString content;
        if (m_version == 0 && m_item->type() == MessageItem::Type::Prompt) {
            content = m_item->bakedPrompt();
        } else {
            content = m_item->content();
        }
        obj.emplace_back("content", content.toStdString());
    }
    if (m_item->type() == MessageItem::Type::Prompt) {
        {
            auto sources = m_item->sources() | views::transform([](auto &r) {
                return JinjaResultInfo(r).AsJson();
            });
            obj.emplace("sources", json::array_t(sources.begin(), sources.end()));
        }
        {
            auto attachments = m_item->promptAttachments() | views::transform([](auto &pa) {
                return JinjaPromptAttachment(pa).AsJson();
            });
            obj.emplace("prompt_attachments", json::array_t(attachments.begin(), attachments.end()));
        }
    }
    return obj;
}
