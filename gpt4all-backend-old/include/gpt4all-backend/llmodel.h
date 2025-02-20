#ifndef LLMODEL_H
#define LLMODEL_H

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

class Dlhandle;

using namespace std::string_literals;

#define LLMODEL_MAX_PROMPT_BATCH 128

class LLModel {
public:
    using Token = int32_t;
    using PromptCallback      = std::function<bool(std::span<const Token> batch, bool cached)>;
    using ResponseCallback    = std::function<bool(Token token, std::string_view piece)>;
    using EmbedCancelCallback = bool(unsigned *batchSizes, unsigned nBatch, const char *backend);
    using ProgressCallback    = std::function<bool(float progress)>;

    class BadArchError: public std::runtime_error {
    public:
        BadArchError(std::string arch)
            : runtime_error("Unsupported model architecture: " + arch)
            , m_arch(std::move(arch))
            {}

        const std::string &arch() const noexcept { return m_arch; }

    private:
        std::string m_arch;
    };

    class MissingImplementationError: public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };

    class UnsupportedModelError: public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };

    struct GPUDevice {
        const char *backend;
        int index;
        int type;
        size_t heapSize;
        std::string name;
        std::string vendor;

        GPUDevice(const char *backend, int index, int type, size_t heapSize, std::string name, std::string vendor):
            backend(backend), index(index), type(type), heapSize(heapSize), name(std::move(name)),
            vendor(std::move(vendor)) {}

        std::string selectionName() const
        {
            assert(backend == "cuda"s || backend == "kompute"s);
            return backendName() + ": " + name;
        }

        std::string backendName() const { return backendIdToName(backend); }

        static std::string backendIdToName(const std::string &backend) { return s_backendNames.at(backend); }

        static std::string updateSelectionName(const std::string &name) {
            if (name == "Auto" || name == "CPU" || name == "Metal")
                return name;
            auto it = std::find_if(s_backendNames.begin(), s_backendNames.end(), [&name](const auto &entry) {
                return name.starts_with(entry.second + ": ");
            });
            if (it != s_backendNames.end())
                return name;
            return "Vulkan: " + name; // previously, there were only Vulkan devices
        }

    private:
        static inline const std::unordered_map<std::string, std::string> s_backendNames {
            {"cpu", "CPU"}, {"metal", "Metal"}, {"cuda", "CUDA"}, {"kompute", "Vulkan"},
        };
    };

    class Implementation {
    public:
        Implementation(const Implementation &) = delete;
        Implementation(Implementation &&);
        ~Implementation();

        std::string_view modelType() const { return m_modelType; }
        std::string_view buildVariant() const { return m_buildVariant; }

        static LLModel *construct(const std::string &modelPath, const std::string &backend = "auto", int n_ctx = 2048);
        static std::vector<GPUDevice> availableGPUDevices(size_t memoryRequired = 0);
        static int32_t maxContextLength(const std::string &modelPath);
        static int32_t layerCount(const std::string &modelPath);
        static bool isEmbeddingModel(const std::string &modelPath);
        static auto chatTemplate(const char *modelPath) -> std::expected<std::string, std::string>;
        static void setImplementationsSearchPath(const std::string &path);
        static const std::string &implementationsSearchPath();
        static bool hasSupportedCPU();
        // 0 for no, 1 for yes, -1 for non-x86_64
        static int cpuSupportsAVX2();

    private:
        Implementation(Dlhandle &&);

        static const std::vector<Implementation> &implementationList();
        static const Implementation *implementation(const char *fname, const std::string &buildVariant);
        static LLModel *constructGlobalLlama(const std::optional<std::string> &backend = std::nullopt);

        char *(*m_getFileArch)(const char *fname);
        bool (*m_isArchSupported)(const char *arch);
        LLModel *(*m_construct)();

        std::string_view m_modelType;
        std::string_view m_buildVariant;
        Dlhandle *m_dlhandle;
    };

    struct PromptContext {
        int32_t n_predict = 200;
        int32_t top_k = 40;
        float   top_p = 0.9f;
        float   min_p = 0.0f;
        float   temp = 0.9f;
        int32_t n_batch = 9;
        float   repeat_penalty = 1.10f;
        int32_t repeat_last_n = 64;     // last n tokens to penalize
        float   contextErase = 0.5f;    // percent of context to erase if we exceed the context window
    };

    explicit LLModel() {}
    virtual ~LLModel() {}

    virtual bool supportsEmbedding() const = 0;
    virtual bool supportsCompletion() const = 0;
    virtual bool loadModel(const std::string &modelPath, int n_ctx, int ngl) = 0;
    virtual bool isModelBlacklisted(const std::string &modelPath) const { (void)modelPath; return false; }
    virtual bool isEmbeddingModel(const std::string &modelPath) const { (void)modelPath; return false; }
    virtual bool isModelLoaded() const = 0;
    virtual size_t requiredMem(const std::string &modelPath, int n_ctx, int ngl) = 0;
    virtual size_t stateSize() const = 0;
    virtual size_t saveState(std::span<uint8_t> stateOut, std::vector<Token> &inputTokensOut) const = 0;
    virtual size_t restoreState(std::span<const uint8_t> state, std::span<const Token> inputTokens) = 0;

    // This method requires the model to return true from supportsCompletion otherwise it will throw
    // an error
    virtual void prompt(std::string_view        prompt,
                        const PromptCallback   &promptCallback,
                        const ResponseCallback &responseCallback,
                        const PromptContext    &ctx);

    virtual int32_t countPromptTokens(std::string_view prompt) const;

    virtual size_t embeddingSize() const {
        throw std::logic_error(std::string(implementation().modelType()) + " does not support embeddings");
    }
    // user-specified prefix
    virtual void embed(const std::vector<std::string> &texts, float *embeddings, std::optional<std::string> prefix,
                       int dimensionality = -1, size_t *tokenCount = nullptr, bool doMean = true, bool atlas = false,
                       EmbedCancelCallback *cancelCb = nullptr);
    // automatic prefix
    virtual void embed(const std::vector<std::string> &texts, float *embeddings, bool isRetrieval,
                       int dimensionality = -1, size_t *tokenCount = nullptr, bool doMean = true, bool atlas = false);

    virtual void setThreadCount(int32_t n_threads) { (void)n_threads; }
    virtual int32_t threadCount() const { return 1; }

    const Implementation &implementation() const {
        return *m_implementation;
    }

    virtual std::vector<GPUDevice> availableGPUDevices(size_t memoryRequired) const {
        (void)memoryRequired;
        return {};
    }

    virtual bool initializeGPUDevice(size_t memoryRequired, const std::string &name) const {
        (void)memoryRequired;
        (void)name;
        return false;
    }

    virtual bool initializeGPUDevice(int device, std::string *unavail_reason = nullptr) const {
        (void)device;
        if (unavail_reason) {
            *unavail_reason = "model has no GPU support";
        }
        return false;
    }

    virtual bool usingGPUDevice() const { return false; }
    virtual const char *backendName() const { return "cpu"; }
    virtual const char *gpuDeviceName() const { return nullptr; }

    void setProgressCallback(ProgressCallback callback) { m_progressCallback = callback; }

    virtual int32_t contextLength() const = 0;
    virtual auto specialTokens() -> std::unordered_map<std::string, std::string> const = 0;

