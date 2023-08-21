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

#if defined(CROSSPATH_IS_LINUX)
#  include <unistd.h>
#elif defined(CROSSPATH_IS_WINDOWS)
#  include <windows.h>
#  include <processthreadsapi.h>
#endif



Crosspath::Crosspath() {
#if defined(CROSSPATH_IS_POSIX)
    home = getenv("HOME");
#elif defined(CROSSPATH_IS_WINDOWS)
    home = getenv("USERPROFILE");
#endif

    std::error_code ec;
    std::filesystem::create_directories(data(), ec);
    std::filesystem::create_directories(cache(), ec);
    std::filesystem::create_directories(tmp(), ec);
}

Crosspath::~Crosspath() {
    std::error_code ec;
    std::filesystem::remove_all(tmp(), ec);
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

std::string Crosspath::cache() const {
    return home +
#if defined(CROSSPATH_IS_POSIX)
        "/.cache/" CROSSPATH_VENDOR "/" CROSSPATH_APP "/";
#elif defined(CROSSPATH_IS_WINDOWS)
        "\\AppData\\Local\\" CROSSPATH_VENDOR "\\" CROSSPATH_APP "\\Cache\\";
#endif
}

std::string Crosspath::tmp() const {
    return
#if defined(CROSSPATH_IS_LINUX)
        "/tmp/" CROSSPATH_VENDOR "-" CROSSPATH_APP "." + std::to_string(getpid()) + "/";
#elif defined(CROSSPATH_IS_DARWIN)
        getenv("TMPDIR");
#elif defined(CROSSPATH_IS_WINDOWS)
        "\\AppData\\Local\\Temp\\" CROSSPATH_VENDOR "-" CROSSPATH_APP "." + std::to_string(GetCurrentProcessId()) + "\\";
#endif
}
