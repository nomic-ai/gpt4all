#include <filesystem>
#include <string>

#ifndef _WIN32
#   include <dlfcn.h>
#else
#   include <algorithm>
#   include <filesystem>
#   include <sstream>
#   define WIN32_LEAN_AND_MEAN
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
#   include <windows.h>
#endif

#include "oscompat.h"

namespace fs = std::filesystem;


#ifndef _WIN32

fs::path pathFromUtf8(const std::string &utf8) { return utf8; }

Dlhandle::Dlhandle(const PathString &fpath) {
    chandle = dlopen(fpath.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (!chandle) {
        throw Exception("dlopen(\"" + fpath + "\"): " + dlerror());
    }
}

Dlhandle::~Dlhandle() {
    if (chandle) dlclose(chandle);
}

void *Dlhandle::get_internal(const char *symbol) const {
    return dlsym(chandle, symbol);
}

#else // defined(_WIN32)

static std::wstring utf8ToWide(const std::string &mbs) {
    std::wstring wstr;

    int wlen = MultiByteToWideChar(CP_UTF8, 0, mbs.c_str(), -1, NULL, 0);
    if (wlen) {
        wstr.resize(wlen - 1); // exclude null terminator
        wlen = MultiByteToWideChar(CP_UTF8, 0, mbs.c_str(), -1, wstr.data(), wlen);
        if (wlen) return wstr;
    }

    auto err = GetLastError();
    std::ostringstream ss;
    ss << "MultiByteToWideChar(\"" << mbs << "\") failed with error 0x" << std::hex << err;
    throw std::runtime_error(ss.str());
}

fs::path pathFromUtf8(const std::string &utf8) { return utf8ToWide(utf8); }

Dlhandle::Dlhandle(const PathString &fpath) {
    auto afpath = std::filesystem::absolute(fpath).wstring();
    std::replace(afpath.begin(), afpath.end(), L'/', L'\\');

    chandle = LoadLibraryExW(afpath.c_str(), NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);

    if (!chandle) {
        auto err = GetLastError();
        std::ostringstream ss;
        ss << "LoadLibraryExW failed with error 0x" << std::hex << err;
        throw Exception(ss.str());
    }
}

Dlhandle::~Dlhandle() {
    if (chandle) FreeLibrary(HMODULE(chandle));
}

void *Dlhandle::get_internal(const char *symbol) const {
    return GetProcAddress(HMODULE(chandle), symbol);
}

#endif // defined(_WIN32)
