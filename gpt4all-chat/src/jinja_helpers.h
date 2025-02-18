#pragma once

#include "chatmodel.h"
#include "database.h"

#include <nlohmann/json.hpp>

#include <QtTypes> // IWYU pragma: keep

// IWYU pragma: no_forward_declare MessageItem
// IWYU pragma: no_forward_declare PromptAttachment
// IWYU pragma: no_forward_declare ResultInfo

using json = nlohmann::ordered_json;


template <typename Derived>
class JinjaHelper {
public:
    json::object_t AsJson() const { return static_cast<const Derived *>(this)->AsJson(); }
};

class JinjaResultInfo : public JinjaHelper<JinjaResultInfo> {
public:
    explicit JinjaResultInfo(const ResultInfo &source) noexcept
        : m_source(&source) {}

    json::object_t AsJson() const;

private:
    const ResultInfo *m_source;
};

class JinjaPromptAttachment : public JinjaHelper<JinjaPromptAttachment> {
public:
    explicit JinjaPromptAttachment(const PromptAttachment &attachment) noexcept
        : m_attachment(&attachment) {}

    json::object_t AsJson() const;

private:
    const PromptAttachment *m_attachment;
};

class JinjaMessage : public JinjaHelper<JinjaMessage> {
public:
    explicit JinjaMessage(uint version, const MessageItem &item) noexcept
        : m_version(version), m_item(&item) {}

    json::object_t AsJson() const;

private:
    uint               m_version;
    const MessageItem *m_item;
};
