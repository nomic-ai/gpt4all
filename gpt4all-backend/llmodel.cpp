#include "llmodel.h"
#include "dlhandle.h"

#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <filesystem>



static
bool requires_avxonly() {
#ifdef __x86_64__
    return !__builtin_cpu_supports("avx2") && !__builtin_cpu_supports("fma");
#else
    return false;  // Don't know how to handle ARM
#endif
}


std::vector<Dlhandle> LLModel::implementations = {{}};

void LLModel::loadImplementationList() {
    // Collect all model implementation libraries if that hasn't been done yet
    if (!implementations[0].is_valid()) {
        implementations.clear();
        // Iterate over all libraries
        for (const auto& f : std::filesystem::directory_iterator(".")) {
            // Get path
            const auto& p = f.path();
            // Check extension
            if (p.extension() != LIB_FILE_EXT) continue;
            // Add to list if model implementation
            try {
                Dlhandle dl(p);
                if (dl.get<bool(uint32_t)>("is_g4a_backend_model_implementation")) {
                    implementations.emplace_back(std::move(dl));
                }
            } catch (...) {}
        }
    }
}

Dlhandle *LLModel::getImplementation(std::ifstream& f, const std::string& buildVariant) {
    loadImplementationList();
    // Iterate over all libraries
    for (auto& dl : implementations) {
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

std::vector<std::string_view> LLModel::getBuildVariantList() {
    std::vector<std::string_view> fres;
    loadImplementationList();
    // Collect all build variants
    for (auto& dl : implementations) {
        // Get build variant getter
        auto get_build_variant = dl.get<const char *()>("get_build_variant");
        if (!get_build_variant) continue;
        // Get build variant
        std::string_view build_variant = get_build_variant();
        // Make sure it's not listed yet
        if (std::find(fres.begin(), fres.end(), build_variant) != fres.end())
            continue;
        // Add it to list
        fres.push_back(build_variant);
    }
    // Return final list
    return fres;
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
    // Get inference constructor
    auto constructor = impl->get<LLModel *()>("construct");
    if (!constructor) return nullptr;
    // Construct llmodel implementation
    auto fres = constructor();
    // Return final instance
    return fres;
}
