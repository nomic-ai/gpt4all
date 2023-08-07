#include "llmodel.h"
#include "dlhandle.h"
#include "sysinfo.h"

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cassert>
#include <cstdlib>
#include <sstream>
#ifdef _MSC_VER
#include <intrin.h>
#endif

std::string s_implementations_search_path = ".";

static bool has_at_least_minimal_hardware() {
#if defined(__x86_64__) || defined(_M_X64)
    #ifndef _MSC_VER
        return __builtin_cpu_supports("avx");
    #else
        int cpuInfo[4];
        __cpuid(cpuInfo, 1);
        return cpuInfo[2] & (1 << 28);
    #endif
#else
    return true; // Don't know how to handle non-x86_64
#endif
}

static bool requires_avxonly() {
#if defined(__x86_64__) || defined(_M_X64)
    #ifndef _MSC_VER
        return !__builtin_cpu_supports("avx2");
    #else
        int cpuInfo[4];
        __cpuidex(cpuInfo, 7, 0);
        return !(cpuInfo[1] & (1 << 5));
    #endif
#else
    return false; // Don't know how to handle non-x86_64
#endif
}

LLModel::Implementation::Implementation(Dlhandle &&dlhandle_)
    : m_dlhandle(new Dlhandle(std::move(dlhandle_))) {
    auto get_model_type = m_dlhandle->get<const char *()>("get_model_type");
    assert(get_model_type);
    m_modelType = get_model_type();
    auto get_build_variant = m_dlhandle->get<const char *()>("get_build_variant");
    assert(get_build_variant);
    m_buildVariant = get_build_variant();
    m_magicMatch = m_dlhandle->get<bool(std::ifstream&)>("magic_match");
    assert(m_magicMatch);
    m_construct = m_dlhandle->get<LLModel *()>("construct");
    assert(m_construct);
}

LLModel::Implementation::Implementation(Implementation &&o)
    : m_magicMatch(o.m_magicMatch)
    , m_construct(o.m_construct)
    , m_modelType(o.m_modelType)
    , m_buildVariant(o.m_buildVariant)
    , m_dlhandle(o.m_dlhandle) {
    o.m_dlhandle = nullptr;
}

LLModel::Implementation::~Implementation() {
    if (m_dlhandle) delete m_dlhandle;
}

bool LLModel::Implementation::isImplementation(const Dlhandle &dl) {
    return dl.get<bool(uint32_t)>("is_g4a_backend_model_implementation");
}

const std::vector<LLModel::Implementation> &LLModel::Implementation::implementationList() {
    // NOTE: allocated on heap so we leak intentionally on exit so we have a chance to clean up the
    // individual models without the cleanup of the static list interfering
    static auto* libs = new std::vector<Implementation>([] () {
        std::vector<Implementation> fres;

        auto search_in_directory = [&](const std::string& paths) {
            std::stringstream ss(paths);
            std::string path;
            // Split the paths string by the delimiter and process each path.
            while (std::getline(ss, path, ';')) {
                std::filesystem::path fs_path(path);
                // Iterate over all libraries
                for (const auto& f : std::filesystem::directory_iterator(fs_path)) {
                    const std::filesystem::path& p = f.path();
                    if (p.extension() != LIB_FILE_EXT) continue;
                    // Add to list if model implementation
                    try {
                        Dlhandle dl(p.string());
                        if (!Implementation::isImplementation(dl)) {
                            continue;
                        }
                        fres.emplace_back(Implementation(std::move(dl)));
                    } catch (...) {}
                }
            }
        };

        search_in_directory(s_implementations_search_path);

        return fres;
    }());
    // Return static result
    return *libs;
}

const LLModel::Implementation* LLModel::Implementation::implementation(std::ifstream& f, const std::string& buildVariant) {
    for (const auto& i : implementationList()) {
        f.seekg(0);
        if (!i.m_magicMatch(f)) continue;
        if (buildVariant != i.m_buildVariant) continue;
        return &i;
    }
    return nullptr;
}

LLModel *LLModel::Implementation::construct(const std::string &modelPath, std::string buildVariant) {

    if (!has_at_least_minimal_hardware())
        return nullptr;

    // Read magic
    std::ifstream f(modelPath, std::ios::binary);
    if (!f) return nullptr;
    // Get correct implementation
    const Implementation* impl = nullptr;

    #if defined(__APPLE__) && defined(__arm64__) // FIXME: See if metal works for intel macs
        if (buildVariant == "auto") {
            size_t total_mem = getSystemTotalRAMInBytes();
            impl = implementation(f, "metal");
            if(impl) {
                LLModel* metalimpl = impl->m_construct();
                metalimpl->m_implementation = impl;
                size_t req_mem = metalimpl->requiredMem(modelPath);
                float req_to_total = (float) req_mem / (float) total_mem;
                // on a 16GB M2 Mac a 13B q4_0 (0.52) works for me but a 13B q4_K_M (0.55) does not
                if (req_to_total >= 0.53) {
                    delete metalimpl;
                    impl = nullptr;
                } else {
                    return metalimpl;
                }
            }
        }
    #endif

    if (!impl) {
        //TODO: Auto-detect CUDA/OpenCL
        if (buildVariant == "auto") {
            if (requires_avxonly()) {
                buildVariant = "avxonly";
            } else {
                buildVariant = "default";
            }
        }
        impl = implementation(f, buildVariant);
        if (!impl) return nullptr;
    }
    f.close();

    // Construct and return llmodel implementation
    auto fres = impl->m_construct();
    fres->m_implementation = impl;
    return fres;
}

void LLModel::Implementation::setImplementationsSearchPath(const std::string& path) {
    s_implementations_search_path = path;
}

const std::string& LLModel::Implementation::implementationsSearchPath() {
    return s_implementations_search_path;
}
