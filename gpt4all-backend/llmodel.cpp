#include "llmodel.h"
#include "dlhandle.h"

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>



static
Dlhandle *get_implementation(uint32_t magic, const std::string& buildVariant) {
    // Collect all model implementation libraries
    static auto libs = [] () {
        std::vector<Dlhandle> fres;
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
                    fres.emplace_back(std::move(dl));
                }
            } catch (...) {}
        }
        return fres;
    }();
    // Iterate over all libraries
    for (auto& dl : libs) {
        // Check that magic matches
        auto magic_match = dl.get<bool(uint32_t)>("magic_match");
        if (!magic_match || !magic_match(magic)) {
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
        buildVariant = "default";
    }
    // Read magic
    std::ifstream f(modelPath, std::ios::binary);
    uint32_t magic;
    if (!f.read(reinterpret_cast<char*>(&magic), sizeof(magic))) {
        return nullptr;
    }
    f.close();
    // Get correct implementation
    auto impl = get_implementation(magic, buildVariant);
    if (!impl) return nullptr;
    // Get inference constructor
    auto constructor = impl->get<LLModel *()>("construct");
    if (!constructor) return nullptr;
    // Construct llmodel implementation
    auto fres = constructor();
    // Return final instance
    return fres;
}
