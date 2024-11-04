#pragma once

#include "chatmodel.h"
#include "database.h"

#include <jinja2cpp/value.h>

#include <functional>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace views = std::views;

template <typename T>
using JinjaFieldMap = std::unordered_map<std::string_view, std::function<jinja2::Value (const T &)>>;

template <typename Derived>
class JinjaComparable : public jinja2::IMapItemAccessor {
public:
    JinjaComparable() = default;

    bool IsEqual(const jinja2::IComparable &other) const override;

private:
    Q_DISABLE_COPY_MOVE(JinjaComparable)
};

template <typename Derived>
class JinjaHelper : public JinjaComparable<Derived> {
public:
    size_t GetSize() const override
    { return Derived::s_fields.size(); }

    bool HasValue(const std::string &name) const override
    { return Derived::s_fields.contains(name); }

    jinja2::Value GetValueByName(const std::string &name) const override;

    std::vector<std::string> GetKeys() const override
    { auto keys = views::elements<0>(Derived::s_fields); return { keys.begin(), keys.end() }; }
};

class JinjaResultInfo : public JinjaHelper<JinjaResultInfo> {
public:
    explicit JinjaResultInfo(const ResultInfo &source) noexcept
        : m_source(&source) {}

    ~JinjaResultInfo() override;

    const ResultInfo &value() const { return *m_source; }

    friend bool operator==(const JinjaResultInfo &a, const JinjaResultInfo &b)
    { return a.m_source == b.m_source || *a.m_source == *b.m_source; }

private:
    static const JinjaFieldMap<ResultInfo> s_fields;
    const ResultInfo *m_source;

    friend class JinjaHelper<JinjaResultInfo>;
};

class JinjaMessage : public JinjaHelper<JinjaMessage> {
public:
    explicit JinjaMessage(const ChatItem &item) noexcept
        : m_item(&item) {}

    const ChatItem &value() const { return *m_item; }

    size_t GetSize() const override { return keys().size(); }
    bool HasValue(const std::string &name) const override { return keys().contains(name); }

    jinja2::Value GetValueByName(const std::string &name) const override
    { return HasValue(name) ? JinjaHelper::GetValueByName(name) : jinja2::EmptyValue(); }

    std::vector<std::string> GetKeys() const override;

private:
    auto keys() const -> const std::unordered_set<std::string_view> &;

private:
    static const JinjaFieldMap<ChatItem> s_fields;
    const ChatItem *m_item;

    friend class JinjaHelper<JinjaMessage>;
    friend bool operator==(const JinjaMessage &a, const JinjaMessage &b);
};

#include "jinja_helpers.inl"
