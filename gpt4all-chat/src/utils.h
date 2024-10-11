#pragma once

#include <fmt/base.h>
#include <fmt/format.h>

#include <QString>
#include <QVariant>

#include <string>


// fmtlib formatters for QString and QVariant

#define MAKE_FORMATTER(type, conversion)                                      \
    template <>                                                               \
    struct fmt::formatter<type, char>: fmt::formatter<std::string, char> {    \
        template <typename FmtContext>                                        \
        FmtContext::iterator format(const type &value, FmtContext &ctx) const \
        {                                                                     \
            return formatter<std::string, char>::format(conversion, ctx);     \
        }                                                                     \
    }

MAKE_FORMATTER(QString,  value.toStdString()           );
MAKE_FORMATTER(QVariant, value.toString().toStdString());
