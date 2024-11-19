#include "llmodel.h"

#include "dlhandle.h"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
#   include <windows.h>
#endif

#ifdef _MSC_VER
#   include <intrin.h>
#endif

#if defined(__APPLE__) && defined(__aarch64__)
#   include "sysinfo.h" // for getSystemTotalRAMInBytes
#endif

namespace fs = std::filesystem;

#ifndef __APPLE__
static const std::string DEFAULT_BACKENDS[] = {"kompute", "cpu"};
#elif defined(__aarch64__)
static const std::string DEFAULT_BACKENDS[] = {"metal", "cpu"};
#else
static const std::string DEFAULT_BACKENDS[] = {"cpu"};
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
    #define cpu_supports_avx()  !!(get_cpu_info(1, 2) & (1 << 28))
    // AVX2 via EAX=7, ECX=0: Extended Features, bit 5 of EBX
    #define cpu_supports_avx2() !!(get_cpu_info(7, 1) & (1 <<  5))
#else
    // gcc/clang
    #define cpu_supports_avx()  !!__builtin_cpu_supports("avx")
    #define cpu_supports_avx2() !!__builtin_cpu_supports("avx2")
#endif

LLModel::Implementation::Implementation(Dlhandle &&dlhandle_)
    : m_dlhandle(new Dlhandle(std::move(dlhandle_))) {
    auto get_model_type = m_dlhandle->get<const char *()>("get_model_type");
    assert(get_model_type);
    m_modelType = get_model_type();
    auto get_build_variant = m_dlhandle->get<const char *()>("get_build_variant");
    assert(get_build_variant);
    m_buildVariant = get_build_variant();
    m_getFileArch = m_dlhandle->get<char *(const char *)>("get_file_arch");
    assert(m_getFileArch);
    m_isArchSupported = m_dlhandle->get<bool(const char *)>("is_arch_supported");
    assert(m_isArchSupported);
    m_construct = m_dlhandle->get<LLModel *()>("construct");
    assert(m_construct);
}

LLModel::Implementation::Implementation(Implementation &&o)
    : m_getFileArch(o.m_getFileArch)
    , m_isArchSupported(o.m_isArchSupported)
    , m_construct(o.m_construct)
    , m_modelType(o.m_modelType)
    , m_buildVariant(o.m_buildVariant)
    , m_dlhandle(o.m_dlhandle) {
    o.m_dlhandle = nullptr;
}

LLModel::Implementation::~Implementation()
{
    delete m_dlhandle;
}

static bool isImplementation(const Dlhandle &dl)
{
    return dl.get<bool(uint32_t)>("is_g4a_backend_model_implementation");
}

// Add the CUDA Toolkit to the DLL search path on Windows.
// This is necessary for chat.exe to find CUDA when started from Qt Creator.
static void addCudaSearchPath()
{
#ifdef _WIN32
    if (const auto *cudaPath = _wgetenv(L"CUDA_PATH")) {
        auto libDir = std::wstring(cudaPath) + L"\\bin";
        if (!AddDllDirectory(libDir.c_str())) {
            auto err = GetLastError();
            std::wcerr << L"AddDllDirectory(\"" << libDir << L"\") failed with error 0x" << std::hex << err << L"\n";
        }
    }
#endif
}

const std::vector<LLModel::Implementation> &LLModel::Implementation::implementationList()
{
    if (cpu_supports_avx() == 0) {
        throw std::runtime_error("CPU does not support AVX");
    }

    // NOTE: allocated on heap so we leak intentionally on exit so we have a chance to clean up the
    // individual models without the cleanup of the static list interfering
    static auto* libs = new std::vector<Implementation>([] () {
        std::vector<Implementation> fres;

        addCudaSearchPath();

        std::string impl_name_re = "llamamodel-mainline-(cpu|metal|kompute|vulkan|cuda)";
        if (cpu_supports_avx2() == 0) {
            impl_name_re += "-avxonly";
        }
        std::regex re(impl_name_re);
        auto search_in_directory = [&](const std::string& paths) {
            std::stringstream ss(paths);
            std::string path;
            // Split the paths string by the delimiter and process each path.
            while (std::getline(ss, path, ';')) {
                std::u8string u8_path(path.begin(), path.end());
                // Iterate over all libraries
                for (const auto &f : fs::directory_iterator(u8_path)) {
                    const fs::path &p = f.path();

                    if (p.extension() != LIB_FILE_EXT) continue;
                    if (!std::regex_search(p.stem().string(), re)) continue;

                    // Add to list if model implementation
                    Dlhandle dl;
                    try {
                        dl = Dlhandle(p);
                    } catch (const Dlhandle::Exception &e) {
                        std::cerr << "Failed to load " << p.filename().string() << ": " << e.what() << "\n";
                        continue;
                    }
                    if (!isImplementation(dl)) {
                        std::cerr << "Not an implementation: " << p.filename().string() << "\n";
                        continue;
                    }
                    fres.emplace_back(Implementation(std::move(dl)));
                }
            }
        };

        search_in_directory(s_implementations_search_path);

        return fres;
    }());
    // Return static result
    return *libs;
}

static std::string applyCPUVariant(const std::string &buildVariant)
{
    if (buildVariant != "metal" && cpu_supports_avx2() == 0) {
        return buildVariant + "-avxonly";
    }
    return buildVariant;
}

