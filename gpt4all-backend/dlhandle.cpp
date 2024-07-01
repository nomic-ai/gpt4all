#include "dlhandle.h"

#include <string>

#ifndef _WIN32
#   include <dlfcn.h>
#else
#   include <cassert>
#   include <sstream>
#   define WIN32_LEAN_AND_MEAN
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
#   include <windows.h>
#endif

using namespace std::string_literals;
namespace fs = std::filesystem;


#ifndef _WIN32

Dlhandle::Dlhandle(const fs::path &fpath)
{
    chandle = dlopen(fpath.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (!chandle) {
        throw Exception("dlopen: "s + dlerror());
    }
}

Dlhandle::~Dlhandle()
{
    if (chandle) dlclose(chandle);
}

void *Dlhandle::get_internal(const char *symbol) const
{
    return dlsym(chandle, symbol);
}

#else // defined(_WIN32)

Dlhandle::Dlhandle(const fs::path &fpath)
{
    fs::path afpath = fs::absolute(fpath);

    // Suppress the "Entry Point Not Found" dialog, caused by outdated nvcuda.dll from the GPU driver
    UINT lastErrorMode = GetErrorMode();
    SetErrorMode(lastErrorMode | SEM_FAILCRITICALERRORS);

    chandle = LoadLibraryExW(afpath.c_str(), NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);

    SetErrorMode(lastErrorMode);

    if (!chandle) {
        DWORD err = GetLastError();
        std::ostringstream ss;
        ss << "LoadLibraryExW failed with error 0x" << std::hex << err;
        throw Exception(ss.str());
    }
}

Dlhandle::~Dlhandle()
{
    if (chandle) FreeLibrary(HMODULE(chandle));
}

void *Dlhandle::get_internal(const char *symbol) const
{
    return GetProcAddress(HMODULE(chandle), symbol);
}

#endif // defined(_WIN32)
