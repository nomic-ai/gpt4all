#pragma once

#include "llamacpp_backend.h"

#include <optional>
#include <string>
#include <string_view>

class Dlhandle;


class LlamaCppBackendManager {
public:
    class BadArchError : public std::runtime_error {
    public:
        BadArchError(std::string arch)
            : runtime_error("Unsupported model architecture: " + arch)
            , m_arch(std::move(arch))
            {}

        const std::string &arch() const noexcept { return m_arch; }

    private:
        std::string m_arch;
    };

    class MissingImplementationError : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };

    class UnsupportedModelError : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };

    LlamaCppBackendManager(const LlamaCppBackendManager &) = delete;
    LlamaCppBackendManager(LlamaCppBackendManager &&);
    ~LlamaCppBackendManager();

    std::string_view modelType() const { return m_modelType; }
    std::string_view buildVariant() const { return m_buildVariant; }

    static LlamaCppBackend *construct(const std::string &modelPath, const std::string &backend = "auto", int n_ctx = 2048);
    static std::vector<LlamaCppBackend::GPUDevice> availableGPUDevices(size_t memoryRequired = 0);
    static int32_t maxContextLength(const std::string &modelPath);
    static int32_t layerCount(const std::string &modelPath);
    static bool isEmbeddingModel(const std::string &modelPath);
    static void setImplementationsSearchPath(const std::string &path);
    static const std::string &implementationsSearchPath();
    static bool hasSupportedCPU();
    // 0 for no, 1 for yes, -1 for non-x86_64
    static int cpuSupportsAVX2();

private:
    LlamaCppBackendManager(Dlhandle &&);

    static const std::vector<LlamaCppBackendManager> &implementationList();
    static const LlamaCppBackendManager *implementation(const char *fname, const std::string &buildVariant);
    static LlamaCppBackend *constructGlobalLlama(const std::optional<std::string> &backend = std::nullopt);

    char *(*m_getFileArch)(const char *fname);
    bool (*m_isArchSupported)(const char *arch);
    LlamaCppBackend *(*m_construct)();

    std::string_view m_modelType;
    std::string_view m_buildVariant;
    Dlhandle *m_dlhandle;
};
