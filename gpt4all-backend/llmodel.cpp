#include "llmodel.h"
#include "dlhandle.h"

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cassert>
#include <cstdlib>
#include <sstream>

std::string s_implementations_search_path = ".";

static bool has_at_least_minimal_hardware() {
#ifdef __x86_64__
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
#ifdef __x86_64__
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

LLModel::Implementation::Implementation(Dlhandle &&dlhandle_) : dlhandle(new Dlhandle(std::move(dlhandle_))) {
    auto get_model_type = dlhandle->get<const char *()>("get_model_type");
    assert(get_model_type);
    modelType = get_model_type();
    auto get_build_variant = dlhandle->get<const char *()>("get_build_variant");
    assert(get_build_variant);
    buildVariant = get_build_variant();
    magicMatch = dlhandle->get<bool(std::ifstream&)>("magic_match");
    assert(magicMatch);
    construct_ = dlhandle->get<LLModel *()>("construct");
    assert(construct_);
}

LLModel::Implementation::Implementation(Implementation &&o)
    : construct_(o.construct_)
    , modelType(o.modelType)
    , buildVariant(o.buildVariant)
    , magicMatch(o.magicMatch)
    , dlhandle(o.dlhandle) {
    o.dlhandle = nullptr;
}

LLModel::Implementation::~Implementation() {
    if (dlhandle) delete dlhandle;
}

bool LLModel::Implementation::isImplementation(const Dlhandle &dl) {
    return dl.get<bool(uint32_t)>("is_g4a_backend_model_implementation");
}

const std::vector<LLModel::Implementation> &LLModel::implementationList() {
    // NOTE: allocated on heap so we leak intentionally on exit so we have a chance to clean up the
    // individual models without the cleanup of the static list interfering
    static auto* libs = new std::vector<LLModel::Implementation>([] () {
        std::vector<LLModel::Implementation> fres;

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

const LLModel::Implementation* LLModel::implementation(std::ifstream& f, const std::string& buildVariant) {
    for (const auto& i : implementationList()) {
        f.seekg(0);
        if (!i.magicMatch(f)) continue;
        if (buildVariant != i.buildVariant) continue;
        return &i;
    }
    return nullptr;
}

LLModel *LLModel::construct(const std::string &modelPath, std::string buildVariant) {

    if (!has_at_least_minimal_hardware())
        return nullptr;

    // Read magic
    std::ifstream f(modelPath, std::ios::binary);
    if (!f) return nullptr;
    // Get correct implementation
    const LLModel::Implementation* impl = nullptr;

    #if defined(__APPLE__) && defined(__arm64__) // FIXME: See if metal works for intel macs
        if (buildVariant == "auto") {
            impl = implementation(f, "metal");
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
    return impl->construct();
}

void LLModel::setImplementationsSearchPath(const std::string& path) {
    s_implementations_search_path = path;
}

const std::string& LLModel::implementationsSearchPath() {
    return s_implementations_search_path;
}
