#pragma once

#include <filesystem>
#include <regex>
#include <stdexcept>
#include <string>
#include <utility>

namespace fs = std::filesystem;


// platform-aware Unicode paths

#ifndef _WIN32
#   define PATH_LITERAL(s) s
#else
#   define PATH_LITERAL(s) L##s
#endif

using PathString = fs::path::string_type;
using PathRegex  = std::basic_regex<fs::path::value_type>;

fs::path pathFromUtf8(const std::string &utf8);


class Dlhandle {
    void *chandle = nullptr;

public:
    class Exception : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };

    Dlhandle() = default;
    Dlhandle(const PathString &fpath);
    Dlhandle(const Dlhandle &o) = delete;
    Dlhandle(Dlhandle &&o)
        : chandle(o.chandle)
    {
        o.chandle = nullptr;
    }

    ~Dlhandle();

    Dlhandle &operator=(Dlhandle &&o) {
        chandle = std::exchange(o.chandle, nullptr);
        return *this;
    }

    template <typename T>
    T *get(const std::string &symbol) const {
        return reinterpret_cast<T *>(get_internal(symbol.c_str()));
    }

    auto get_fnc(const std::string &symbol) const {
        return get<void*(...)>(symbol);
    }

private:
    void *get_internal(const char *symbol) const;
};
