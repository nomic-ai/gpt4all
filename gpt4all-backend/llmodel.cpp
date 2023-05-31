#include "llmodel.h"
#include "dlhandle.h"

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cassert>



static
bool requires_avxonly() {
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


LLModel::Implementation::Implementation(Dlhandle &&dlhandle_) : dlhandle(std::move(dlhandle_)) {
    auto get_model_type = dlhandle.get<const char *()>("get_model_type");
    assert(get_model_type);
    modelType = get_model_type();
    auto get_build_variant = dlhandle.get<const char *()>("get_build_variant");
    assert(get_build_variant);
    buildVariant = get_build_variant();
    magicMatch = dlhandle.get<bool(std::ifstream&)>("magic_match");
    assert(magicMatch);
    construct_ = dlhandle.get<LLModel *()>("construct");
    assert(construct_);
}

bool LLModel::Implementation::isImplementation(const Dlhandle &dl) {
    return dl.get<bool(uint32_t)>("is_g4a_backend_model_implementation");
}


const std::vector<LLModel::Implementation> &LLModel::getImplementationList() {
    static auto* libs = new std::vector<LLModel::Implementation>([] () {
        std::vector<LLModel::Implementation> fres;

        auto search_in_directory = [&](const std::filesystem::path& path) {
            // Iterate over all libraries
            for (const auto& f : std::filesystem::directory_iterator(path)) {
                // Get path
                const auto& p = f.path();
                // Check extension
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
        };

        search_in_directory(".");
#if defined(__APPLE__)
        search_in_directory("../../../");
#endif
        return fres;
    }());
    // Return static result
    return *libs;
}

const LLModel::Implementation* LLModel::getImplementation(std::ifstream& f, const std::string& buildVariant) {
    // Iterate over all libraries
    for (const auto& i : getImplementationList()) {
        f.seekg(0);
        // Check that magic matches
        if (!i.magicMatch(f)) {
            continue;
        }
        // Check that build variant is correct
        if (buildVariant != i.buildVariant) {
            continue;
        }
        // Looks like we're good to go, return this dlhandle
        return &i;
    }
    // Nothing found, so return nothing
    return nullptr;
}

LLModel *LLModel::construct(const std::string &modelPath, std::string buildVariant) {
    //TODO: Auto-detect
    if (buildVariant == "auto") {
        if (requires_avxonly()) {
            buildVariant = "avxonly";
        } else {
            buildVariant = "default";
        }
    }
    // Read magic
    std::ifstream f(modelPath, std::ios::binary);
    if (!f) return nullptr;
    // Get correct implementation
    auto impl = getImplementation(f, buildVariant);
    if (!impl) return nullptr;
    f.close();
    // Construct and return llmodel implementation
    return impl->construct();
}