protected:
    // These are pure virtual because subclasses need to implement as the default implementation of
    // 'prompt' above calls these functions
    virtual std::vector<Token> tokenize(std::string_view str) const = 0;
    virtual bool isSpecialToken(Token id) const = 0;
    virtual std::string tokenToString(Token id) const = 0;
    virtual void initSampler(const PromptContext &ctx) = 0;
    virtual Token sampleToken() const = 0;
    virtual bool evalTokens(int32_t nPast, std::span<const Token> tokens) const = 0;
    virtual void shiftContext(const PromptContext &promptCtx, int32_t *nPast) = 0;
    virtual int32_t inputLength() const = 0;
    virtual int32_t computeModelInputPosition(std::span<const Token> input) const = 0;
    virtual void setModelInputPosition(int32_t pos) = 0;
    virtual void appendInputToken(Token tok) = 0;
    virtual std::span<const Token> inputTokens() const = 0;
    virtual const std::vector<Token> &endTokens() const = 0;
    virtual bool shouldAddBOS() const = 0;

    virtual int32_t maxContextLength(std::string const &modelPath) const
    {
        (void)modelPath;
        return -1;
    }

    virtual int32_t layerCount(std::string const &modelPath) const
    {
        (void)modelPath;
        return -1;
    }

    virtual auto chatTemplate(const char *modelPath) const -> std::expected<std::string, std::string>
    {
        (void)modelPath;
        return std::unexpected("not implemented");
    }

    const Implementation *m_implementation = nullptr;

    ProgressCallback m_progressCallback;
    static bool staticProgressCallback(float progress, void* ctx)
    {
        LLModel* model = static_cast<LLModel*>(ctx);
        if (model && model->m_progressCallback)
            return model->m_progressCallback(progress);
        return true;
    }

    // prefill context with prompt
    auto decodePrompt(const PromptCallback &promptCallback,
                      const PromptContext  &promptCtx,
                      std::vector<Token>    embd_inp)
        -> std::optional<int32_t>;
    // generate a response
    void generateResponse(const ResponseCallback &responseCallback,
                          const PromptContext    &promptCtx,
                          int32_t                 nPast);

    friend class LLMImplementation;
};

#endif // LLMODEL_H
