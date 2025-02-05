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
#include <sstream>
#include <string>
#include <string_view>
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

using namespace std::string_literals;
using namespace std::string_view_literals;
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

LLModel::Implementation::Implementation(std::string buildBackend, Dlhandle &&dlhandle)
    : m_buildBackend(std::move(buildBackend))
    , m_dlhandle(new Dlhandle(std::move(dlhandle)))
{
    auto get_model_type = m_dlhandle->get<const char *()>("get_model_type");
    assert(get_model_type);
    m_getFileArch = m_dlhandle->get<char *(const char *)>("get_file_arch");
    assert(m_getFileArch);
    m_isArchSupported = m_dlhandle->get<bool(const char *)>("is_arch_supported");
    assert(m_isArchSupported);
    m_construct = m_dlhandle->get<LLModel *()>("construct");
    assert(m_construct);

    m_modelType = get_model_type();
}

LLModel::Implementation::Implementation(Implementation &&o)
    : m_buildBackend(o.m_buildBackend)
    , m_dlhandle(o.m_dlhandle)
    , m_getFileArch(o.m_getFileArch)
    , m_isArchSupported(o.m_isArchSupported)
    , m_construct(o.m_construct)
    , m_modelType(o.m_modelType)
{
    o.m_dlhandle = nullptr;
}

LLModel::Implementation::~Implementation()
{
    delete m_dlhandle;
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

auto LLModel::LazyImplementation::get() -> const Implementation &
{
    if (!impl) impl.emplace(buildBackend, Dlhandle(path));
    return *impl;
}

auto LLModel::getImplementations() -> std::vector<LazyImplementation> &
{
    // in no particular order
    static const std::array ALL_BUILD_BACKENDS { "cpu"sv, "metal"sv, "kompute"sv, "vulkan"sv, "cuda"sv };
    static const std::string_view LIB_EXT(LIB_FILE_EXT);

    if (cpu_supports_avx() == 0) {
        throw std::runtime_error("CPU does not support AVX");
    }

    // NOTE: allocated on heap so we leak intentionally on exit so we have a chance to clean up the
    // individual models without the cleanup of the static list interfering
    static auto* libs = new std::vector<LazyImplementation>([] () {
        std::vector<LazyImplementation> fres;

        addCudaSearchPath();

        bool avxonly = cpu_supports_avx2() == 0;
        std::stringstream ss(s_implementations_search_path);
        std::string piece;
        // Split the paths string by the delimiter and process each path.
        while (std::getline(ss, piece, ';')) {
            auto basePath = fs::path(std::u8string(piece.begin(), piece.end()));
            // Iterate over all libraries
            for (auto &buildBackend : ALL_BUILD_BACKENDS) {
                auto path = basePath /
                    "llamamodel-mainline-"s.append(buildBackend).append(avxonly ? "-avxonly" : "").append(LIB_EXT);
                if (fs::exists(path))
                    fres.push_back(LazyImplementation { std::string(buildBackend), path });
            }
        }

        return fres;
    }());
    // Return static result
    return *libs;
}

auto LLModel::Implementation::findImplementation(const char *fname, const std::string &buildBackend)
    -> const Implementation *
{
    bool buildBackendMatched = false;
    std::optional<std::string> archName;
    for (auto &li : getImplementations()) {
        if (li.buildBackend != buildBackend) continue;
        buildBackendMatched = true;

        auto &i = li.get();
        char *arch = i.m_getFileArch(fname);
        if (!arch) continue;
        archName = arch;

        bool archSupported = i.m_isArchSupported(arch);
        free(arch);
        if (archSupported) return &i;
    }

    if (!buildBackendMatched)
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
        const auto *impl = findImplementation(modelPath.c_str(), desiredBackend);

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

    std::vector<LazyImplementation> *impls;
    try {
        impls = &getImplementations();
    } catch (const std::runtime_error &e) {
        std::cerr << __func__ << ": getImplementations() failed: " << e.what() << "\n";
        return nullptr;
    }

    std::vector<std::string> desiredBackends;
    if (backend) {
        desiredBackends.push_back(backend.value());
    } else {
        desiredBackends.insert(desiredBackends.end(), DEFAULT_BACKENDS, std::end(DEFAULT_BACKENDS));
    }

    const Implementation *impl = nullptr;

    for (const auto &desiredBackend : desiredBackends) {
        auto cacheIt = implCache.find(desiredBackend);
        if (cacheIt != implCache.end())
            return cacheIt->second.get(); // cached

        for (auto &li : *impls) {
            if (li.buildBackend == desiredBackend) {
                auto &i = li.get();
                assert(i.m_modelType == "LLaMA");
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
