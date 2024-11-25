#pragma once

#include <fmt/base.h>
#include <fmt/format.h>

#include <QByteArray>
#include <QJsonValue>
#include <QLatin1StringView>
#include <QString>
#include <QStringView>
#include <QUtf8StringView>
#include <QVariant>

#include <initializer_list>
#include <string_view>
#include <utility>

class QJsonObject;


// fmtlib formatters for QString and QVariant

#define MAKE_FORMATTER(type, conversion)                                        \
    template <>                                                                 \
    struct fmt::formatter<type, char>: fmt::formatter<std::string_view, char> { \
        template <typename FmtContext>                                          \
        FmtContext::iterator format(const type &value, FmtContext &ctx) const   \
        {                                                                       \
            auto valueUtf8 = (conversion);                                      \
            std::string_view view(valueUtf8.cbegin(), valueUtf8.cend());        \
            return formatter<std::string_view, char>::format(view, ctx);        \
        }                                                                       \
    }

MAKE_FORMATTER(QUtf8StringView, value                    );
MAKE_FORMATTER(QStringView,     value.toUtf8()           );
MAKE_FORMATTER(QString,         value.toUtf8()           );
MAKE_FORMATTER(QVariant,        value.toString().toUtf8());

// alternative to QJsonObject's initializer_list constructor that accepts Latin-1 strings
QJsonObject makeJsonObject(std::initializer_list<std::pair<QLatin1StringView, QJsonValue>> args);

#include "utils.inl"
