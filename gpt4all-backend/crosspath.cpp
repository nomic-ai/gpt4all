#include "crosspath.h"

#include <filesystem>

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  define CROSSPATH_IS_WINDOWS
#else
#  define CROSSPATH_IS_POSIX
#  if defined(__linux__)
#    define CROSSPATH_IS_LINUX
#  elif defined(__APPLE__)
#    define CROSSPATH_IS_DARWIN
#  endif
#endif



Crosspath::Crosspath() {
#if defined(CROSSPATH_IS_POSIX)
    home = getenv("HOME");
#elif defined(CROSSPATH_IS_WINDOWS)
    home = getenv("USERPROFILE");
#endif

    std::error_code ec;
    std::filesystem::create_directories(data(), ec);
    std::filesystem::create_directories(config(), ec);
    std::filesystem::create_directories(cache(), ec);
    std::filesystem::create_directories(temp(), ec);
}

std::string Crosspath::data() const {
    return home +
#if defined(CROSSPATH_IS_LINUX)
        "/.local/share/" CROSSPATH_VENDOR "/" CROSSPATH_APP "/";
#elif defined(CROSSPATH_IS_DARWIN)
        "/Library/Application Support/" CROSSPATH_VENDOR "/" CROSSPATH_APP "/";
#elif defined(CROSSPATH_IS_WINDOWS)
        "\\AppData\\Local\\" CROSSPATH_VENDOR "\\" CROSSPATH_APP "\\";
#endif
}

std::string Crosspath::config() const {
    return home +
#if defined(CROSSPATH_IS_LINUX)
        "/.config/" CROSSPATH_VENDOR "/" CROSSPATH_APP "/";
#elif defined(CROSSPATH_IS_DARWIN)
        "/Library/Preferences/" CROSSPATH_VENDOR "/" CROSSPATH_APP "/";
#elif defined(CROSSPATH_IS_WINDOWS)
        "\\AppData\\Local\\" CROSSPATH_VENDOR "\\" CROSSPATH_APP "\\Config\\";
#endif
}

std::string Crosspath::cache() const {
    return home +
#if defined(CROSSPATH_IS_POSIX)
        "/.cache/" CROSSPATH_VENDOR "/" CROSSPATH_APP "/";
#elif defined(CROSSPATH_IS_WINDOWS)
        "\\AppData\\Local\\" CROSSPATH_VENDOR "\\" CROSSPATH_APP "\\Cache\\";
#endif
}

std::string Crosspath::temp() const {
    return
#if defined(CROSSPATH_IS_LINUX)
        "/tmp/" CROSSPATH_VENDOR "-" CROSSPATH_APP "/";
#elif defined(CROSSPATH_IS_DARWIN)
        getenv("TMPDIR");
#elif defined(CROSSPATH_IS_WINDOWS)
        home + "\\AppData\\Local\\Temp\\" CROSSPATH_VENDOR "-" CROSSPATH_APP "\\";
#endif
}
