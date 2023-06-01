#include "llmodel.h"
#include "dlhandle.h"

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>



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


static Dlhandle *get_implementation(std::ifstream& f, const std::string& buildVariant) {
    // Collect all model implementation libraries
    // NOTE: allocated on heap so we leak intentionally on exit so we have a chance to clean up the
    // individual models without the cleanup of the static list interfering
    static auto* libs = new std::vector<Dlhandle>([] () {
        std::vector<Dlhandle> fres;

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
                    if (dl.get<bool(uint32_t)>("is_g4a_backend_model_implementation")) {
                        fres.emplace_back(std::move(dl));
                    }
                } catch (...) {}
            }
        };

        search_in_directory(".");
#if defined(__APPLE__)
        search_in_directory("../../../");
#endif
        return fres;
    }());
    // Iterate over all libraries
    for (auto& dl : *libs) {
        f.seekg(0);
        // Check that magic matches
        auto magic_match = dl.get<bool(std::ifstream&)>("magic_match");
        if (!magic_match || !magic_match(f)) {
            continue;
        }
        // Check that build variant is correct
        auto get_build_variant = dl.get<const char *()>("get_build_variant");
        if (buildVariant != (get_build_variant?get_build_variant():"default")) {
            continue;
        }
        // Looks like we're good to go, return this dlhandle
        return &dl;
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
    auto impl = get_implementation(f, buildVariant);
    if (!impl) return nullptr;
    f.close();
    // Get inference constructor
    auto constructor = impl->get<LLModel *()>("construct");
    if (!constructor) return nullptr;
    // Construct llmodel implementation
    auto fres = constructor();
    // Return final instance
    return fres;
}
