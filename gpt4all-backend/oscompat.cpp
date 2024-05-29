#include <string>

#ifndef _WIN32
#   include <dlfcn.h>
#else
#   include <algorithm>
#   include <filesystem>
#   define WIN32_LEAN_AND_MEAN
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
#   include <windows.h>
#endif

#include "oscompat.h"

#ifndef _WIN32

Dlhandle::Dlhandle(const std::string &fpath) {
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

Dlhandle::Dlhandle(const std::string &fpath) {
    std::string afpath = std::filesystem::absolute(fpath).string();
    std::replace(afpath.begin(), afpath.end(), '/', '\\');
    chandle = LoadLibraryExA(afpath.c_str(), NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
    if (!chandle) {
        throw Exception("dlopen(\"" + fpath + "\"): Error");
    }
}

Dlhandle::~Dlhandle() {
    if (chandle) FreeLibrary(chandle);
}

void *Dlhandle::get_internal(const char *symbol) const {
    return GetProcAddress(chandle, symbol);
}

#endif // defined(_WIN32)
