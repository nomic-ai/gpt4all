#include "llmodel.h"
#include "dlhandle.h"
#include "sysinfo.h"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#ifdef _MSC_VER
#include <intrin.h>
#endif

std::string s_implementations_search_path = ".";

#if !(defined(__x86_64__) || defined(_M_X64))
    // irrelevant on non-x86_64
    #define cpu_supports_avx()  -1
    #define cpu_supports_avx2() -1
#elif defined(_MSC_VER)
    // MSVC
    static int get_cpu_info(int func_id, int reg_id) {
        int info[4];
        __cpuid(info, func_id);
        return info[reg_id];
    }

    // AVX via EAX=1: Processor Info and Feature Bits, bit 28 of ECX
    #define cpu_supports_avx()  (get_cpu_info(1, 2) & (1 << 28))
    // AVX2 via EAX=7, ECX=0: Extended Features, bit 5 of EBX
    #define cpu_supports_avx2() (get_cpu_info(7, 1) & (1 <<  5))
#else
    // gcc/clang
    #define cpu_supports_avx()  __builtin_cpu_supports("avx")
    #define cpu_supports_avx2() __builtin_cpu_supports("avx2")
#endif

LLModel::Implementation::Implementation(Dlhandle &&dlhandle_)
    : m_dlhandle(new Dlhandle(std::move(dlhandle_))) {
    auto get_model_type = m_dlhandle->get<const char *()>("get_model_type");
    assert(get_model_type);
    m_modelType = get_model_type();
    auto get_build_variant = m_dlhandle->get<const char *()>("get_build_variant");
    assert(get_build_variant);
    m_buildVariant = get_build_variant();
    m_magicMatch = m_dlhandle->get<bool(const char*)>("magic_match");
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
    delete m_dlhandle;
}

static bool isImplementation(const Dlhandle &dl) {
    return dl.get<bool(uint32_t)>("is_g4a_backend_model_implementation");
}

const std::vector<LLModel::Implementation> &LLModel::Implementation::implementationList() {
    if (cpu_supports_avx() == 0) {
        throw std::runtime_error("CPU does not support AVX");
    }

    // NOTE: allocated on heap so we leak intentionally on exit so we have a chance to clean up the
    // individual models without the cleanup of the static list interfering
    static auto* libs = new std::vector<Implementation>([] () {
        std::vector<Implementation> fres;

        std::string impl_name_re = "(gptj|llamamodel-mainline)";
        if (cpu_supports_avx2() == 0) {
            impl_name_re += "-avxonly";
        } else {
            impl_name_re += "-(default|metal)";
        }
        std::regex re(impl_name_re);
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
                    if (!std::regex_search(p.stem().string(), re)) continue;

                    // Add to list if model implementation
                    try {
                        Dlhandle dl(p.string());
                        if (!isImplementation(dl))
                            continue;
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

const LLModel::Implementation* LLModel::Implementation::implementation(const char *fname, const std::string& buildVariant) {
    bool buildVariantMatched = false;
    for (const auto& i : implementationList()) {
        if (buildVariant != i.m_buildVariant) continue;
        buildVariantMatched = true;

        if (!i.m_magicMatch(fname)) continue;
        return &i;
    }

    if (!buildVariantMatched)
        throw std::runtime_error("Could not find any implementations for build variant: " + buildVariant);

    return nullptr; // unsupported model format
}

LLModel *LLModel::Implementation::construct(const std::string &modelPath, std::string buildVariant, int n_ctx) {
    // Get correct implementation
    const Implementation* impl = nullptr;

    #if defined(__APPLE__) && defined(__arm64__) // FIXME: See if metal works for intel macs
        if (buildVariant == "auto") {
            size_t total_mem = getSystemTotalRAMInBytes();
            impl = implementation(modelPath.c_str(), "metal");
            if(impl) {
                LLModel* metalimpl = impl->m_construct();
                metalimpl->m_implementation = impl;
                /* TODO(cebtenzzre): after we fix requiredMem, we should change this to happen at
                 * load time, not construct time. right now n_ctx is incorrectly hardcoded 2048 in
                 * most (all?) places where this is called, causing underestimation of required
                 * memory. */
                size_t req_mem = metalimpl->requiredMem(modelPath, n_ctx, 100);
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
    #else
        (void)n_ctx;
    #endif

    if (!impl) {
        //TODO: Auto-detect CUDA/OpenCL
        if (buildVariant == "auto") {
            if (cpu_supports_avx2() == 0) {
                buildVariant = "avxonly";
            } else {
                buildVariant = "default";
            }
        }
        impl = implementation(modelPath.c_str(), buildVariant);
        if (!impl) return nullptr;
    }

    // Construct and return llmodel implementation
    auto fres = impl->m_construct();
    fres->m_implementation = impl;
    return fres;
}

LLModel *LLModel::Implementation::constructDefaultLlama() {
    static std::unique_ptr<LLModel> llama([]() -> LLModel * {
        const std::vector<LLModel::Implementation> *impls;
        try {
            impls = &implementationList();
        } catch (const std::runtime_error &e) {
            std::cerr << __func__ << ": implementationList failed: " << e.what() << "\n";
            return nullptr;
        }

        const LLModel::Implementation *impl = nullptr;
        for (const auto &i: *impls) {
            if (i.m_buildVariant == "metal" || i.m_modelType != "LLaMA") continue;
            impl = &i;
        }
        if (!impl) {
            std::cerr << __func__ << ": could not find llama.cpp implementation\n";
            return nullptr;
        }

        auto fres = impl->m_construct();
        fres->m_implementation = impl;
        return fres;
    }());
    return llama.get();
}

std::vector<LLModel::GPUDevice> LLModel::Implementation::availableGPUDevices() {
    auto *llama = constructDefaultLlama();
    if (llama) { return llama->availableGPUDevices(0); }
    return {};
}

int32_t LLModel::Implementation::maxContextLength(const std::string &modelPath) {
    auto *llama = constructDefaultLlama();
    return llama ? llama->maxContextLength(modelPath) : -1;
}

int32_t LLModel::Implementation::layerCount(const std::string &modelPath) {
    auto *llama = constructDefaultLlama();
    return llama ? llama->layerCount(modelPath) : -1;
}

bool LLModel::Implementation::isEmbeddingModel(const std::string &modelPath) {
    auto *llama = constructDefaultLlama();
    return llama && llama->isEmbeddingModel(modelPath);
}

void LLModel::Implementation::setImplementationsSearchPath(const std::string& path) {
    s_implementations_search_path = path;
}

const std::string& LLModel::Implementation::implementationsSearchPath() {
    return s_implementations_search_path;
}

bool LLModel::Implementation::hasSupportedCPU() {
    return cpu_supports_avx() != 0;
}