const LLModel::Implementation* LLModel::Implementation::implementation(const char *fname, const std::string& buildVariant)
{
    bool buildVariantMatched = false;
    std::optional<std::string> archName;
    for (const auto& i : implementationList()) {
        if (buildVariant != i.m_buildVariant) continue;
        buildVariantMatched = true;

        char *arch = i.m_getFileArch(fname);
        if (!arch) continue;
        archName = arch;

        bool archSupported = i.m_isArchSupported(arch);
        free(arch);
        if (archSupported) return &i;
    }

    if (!buildVariantMatched)
        return nullptr;
    if (!archName)
        throw UnsupportedModelError("Unsupported file format");

    throw BadArchError(std::move(*archName));
}

LLModel *LLModel::Implementation::construct(const std::string &modelPath, const std::string &backend, int n_ctx)
{
    std::vector<std::string> desiredBackends;
    if (backend != "auto") {
        desiredBackends.push_back(backend);
    } else {
        desiredBackends.insert(desiredBackends.end(), DEFAULT_BACKENDS, std::end(DEFAULT_BACKENDS));
    }

    for (const auto &desiredBackend: desiredBackends) {
        const auto *impl = implementation(modelPath.c_str(), applyCPUVariant(desiredBackend));

        if (impl) {
            // Construct llmodel implementation
            auto *fres = impl->m_construct();
            fres->m_implementation = impl;

#if defined(__APPLE__) && defined(__aarch64__) // FIXME: See if metal works for intel macs
            /* TODO(cebtenzzre): after we fix requiredMem, we should change this to happen at
             * load time, not construct time. right now n_ctx is incorrectly hardcoded 2048 in
             * most (all?) places where this is called, causing underestimation of required
             * memory. */
            if (backend == "auto" && desiredBackend == "metal") {
                // on a 16GB M2 Mac a 13B q4_0 (0.52) works for me but a 13B q4_K_M (0.55) does not
                size_t req_mem = fres->requiredMem(modelPath, n_ctx, 100);
                if (req_mem >= size_t(0.53f * getSystemTotalRAMInBytes())) {
                    delete fres;
                    continue;
                }
            }
#else
            (void)n_ctx;
#endif

            return fres;
        }
    }

    throw MissingImplementationError("Could not find any implementations for backend: " + backend);
}

LLModel *LLModel::Implementation::constructGlobalLlama(const std::optional<std::string> &backend)
{
    static std::unordered_map<std::string, std::unique_ptr<LLModel>> implCache;

    const std::vector<Implementation> *impls;
    try {
        impls = &implementationList();
    } catch (const std::runtime_error &e) {
        std::cerr << __func__ << ": implementationList failed: " << e.what() << "\n";
        return nullptr;
    }

    std::vector<std::string> desiredBackends;
    if (backend) {
        desiredBackends.push_back(backend.value());
    } else {
        desiredBackends.insert(desiredBackends.end(), DEFAULT_BACKENDS, std::end(DEFAULT_BACKENDS));
    }

    const Implementation *impl = nullptr;

    for (const auto &desiredBackend: desiredBackends) {
        auto cacheIt = implCache.find(desiredBackend);
        if (cacheIt != implCache.end())
            return cacheIt->second.get(); // cached

        for (const auto &i: *impls) {
            if (i.m_modelType == "LLaMA" && i.m_buildVariant == applyCPUVariant(desiredBackend)) {
                impl = &i;
                break;
            }
        }

        if (impl) {
            auto *fres = impl->m_construct();
            fres->m_implementation = impl;
            implCache[desiredBackend] = std::unique_ptr<LLModel>(fres);
            return fres;
        }
    }

    std::cerr << __func__ << ": could not find Llama implementation for backend: " << backend.value_or("default") << "\n";
    return nullptr;
}

std::vector<LLModel::GPUDevice> LLModel::Implementation::availableGPUDevices(size_t memoryRequired)
{
    std::vector<LLModel::GPUDevice> devices;
#ifndef __APPLE__
    static const std::string backends[] = {"kompute", "cuda"};
    for (const auto &backend: backends) {
        auto *llama = constructGlobalLlama(backend);
        if (llama) {
            auto backendDevs = llama->availableGPUDevices(memoryRequired);
            devices.insert(devices.end(), backendDevs.begin(), backendDevs.end());
        }
    }
#endif
    return devices;
}

int32_t LLModel::Implementation::maxContextLength(const std::string &modelPath)
{
    auto *llama = constructGlobalLlama();
    return llama ? llama->maxContextLength(modelPath) : -1;
}

int32_t LLModel::Implementation::layerCount(const std::string &modelPath)
{
    auto *llama = constructGlobalLlama();
    return llama ? llama->layerCount(modelPath) : -1;
}

bool LLModel::Implementation::isEmbeddingModel(const std::string &modelPath)
{
    auto *llama = constructGlobalLlama();
    return llama && llama->isEmbeddingModel(modelPath);
}

auto LLModel::Implementation::chatTemplate(const char *modelPath) -> std::expected<std::string, std::string>
{
    auto *llama = constructGlobalLlama();
    return llama ? llama->chatTemplate(modelPath) : std::unexpected("backend not available");
}

void LLModel::Implementation::setImplementationsSearchPath(const std::string& path)
{
    s_implementations_search_path = path;
}

const std::string& LLModel::Implementation::implementationsSearchPath()
{
    return s_implementations_search_path;
}

bool LLModel::Implementation::hasSupportedCPU()
{
    return cpu_supports_avx() != 0;
}

int LLModel::Implementation::cpuSupportsAVX2()
{
    return cpu_supports_avx2();
}
