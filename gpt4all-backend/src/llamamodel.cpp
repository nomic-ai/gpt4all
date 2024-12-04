#define LLAMAMODEL_H_I_KNOW_WHAT_I_AM_DOING_WHEN_INCLUDING_THIS_FILE
#include "llamamodel_impl.h"

#include "llmodel.h"
#include "utils.h"

#include <ggml.h>
#include <llama.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#ifdef GGML_USE_KOMPUTE
#   include <ggml-kompute.h>
#elif defined(GGML_USE_VULKAN)
#   include <ggml-vulkan.h>
#elif defined(GGML_USE_CUDA)
#   include <ggml-cuda.h>
#endif

using namespace std::string_literals;


// Maximum supported GGUF version
static constexpr int GGUF_VER_MAX = 3;

static const char * const modelType_ = "LLaMA";

// note: same order as LLM_ARCH_NAMES in llama.cpp
static const std::vector<const char *> KNOWN_ARCHES {
    "llama",
    "falcon",
    // "grok", -- 314B parameters
    "gpt2",
    // "gptj", -- no inference code
    "gptneox",
    "mpt",
    "baichuan",
    "starcoder",
    "refact",
    "bert",
    "nomic-bert",
    // "jina-bert-v2", -- Assertion `i01 >= 0 && i01 < ne01' failed.
    "bloom",
    "stablelm",
    "qwen",
    "qwen2",
    "qwen2moe",
    "phi2",
    "phi3",
    // "plamo", -- https://github.com/ggerganov/llama.cpp/issues/5669
    "codeshell",
    "orion",
    "internlm2",
    // "minicpm", -- CUDA generates garbage
    "gemma",
    "gemma2",
    "starcoder2",
    // "mamba", -- CUDA missing SSM_CONV
    "xverse",
    "command-r",
    // "dbrx", -- 16x12B parameters
    "olmo",
    "openelm",
    // "arctic", -- 10B+128x3.66B parameters
    "deepseek2",
    "chatglm",
    // "bitnet", -- tensor not within file bounds?
    // "t5", -- seq2seq model
    "jais",
};

static const std::vector<const char *> EMBEDDING_ARCHES {
    "bert", "nomic-bert",
};

static bool is_embedding_arch(const std::string &arch)
{
    return std::find(EMBEDDING_ARCHES.begin(), EMBEDDING_ARCHES.end(), arch) < EMBEDDING_ARCHES.end();
}

static bool llama_verbose()
{
    const char* var = getenv("GPT4ALL_VERBOSE_LLAMACPP");
    return var && *var;
}

static void llama_log_callback(ggml_log_level level, const char *text, void *userdata, bool warn)
{
    (void)userdata;

    static ggml_log_level lastlevel = GGML_LOG_LEVEL_NONE;
    if (!llama_verbose()) {
        auto efflevel = level == GGML_LOG_LEVEL_CONT ? lastlevel : level;
        lastlevel = efflevel;
        switch (efflevel) {
            case GGML_LOG_LEVEL_CONT:
                UNREACHABLE();
                break;
            case GGML_LOG_LEVEL_WARN:
                if (warn) break;
                [[fallthrough]];
            case GGML_LOG_LEVEL_NONE: // not used?
            case GGML_LOG_LEVEL_INFO:
            case GGML_LOG_LEVEL_DEBUG:
                return; // suppress
            case GGML_LOG_LEVEL_ERROR:
                ;
        }
    }

    fputs(text, stderr);
}

struct gpt_params {
    int32_t n_keep        = 0;    // number of tokens to keep from initial prompt

    // sampling parameters
    float   tfs_z         = 1.0f; // 1.0 = disabled
    float   typical_p     = 1.0f; // 1.0 = disabled

    std::string prompt = "";

    enum ggml_type kv_type = GGML_TYPE_F16; // use f16 instead of f32 for memory kv

    bool use_mmap          = true;  // use mmap for faster loads
    bool use_mlock         = false; // use mlock to keep model in memory
};

const char *get_arch_name(gguf_context *ctx_gguf)
{
    const int kid = gguf_find_key(ctx_gguf, "general.architecture");
    if (kid == -1)
        throw std::runtime_error("key not found in model: general.architecture");

    enum gguf_type ktype = gguf_get_kv_type(ctx_gguf, kid);
    if (ktype != GGUF_TYPE_STRING)
        throw std::runtime_error("key general.architecture has wrong type");

    return gguf_get_val_str(ctx_gguf, kid);
}

static gguf_context *load_gguf(const char *fname)
{
    struct gguf_init_params params = {
        /*.no_alloc = */ true,
        /*.ctx      = */ nullptr,
    };
    gguf_context *ctx = gguf_init_from_file(fname, params);
    if (!ctx) {
        std::cerr << __func__ << ": gguf_init_from_file failed\n";
        return nullptr;
    }

    int gguf_ver = gguf_get_version(ctx);
    if (gguf_ver > GGUF_VER_MAX) {
        std::cerr << __func__ << ": unsupported gguf version: " << gguf_ver << "\n";
        gguf_free(ctx);
        return nullptr;
    }

    return ctx;
}

static int32_t get_arch_key_u32(std::string const &modelPath, std::string const &archKey)
{
    int32_t value = -1;
    std::string arch;

    auto * ctx = load_gguf(modelPath.c_str());
    if (!ctx)
        goto cleanup;

    try {
        arch = get_arch_name(ctx);
    } catch (const std::runtime_error &) {
        goto cleanup; // cannot read key
    }

    {
        auto key = arch + "." + archKey;
        int keyidx = gguf_find_key(ctx, key.c_str());
        if (keyidx != -1) {
            value = gguf_get_val_u32(ctx, keyidx);
        } else {
            std::cerr << __func__ << ": " << key << " not found in " << modelPath << "\n";
        }
    }

cleanup:
    gguf_free(ctx);
    return value;
}

struct LLamaPrivate {
    bool                         modelLoaded  = false;
    int                          device       = -1;
    std::string                  deviceName;
    int64_t                      n_threads    = 0;
    std::vector<LLModel::Token>  end_tokens;
    const char                  *backend_name = nullptr;
    std::vector<LLModel::Token>  inputTokens;

    llama_model          *model        = nullptr;
    llama_context        *ctx          = nullptr;
    llama_model_params    model_params;
    llama_context_params  ctx_params;
    llama_sampler        *sampler_chain;
};

LLamaModel::LLamaModel()
    : d_ptr(std::make_unique<LLamaPrivate>())
{
    auto sparams = llama_sampler_chain_default_params();
    d_ptr->sampler_chain = llama_sampler_chain_init(sparams);
}

// default hparams (LLaMA 7B)
struct llama_file_hparams {
    uint32_t n_vocab = 32000;
    uint32_t n_embd  = 4096;
    uint32_t n_mult  = 256;
    uint32_t n_head  = 32;
    uint32_t n_layer = 32;
    uint32_t n_rot   = 64;
    enum llama_ftype ftype = LLAMA_FTYPE_MOSTLY_F16;
};

size_t LLamaModel::requiredMem(const std::string &modelPath, int n_ctx, int ngl)
{
    // TODO(cebtenzzre): update to GGUF
    (void)ngl; // FIXME(cetenzzre): use this value
    auto fin = std::ifstream(modelPath, std::ios::binary);
    fin.seekg(0, std::ios_base::end);
    size_t filesize = fin.tellg();
    fin.seekg(0, std::ios_base::beg);
    uint32_t magic = 0;
    fin.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != 0x67676a74) return 0;
    uint32_t version = 0;
    fin.read(reinterpret_cast<char*>(&version), sizeof(version));
    llama_file_hparams hparams;
    fin.read(reinterpret_cast<char*>(&hparams.n_vocab), sizeof(hparams.n_vocab));
    fin.read(reinterpret_cast<char*>(&hparams.n_embd), sizeof(hparams.n_embd));
    fin.read(reinterpret_cast<char*>(&hparams.n_head), sizeof(hparams.n_head));
    fin.read(reinterpret_cast<char*>(&hparams.n_layer), sizeof(hparams.n_layer));
    fin.read(reinterpret_cast<char*>(&hparams.n_rot), sizeof(hparams.n_rot));
    fin.read(reinterpret_cast<char*>(&hparams.ftype), sizeof(hparams.ftype));
    const size_t kvcache_element_size = 2; // fp16
    const size_t est_kvcache_size = hparams.n_embd * hparams.n_layer * 2u * n_ctx * kvcache_element_size;
    return filesize + est_kvcache_size;
}

bool LLamaModel::isModelBlacklisted(const std::string &modelPath) const
{
    auto * ctx = load_gguf(modelPath.c_str());
    if (!ctx) {
        std::cerr << __func__ << ": failed to load " << modelPath << "\n";
        return false;
    }

    auto get_key = [ctx, &modelPath](const char *name) {
        int keyidx = gguf_find_key(ctx, name);
        if (keyidx == -1) {
            throw std::logic_error(name + " not found in "s + modelPath);
        }
        return keyidx;
    };

    bool res = false;
    try {
        std::string name(gguf_get_val_str(ctx, get_key("general.name")));
        int token_idx = get_key("tokenizer.ggml.tokens");
        int n_vocab = gguf_get_arr_n(ctx, token_idx);

        // check for known bad models
        if (name == "open-orca_mistral-7b-openorca"
            && n_vocab == 32002
            && gguf_get_arr_str(ctx, token_idx, 32000) == "<dummy32000>"s // should be <|im_end|>
        ) {
            res = true;
        }
    } catch (const std::logic_error &e) {
        std::cerr << __func__ << ": " << e.what() << "\n";
    }

    gguf_free(ctx);
    return res;
}

bool LLamaModel::isEmbeddingModel(const std::string &modelPath) const
{
    bool result = false;
    std::string arch;

    auto *ctx_gguf = load_gguf(modelPath.c_str());
    if (!ctx_gguf) {
        std::cerr << __func__ << ": failed to load GGUF from " <<  modelPath << "\n";
        goto cleanup;
    }

    try {
        arch = get_arch_name(ctx_gguf);
    } catch (const std::runtime_error &) {
        goto cleanup; // cannot read key
    }

    result = is_embedding_arch(arch);

cleanup:
    gguf_free(ctx_gguf);
    return result;
}

bool LLamaModel::loadModel(const std::string &modelPath, int n_ctx, int ngl)
{
    d_ptr->modelLoaded = false;

    // clean up after previous loadModel()
    if (d_ptr->model) {
        llama_free_model(d_ptr->model);
        d_ptr->model = nullptr;
    }
    if (d_ptr->ctx) {
        llama_free(d_ptr->ctx);
        d_ptr->ctx = nullptr;
    }

    if (n_ctx < 8) {
        std::cerr << "warning: minimum context size is 8, using minimum size.\n";
        n_ctx = 8;
    }

    // -- load the model --

    gpt_params params;

    d_ptr->model_params = llama_model_default_params();

    d_ptr->model_params.use_mmap  = params.use_mmap;
#if defined (__APPLE__)
    d_ptr->model_params.use_mlock = true;
#else
    d_ptr->model_params.use_mlock = params.use_mlock;
#endif

    d_ptr->model_params.progress_callback = &LLModel::staticProgressCallback;
    d_ptr->model_params.progress_callback_user_data = this;

    d_ptr->backend_name = "cpu"; // default

#if defined(GGML_USE_KOMPUTE) || defined(GGML_USE_VULKAN) || defined(GGML_USE_CUDA)
    if (d_ptr->device != -1) {
        d_ptr->model_params.main_gpu = d_ptr->device;
        d_ptr->model_params.n_gpu_layers = ngl;
        d_ptr->model_params.split_mode = LLAMA_SPLIT_MODE_NONE;
    } else {
#ifdef GGML_USE_CUDA
        std::cerr << "Llama ERROR: CUDA loadModel was called without a device\n";
        return false;
#endif // GGML_USE_CUDA
    }
#elif defined(GGML_USE_METAL)
    (void)ngl;

    if (llama_verbose()) {
        std::cerr << "llama.cpp: using Metal" << std::endl;
    }
    d_ptr->backend_name = "metal";

    // always fully offload on Metal
    // TODO(cebtenzzre): use this parameter to allow using more than 53% of system RAM to load a model
    d_ptr->model_params.n_gpu_layers = 100;
#else // !KOMPUTE && !VULKAN && !CUDA && !METAL
    (void)ngl;
#endif

    d_ptr->model = llama_load_model_from_file(modelPath.c_str(), d_ptr->model_params);
    if (!d_ptr->model) {
        fflush(stdout);
#ifndef GGML_USE_CUDA
        d_ptr->device = -1;
        d_ptr->deviceName.clear();
#endif
        std::cerr << "LLAMA ERROR: failed to load model from " << modelPath << std::endl;
        return false;
    }

    // -- initialize the context --

    d_ptr->ctx_params = llama_context_default_params();

    bool isEmbedding = is_embedding_arch(llama_model_arch(d_ptr->model));
    const int n_ctx_train = llama_n_ctx_train(d_ptr->model);
    if (isEmbedding) {
        d_ptr->ctx_params.n_batch  = n_ctx;
        d_ptr->ctx_params.n_ubatch = n_ctx;
    } else {
        if (n_ctx > n_ctx_train) {
            std::cerr << "warning: model was trained on only " << n_ctx_train << " context tokens ("
                      << n_ctx << " specified)\n";
        }
    }

    d_ptr->ctx_params.n_ctx  = n_ctx;
    d_ptr->ctx_params.type_k = params.kv_type;
    d_ptr->ctx_params.type_v = params.kv_type;

    // The new batch API provides space for n_vocab*n_tokens logits. Tell llama.cpp early
    // that we want this many logits so the state serializes consistently.
    d_ptr->ctx_params.logits_all = true;

    d_ptr->n_threads = std::min(4, (int32_t) std::thread::hardware_concurrency());
    d_ptr->ctx_params.n_threads       = d_ptr->n_threads;
    d_ptr->ctx_params.n_threads_batch = d_ptr->n_threads;

    if (isEmbedding)
        d_ptr->ctx_params.embeddings = true;

    d_ptr->ctx = llama_new_context_with_model(d_ptr->model, d_ptr->ctx_params);
    if (!d_ptr->ctx) {
        fflush(stdout);
        std::cerr << "LLAMA ERROR: failed to init context for model " <<  modelPath << std::endl;
        llama_free_model(d_ptr->model);
        d_ptr->model = nullptr;
#ifndef GGML_USE_CUDA
        d_ptr->device = -1;
        d_ptr->deviceName.clear();
#endif
        return false;
    }

    d_ptr->end_tokens = {llama_token_eos(d_ptr->model)};

    if (usingGPUDevice()) {
#ifdef GGML_USE_KOMPUTE
        if (llama_verbose()) {
            std::cerr << "llama.cpp: using Vulkan on " << d_ptr->deviceName << std::endl;
        }
        d_ptr->backend_name = "kompute";
#elif defined(GGML_USE_VULKAN)
        d_ptr->backend_name = "vulkan";
#elif defined(GGML_USE_CUDA)
        d_ptr->backend_name = "cuda";
#endif
    }

    m_supportsEmbedding = isEmbedding;
    m_supportsCompletion = !isEmbedding;

    fflush(stdout);
    d_ptr->modelLoaded = true;
    return true;
}

void LLamaModel::setThreadCount(int32_t n_threads)
{
    d_ptr->n_threads = n_threads;
    llama_set_n_threads(d_ptr->ctx, n_threads, n_threads);
}

int32_t LLamaModel::threadCount() const
{
    return d_ptr->n_threads;
}

LLamaModel::~LLamaModel()
{
    if (d_ptr->ctx) {
        llama_free(d_ptr->ctx);
    }
    llama_free_model(d_ptr->model);
    llama_sampler_free(d_ptr->sampler_chain);
}

bool LLamaModel::isModelLoaded() const
{
    return d_ptr->modelLoaded;
}

size_t LLamaModel::stateSize() const
{
    return llama_state_get_size(d_ptr->ctx);
}

size_t LLamaModel::saveState(std::span<uint8_t> stateOut, std::vector<Token> &inputTokensOut) const
{
    size_t bytesWritten = llama_state_get_data(d_ptr->ctx, stateOut.data(), stateOut.size());
    if (bytesWritten)
        inputTokensOut.assign(d_ptr->inputTokens.begin(), d_ptr->inputTokens.end());
    return bytesWritten;
}

size_t LLamaModel::restoreState(std::span<const uint8_t> state, std::span<const Token> inputTokens)
{
    size_t bytesRead = llama_state_set_data(d_ptr->ctx, state.data(), state.size());
    if (bytesRead)
        d_ptr->inputTokens.assign(inputTokens.begin(), inputTokens.end());
    return bytesRead;
}

std::vector<LLModel::Token> LLamaModel::tokenize(std::string_view str) const
{
    std::vector<LLModel::Token> fres(str.length() + 4);
    int32_t fres_len = llama_tokenize(
        d_ptr->model, str.data(), str.length(), fres.data(), fres.size(), /*add_special*/ true, /*parse_special*/ true
    );
    fres.resize(fres_len);
    return fres;
}

bool LLamaModel::isSpecialToken(Token id) const
{
    return llama_token_get_attr(d_ptr->model, id)
        & (LLAMA_TOKEN_ATTR_CONTROL | LLAMA_TOKEN_ATTR_USER_DEFINED | LLAMA_TOKEN_ATTR_UNKNOWN);
}

std::string LLamaModel::tokenToString(Token id) const
{
    std::vector<char> result(8, 0);
    const int n_tokens = llama_token_to_piece(d_ptr->model, id, result.data(), result.size(), 0, true);
    if (n_tokens < 0) {
        result.resize(-n_tokens);
        int check = llama_token_to_piece(d_ptr->model, id, result.data(), result.size(), 0, true);
        GGML_ASSERT(check == -n_tokens);
    }
    else {
        result.resize(n_tokens);
    }

    return std::string(result.data(), result.size());
}

void LLamaModel::initSampler(const PromptContext &promptCtx)
{
    auto *model = d_ptr->model;
    auto *chain = d_ptr->sampler_chain;

    // clear sampler chain
    for (int i = llama_sampler_chain_n(chain) - 1; i >= 0; i--) {
        auto *smpl = llama_sampler_chain_remove(chain, i);
        llama_sampler_free(smpl);
    }

    // build new chain
    llama_sampler_chain_add(chain,
        llama_sampler_init_penalties(
            llama_n_vocab(model),
            llama_token_eos(model),
            llama_token_nl(model),
            promptCtx.repeat_last_n,
            promptCtx.repeat_penalty,
            // TODO(jared): consider making the below configurable
            /*penalty_freq*/    0.0f,
            /*penalty_present*/ 0.0f,
            /*penalize_nl*/     true,
            /*ignore_eos*/      false
        )
    );
    if (promptCtx.temp == 0.0f) {
        llama_sampler_chain_add(chain, llama_sampler_init_greedy());
    } else {
        struct llama_sampler *samplers[] = {
            llama_sampler_init_top_k(promptCtx.top_k),
            llama_sampler_init_top_p(promptCtx.top_p, 1),
            llama_sampler_init_min_p(promptCtx.min_p, 1),
            llama_sampler_init_temp(promptCtx.temp),
            llama_sampler_init_softmax(),
            llama_sampler_init_dist(LLAMA_DEFAULT_SEED),
        };
        for (auto *smpl : samplers)
            llama_sampler_chain_add(chain, smpl);
    }
}

LLModel::Token LLamaModel::sampleToken() const
{
    return llama_sampler_sample(d_ptr->sampler_chain, d_ptr->ctx, -1);
}

bool LLamaModel::evalTokens(int32_t nPast, std::span<const Token> tokens) const
{
    assert(!tokens.empty());

    llama_kv_cache_seq_rm(d_ptr->ctx, 0, nPast, -1);

    llama_batch batch = llama_batch_init(tokens.size(), 0, 1);

    batch.n_tokens = tokens.size();

    for (int32_t i = 0; i < batch.n_tokens; i++) {
        batch.token   [i] = tokens[i];
        batch.pos     [i] = nPast + i;
        batch.n_seq_id[i] = 1;
        batch.seq_id  [i][0] = 0;
        batch.logits  [i] = false;
    }

    // llama_decode will output logits only for the last token of the prompt
    batch.logits[batch.n_tokens - 1] = true;

    int res = llama_decode(d_ptr->ctx, batch);
    llama_batch_free(batch);
    return res == 0;
}

void LLamaModel::shiftContext(const PromptContext &promptCtx, int32_t *nPast)
{
    // infinite text generation via context shifting

    // erase up to n_ctx*contextErase tokens
    int n_keep = shouldAddBOS();
    int n_past = *nPast;
    int n_discard = std::min(n_past - n_keep, int(contextLength() * promptCtx.contextErase));

    assert(n_discard > 0);
    if (n_discard <= 0)
        return;

    std::cerr << "Llama: context full, swapping: n_past = " << n_past << ", n_keep = " << n_keep
              << ", n_discard = " << n_discard << "\n";

    // erase the first n_discard tokens from the context
    llama_kv_cache_seq_rm (d_ptr->ctx, 0, n_keep,             n_keep + n_discard);
    llama_kv_cache_seq_add(d_ptr->ctx, 0, n_keep + n_discard, n_past,             -n_discard);

    auto &inp = d_ptr->inputTokens;
    inp.erase(inp.begin() + n_keep, inp.begin() + n_keep + n_discard);
    *nPast = inp.size();
}

int32_t LLamaModel::contextLength() const
{
    return llama_n_ctx(d_ptr->ctx);
}

auto LLamaModel::specialTokens() -> std::unordered_map<std::string, std::string> const
{
    if (!d_ptr->model)
        throw std::logic_error("model not loaded");

    std::unordered_map<std::string, std::string> tokens;
    if (auto id = llama_token_bos(d_ptr->model); id != LLAMA_TOKEN_NULL)
        tokens.emplace("bos_token", tokenToString(id));
    if (auto id = llama_token_eos(d_ptr->model); id != LLAMA_TOKEN_NULL)
        tokens.emplace("eos_token", tokenToString(id));
    return tokens;
}

int32_t LLamaModel::inputLength() const
{
    return d_ptr->inputTokens.size();
}

int32_t LLamaModel::computeModelInputPosition(std::span<const Token> input) const
{
    // find common prefix
    auto cacheIt = d_ptr->inputTokens.begin();
    auto inputIt = input.begin();
    while (cacheIt < d_ptr->inputTokens.end() && inputIt < input.end() && *cacheIt == *inputIt) {
        ++cacheIt; ++inputIt;
    }
    // tell the caller to ignore the tokens between [begin, inputIt)
    return inputIt - input.begin();
}

void LLamaModel::setModelInputPosition(int32_t pos)
{
    auto &inp = d_ptr->inputTokens;
    assert(pos >= 0);
    assert(pos <= inp.size());
    // truncate token cache to end at the new n_past
    if (pos < inp.size())
        inp.resize(pos);
}

void LLamaModel::appendInputToken(Token tok)
{
    d_ptr->inputTokens.push_back(tok);
}

auto LLamaModel::inputTokens() const -> std::span<const Token>
{
    return d_ptr->inputTokens;
}

const std::vector<LLModel::Token> &LLamaModel::endTokens() const
{
    return d_ptr->end_tokens;
}

bool LLamaModel::shouldAddBOS() const
{
    return llama_add_bos_token(d_ptr->model);
}

int32_t LLamaModel::maxContextLength(std::string const &modelPath) const
{
    return get_arch_key_u32(modelPath, "context_length");
}

int32_t LLamaModel::layerCount(std::string const &modelPath) const
{
    return get_arch_key_u32(modelPath, "block_count");
}

// TODO(jared): reduce redundant code and operations by combining all metadata getters for unloaded
//              models into a class that keeps the model file open
auto LLamaModel::chatTemplate(const char *modelPath) const -> std::expected<std::string, std::string>
{
    auto *ctx = load_gguf(modelPath);
    if (!ctx)
        return std::unexpected("failed to open model file");

    std::expected<std::string, std::string> result;
    enum gguf_type ktype;
    const int kid = gguf_find_key(ctx, "tokenizer.chat_template");
    if (kid == -1) {
        result = std::unexpected("key not found");
        goto cleanup;
    }

    ktype = gguf_get_kv_type(ctx, kid);
    if (ktype != GGUF_TYPE_STRING) {
        result = std::unexpected(
            "expected key type STRING (" + std::to_string(GGUF_TYPE_STRING) + "), got " + std::to_string(ktype)
        );
        goto cleanup;
    }

    result = gguf_get_val_str(ctx, kid);

cleanup:
    gguf_free(ctx);
    return result;
}

#ifdef GGML_USE_VULKAN
static const char *getVulkanVendorName(uint32_t vendorID)
{
    switch (vendorID) {
        case 0x10DE: return "nvidia";
        case 0x1002: return "amd";
        case 0x8086: return "intel";
        default:     return "unknown";
    }
}
#endif

std::vector<LLModel::GPUDevice> LLamaModel::availableGPUDevices(size_t memoryRequired) const
{
#if defined(GGML_USE_KOMPUTE) || defined(GGML_USE_VULKAN) || defined(GGML_USE_CUDA)
    size_t count = 0;

#ifdef GGML_USE_KOMPUTE
    auto *lcppDevices = ggml_vk_available_devices(memoryRequired, &count);
#elif defined(GGML_USE_VULKAN)
    (void)memoryRequired; // hasn't been used since GGUF was added
    auto *lcppDevices = ggml_vk_available_devices(&count);
#else // defined(GGML_USE_CUDA)
    (void)memoryRequired;
    auto *lcppDevices = ggml_cuda_available_devices(&count);
#endif

    if (lcppDevices) {
        std::vector<LLModel::GPUDevice> devices;
        devices.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            auto & dev = lcppDevices[i];

            devices.emplace_back(
#ifdef GGML_USE_KOMPUTE
                /* backend  = */ "kompute",
                /* index    = */ dev.index,
                /* type     = */ dev.type,
                /* heapSize = */ dev.heapSize,
                /* name     = */ dev.name,
                /* vendor   = */ dev.vendor
#elif defined(GGML_USE_VULKAN)
                /* backend  = */ "vulkan",
                /* index    = */ dev.index,
                /* type     = */ dev.type,
                /* heapSize = */ dev.heapSize,
                /* name     = */ dev.name,
                /* vendor   = */ getVulkanVendorName(dev.vendorID)
#else // defined(GGML_USE_CUDA)
                /* backend  = */ "cuda",
                /* index    = */ dev.index,
                /* type     = */ 2, // vk::PhysicalDeviceType::eDiscreteGpu
                /* heapSize = */ dev.heapSize,
                /* name     = */ dev.name,
                /* vendor   = */ "nvidia"
#endif
            );

#ifndef GGML_USE_CUDA
            ggml_vk_device_destroy(&dev);
#else
            ggml_cuda_device_destroy(&dev);
#endif
        }

        free(lcppDevices);
        return devices;
    }
#else
    (void)memoryRequired;
    std::cerr << __func__ << ": built without a GPU backend\n";
#endif

    return {};
}

bool LLamaModel::initializeGPUDevice(size_t memoryRequired, const std::string &name) const
{
#if defined(GGML_USE_VULKAN) || defined(GGML_USE_CUDA)
    auto devices = availableGPUDevices(memoryRequired);

    auto dev_it = devices.begin();
#ifndef GGML_USE_CUDA
    if (name == "amd" || name == "nvidia" || name == "intel") {
        dev_it = std::find_if(dev_it, devices.end(), [&name](auto &dev) { return dev.vendor == name; });
    } else
#endif
    if (name != "gpu") {
        dev_it = std::find_if(dev_it, devices.end(), [&name](auto &dev) { return dev.name == name; });
    }

    if (dev_it < devices.end()) {
        d_ptr->device     = dev_it->index;
        d_ptr->deviceName = dev_it->name;
        return true;
    }
    return false;
#elif defined(GGML_USE_KOMPUTE)
    ggml_vk_device device;
    bool ok = ggml_vk_get_device(&device, memoryRequired, name.c_str());
    if (ok) {
        d_ptr->device = device.index;
        d_ptr->deviceName = device.name;
        ggml_vk_device_destroy(&device);
        return true;
    }
#else
    (void)memoryRequired;
    (void)name;
#endif
    return false;
}

bool LLamaModel::initializeGPUDevice(int device, std::string *unavail_reason) const
{
#if defined(GGML_USE_KOMPUTE) || defined(GGML_USE_VULKAN) || defined(GGML_USE_CUDA)
    (void)unavail_reason;
    auto devices = availableGPUDevices();
    auto it = std::find_if(devices.begin(), devices.end(), [device](auto &dev) { return dev.index == device; });
    d_ptr->device = device;
    d_ptr->deviceName = it < devices.end() ? it->name : "(unknown)";
    return true;
#else
    (void)device;
    if (unavail_reason) {
        *unavail_reason = "built without a GPU backend";
    }
    return false;
#endif
}

bool LLamaModel::usingGPUDevice() const
{
    if (!d_ptr->model)
        return false;

    bool usingGPU = llama_model_using_gpu(d_ptr->model);
#ifdef GGML_USE_KOMPUTE
    assert(!usingGPU || ggml_vk_has_device());
#endif
    return usingGPU;
}

const char *LLamaModel::backendName() const
{
    return d_ptr->backend_name;
}

const char *LLamaModel::gpuDeviceName() const
{
    if (usingGPUDevice()) {
#if defined(GGML_USE_KOMPUTE) || defined(GGML_USE_VULKAN) || defined(GGML_USE_CUDA)
        return d_ptr->deviceName.c_str();
#elif defined(GGML_USE_METAL)
        return "Metal";
#endif
    }
    return nullptr;
}

void llama_batch_add(
                 struct llama_batch & batch,
                        llama_token   id,
                          llama_pos   pos,
    const std::vector<llama_seq_id> & seq_ids,
                               bool   logits) {
    batch.token   [batch.n_tokens] = id;
    batch.pos     [batch.n_tokens] = pos;
    batch.n_seq_id[batch.n_tokens] = seq_ids.size();
    for (size_t i = 0; i < seq_ids.size(); ++i) {
        batch.seq_id[batch.n_tokens][i] = seq_ids[i];
    }
    batch.logits  [batch.n_tokens] = logits;

    batch.n_tokens++;
}

static void batch_add_seq(llama_batch &batch, const std::vector<LLModel::Token> &tokens, int seq_id)
{
    for (unsigned i = 0; i < tokens.size(); i++) {
        llama_batch_add(batch, tokens[i], i, { seq_id }, i == tokens.size() - 1);
    }
}

size_t LLamaModel::embeddingSize() const
{
    return llama_n_embd(d_ptr->model);
}

struct EmbModelSpec {
    const char *docPrefix;
    const char *queryPrefix;
    std::vector<const char *> otherPrefixes = {};
    bool matryoshkaCapable = false;
    const char *recommendedDims = nullptr;
};

struct EmbModelGroup {
    EmbModelSpec spec;
    std::vector<const char *> names;
};

static const EmbModelSpec NOPREFIX_SPEC {"", ""};
static const EmbModelSpec NOMIC_SPEC    {"search_document", "search_query", {"clustering", "classification"}};
static const EmbModelSpec E5_SPEC       {"passage", "query"};

static const EmbModelSpec NOMIC_1_5_SPEC {
    "search_document", "search_query", {"clustering", "classification"}, true, "[768, 512, 384, 256, 128]",
};
static const EmbModelSpec LLM_EMBEDDER_SPEC {
    "Represent this document for retrieval",
    "Represent this query for retrieving relevant documents",
};
static const EmbModelSpec BGE_SPEC {
    "", "Represent this sentence for searching relevant passages",
};
static const EmbModelSpec E5_MISTRAL_SPEC {
    "", "Instruct: Given a query, retrieve relevant passages that answer the query\nQuery",
};

static const EmbModelGroup EMBEDDING_MODEL_SPECS[] {
    {NOPREFIX_SPEC,     {"all-MiniLM-L6-v1", "all-MiniLM-L12-v1", "all-MiniLM-L6-v2", "all-MiniLM-L12-v2"}},
    {NOMIC_SPEC,        {"nomic-embed-text-v1", "nomic-embed-text-v1-ablated", "nomic-embed-text-v1-unsupervised"}},
    {NOMIC_1_5_SPEC,    {"nomic-embed-text-v1.5"}},
    {LLM_EMBEDDER_SPEC, {"llm-embedder"}},
    {BGE_SPEC,          {"bge-small-en", "bge-base-en", "bge-large-en",
                         "bge-small-en-v1.5", "bge-base-en-v1.5", "bge-large-en-v1.5"}},
    // NOTE: E5 Mistral is not yet implemented in llama.cpp, so it's not in EMBEDDING_ARCHES
    {E5_SPEC,           {"e5-small", "e5-base", "e5-large",
                         "e5-small-unsupervised", "e5-base-unsupervised", "e5-large-unsupervised",
                         "e5-small-v2", "e5-base-v2", "e5-large-v2"}},
    {E5_MISTRAL_SPEC,   {"e5-mistral-7b-instruct",
                         "multilingual-e5-small", "multilingual-e5-base", "multilingual-e5-large",
                         "multilingual-e5-large-instruct"}},
};

static const EmbModelSpec *getEmbedSpec(const std::string &modelName) {
    static const auto &specs = EMBEDDING_MODEL_SPECS;
    auto it = std::find_if(specs, std::end(specs),
        [&modelName](auto &spec) {
            auto &names = spec.names;
            return std::find(names.begin(), names.end(), modelName) < names.end();
        }
    );
    return it < std::end(specs) ? &it->spec : nullptr;
}

void LLamaModel::embed(
    const std::vector<std::string> &texts, float *embeddings, bool isRetrieval, int dimensionality, size_t *tokenCount,
    bool doMean, bool atlas
) {
    const EmbModelSpec *spec;
    std::optional<std::string> prefix;
    if (d_ptr->model && (spec = getEmbedSpec(llama_model_name(d_ptr->model))))
        prefix = isRetrieval ? spec->queryPrefix : spec->docPrefix;

    embed(texts, embeddings, prefix, dimensionality, tokenCount, doMean, atlas);
}

void LLamaModel::embed(
    const std::vector<std::string> &texts, float *embeddings, std::optional<std::string> prefix, int dimensionality,
    size_t *tokenCount, bool doMean, bool atlas, LLModel::EmbedCancelCallback *cancelCb
) {
    if (!d_ptr->model)
        throw std::logic_error("no model is loaded");

    const char *modelName = llama_model_name(d_ptr->model);
    if (!m_supportsEmbedding)
        throw std::logic_error("not an embedding model: "s + modelName);

    auto *spec = getEmbedSpec(modelName);
    if (!spec)
        std::cerr << __func__ << ": warning: unknown model " << modelName << "\n";

    const int32_t n_embd = llama_n_embd(d_ptr->model);
    if (dimensionality < 0) {
        dimensionality = n_embd;
    } else if (spec && dimensionality != n_embd) {
        auto msg = [dimensionality, modelName]() {
            return "unsupported dimensionality " + std::to_string(dimensionality) + " for model " + modelName;
        };
        if (!spec->matryoshkaCapable)
            throw std::out_of_range(msg() + " (supported: " + std::to_string(n_embd) + ")");
        if (dimensionality == 0 || dimensionality > n_embd)
            throw std::out_of_range(msg() + " (recommended: " + spec->recommendedDims + ")");
    }

    if (!prefix) {
        if (!spec)
            throw std::invalid_argument("unknown model "s + modelName + ", specify a prefix if applicable or an empty string");
        prefix = spec->docPrefix;
    } else if (spec && prefix != spec->docPrefix && prefix != spec->queryPrefix &&
               std::find(spec->otherPrefixes.begin(), spec->otherPrefixes.end(), *prefix) == spec->otherPrefixes.end())
    {
        std::stringstream ss;
        ss << std::quoted(*prefix) << " is not a valid task type for model " << modelName;
        throw std::invalid_argument(ss.str());
    }

    embedInternal(texts, embeddings, *prefix, dimensionality, tokenCount, doMean, atlas, cancelCb, spec);
}

// MD5 hash of "nomic empty"
static const char EMPTY_PLACEHOLDER[] = "24df574ea1c998de59d5be15e769658e";

auto product(double a) -> std::function<double(double)>
{
    return [a](double b) { return a * b; };
}

template <typename T>
double getL2NormScale(T *start, T *end)
{
    double magnitude = std::sqrt(std::inner_product(start, end, start, 0.0));
    return 1.0 / std::max(magnitude, 1e-12);
}

void LLamaModel::embedInternal(
    const std::vector<std::string> &texts, float *embeddings, std::string prefix, int dimensionality,
    size_t *tokenCount, bool doMean, bool atlas, LLModel::EmbedCancelCallback *cancelCb, const EmbModelSpec *spec
) {
    typedef std::vector<LLModel::Token> TokenString;
    static constexpr int32_t atlasMaxLength = 8192;
    static constexpr int chunkOverlap = 8; // Atlas overlaps chunks of input by 8 tokens

    const llama_token bos_token = llama_token_bos(d_ptr->model);
    const llama_token eos_token = llama_token_eos(d_ptr->model);

    bool useBOS = llama_add_bos_token(d_ptr->model);
    bool useEOS = llama_vocab_type(d_ptr->model) == LLAMA_VOCAB_TYPE_WPM;

    // no EOS, optional BOS
    auto tokenize = [this, useBOS, useEOS, eos_token](std::string text, TokenString &tokens, bool wantBOS) {
        if (!text.empty() && text[0] != ' ') {
            text = ' ' + text; // normalize for SPM - our fork of llama.cpp doesn't add a space prefix
        }

        tokens.resize(text.length()+4);
        int32_t n_tokens = llama_tokenize_gpt4all(
            d_ptr->model, text.c_str(), text.length(), tokens.data(), tokens.size(), /*add_special*/ wantBOS,
            /*parse_special*/ false, /*insert_space*/ false
        );
        if (n_tokens) {
            (void)eos_token;
            (void)useBOS;
            assert((useEOS && wantBOS && useBOS) == (eos_token != -1 && tokens[n_tokens - 1] == eos_token));
            if (useEOS && wantBOS)
                n_tokens--; // erase EOS/SEP
        }
        tokens.resize(n_tokens);
    };

    // tokenize the texts
    std::vector<TokenString> inputs;
    for (unsigned i = 0; i < texts.size(); i++) {
        auto &text = texts[i];
        auto &inp = inputs.emplace_back();
        tokenize(text, inp, false);
        if (atlas && inp.size() > atlasMaxLength) {
            if (doMean) {
                throw std::length_error(
                    "length of text at index " + std::to_string(i) + " is " + std::to_string(inp.size()) +
                    " tokens which exceeds limit of " + std::to_string(atlasMaxLength)
                );
            }
            inp.resize(atlasMaxLength);
        } else if (inp.empty()) {
            if (!atlas || !text.empty()) {
                std::cerr << __func__ << ": warning: chunking tokenized text at index " << std::to_string(i)
                          << " into zero tokens\n";
            }
            tokenize(EMPTY_PLACEHOLDER, inp, false);
        }
    }

    // tokenize the prefix
    TokenString prefixTokens;
    if (prefix.empty()) {
        prefixTokens.push_back(bos_token);
    } else {
        tokenize(prefix + ':', prefixTokens, true);
    }

    // n_ctx_train: max sequence length of model (RoPE scaling not implemented)
    const uint32_t n_ctx_train = llama_n_ctx_train(d_ptr->model);
    // n_batch (equals n_ctx): max tokens per call to llama_decode (one more more sequences)
    const uint32_t n_batch = llama_n_batch(d_ptr->ctx);

    // effective sequence length minus prefix and SEP token
    const uint32_t max_len = std::min(n_ctx_train, n_batch) - (prefixTokens.size() + useEOS);
    if (max_len <= chunkOverlap) {
        throw std::logic_error("max chunk length of " + std::to_string(max_len) + " is smaller than overlap of " +
                               std::to_string(chunkOverlap) + " tokens");
    }

    // split into max_len-sized chunks
    struct split_batch { unsigned idx; TokenString batch; };
    std::vector<split_batch> batches;
    size_t totalTokens = 0;
    for (unsigned i = 0; i < inputs.size(); i++) {
        auto &input = inputs[i];
        for (unsigned j = 0; j < input.size(); j += max_len) {
            if (j) { j -= chunkOverlap; }
            unsigned end = std::min(j + max_len, unsigned(input.size()));
            batches.push_back({ i, {} });
            auto &batch = batches.back().batch;
            batch = prefixTokens;
            batch.insert(batch.end(), input.begin() + j, input.begin() + end);
            totalTokens += end - j;
            batch.push_back(eos_token);
            if (!doMean) { break; /* limit text to one chunk */ }
        }
    }
    inputs.clear();

    if (cancelCb) {
        // copy of batching code below, but just count tokens instead of running inference
        unsigned nBatchTokens = 0;
        std::vector<unsigned> batchSizes;
        for (const auto &inp: batches) {
            if (nBatchTokens + inp.batch.size() > n_batch) {
                batchSizes.push_back(nBatchTokens);
                nBatchTokens = 0;
            }
            nBatchTokens += inp.batch.size();
        }
        batchSizes.push_back(nBatchTokens);
        if (cancelCb(batchSizes.data(), batchSizes.size(), d_ptr->backend_name)) {
            throw std::runtime_error("operation was canceled");
        }
    }

    // initialize batch
    struct llama_batch batch = llama_batch_init(n_batch, 0, 1);

    // n_texts x n_embd matrix
    const int32_t n_embd = llama_n_embd(d_ptr->model);
    std::vector<double> embeddingsSum(texts.size() * n_embd);
    std::vector<int> embeddingsSumTotal(texts.size());
    std::vector<int> queued_indices; // text indices of batches to be processed

    auto decode = [this, &queued_indices, n_embd, &batch, &embeddingsSum, &embeddingsSumTotal, spec, dimensionality]() {
        if (llama_decode(d_ptr->ctx, batch) < 0)
            throw std::runtime_error("llama_decode failed");

        for (int i = 0; i < batch.n_tokens; ++i) {
            if (!batch.logits[i]) { continue; }
            int i_prompt = queued_indices[batch.seq_id[i][0]];
            auto *out = &embeddingsSum[i_prompt * n_embd];

            // sequence embeddings aren't available when pooling_type is NONE
            auto *embd = llama_get_embeddings_seq(d_ptr->ctx, batch.seq_id[i][0]);
            if (!embd) { embd = llama_get_embeddings_ith(d_ptr->ctx, i); }
            assert(embd);

            auto *embd_end = embd + n_embd;

            // layer normalization for nomic-embed-text-v1.5
            if (spec && spec->matryoshkaCapable) {
                // normalize mean
                double mean = std::accumulate(embd, embd_end, 0.0) / n_embd;
                std::transform(embd, embd_end, embd, [mean](double f){ return f - mean; });

                // unbiased sample variance, with Bessel's correction
                double variance = std::inner_product(embd, embd_end, embd, 0.0) / (n_embd - 1);

                // trim to matryoshka dim
                embd_end = embd + dimensionality;

                // normalize variance
                std::transform(embd, embd_end, embd, product(1.0 / std::sqrt(variance + 1e-5)));
            }

            // L2 norm
            auto scale = getL2NormScale(embd, embd_end);
            std::transform(embd, embd_end, out, out, [scale](double e, double o){ return o + scale * e; });
            embeddingsSumTotal[i_prompt]++;
        }
    };

    // break into batches
    for (const auto &inp: batches) {
        // encode if at capacity
        if (batch.n_tokens + inp.batch.size() > n_batch) {
            decode();
            batch.n_tokens = 0;
            queued_indices.clear();
        }

        // add to batch
        batch_add_seq(batch, inp.batch, queued_indices.size());
        queued_indices.push_back(inp.idx);
    }

    // final batch
    decode();

    for (unsigned i = 0; i < texts.size(); i++) {
        auto *embd = &embeddingsSum[i * n_embd];
        auto *embd_end = embd + dimensionality;
        int total = embeddingsSumTotal[i];

        // average over chunks
        std::transform(embd, embd_end, embd, product(1.0 / total));

        // L2 norm and copy
        auto scale = getL2NormScale(embd, embd_end);
        std::transform(embd, embd_end, embeddings, product(scale));
        embeddings += dimensionality;
    }

    if (tokenCount) { *tokenCount = totalTokens; }

    llama_batch_free(batch);
}

#if defined(_WIN32)
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT __attribute__ ((visibility ("default")))
#endif

extern "C" {
DLL_EXPORT bool is_g4a_backend_model_implementation()
{
    return true;
}

DLL_EXPORT const char *get_model_type()
{
    return modelType_;
}

DLL_EXPORT const char *get_build_variant()
{
    return GGML_BUILD_VARIANT;
}

DLL_EXPORT char *get_file_arch(const char *fname)
{
    char *arch = nullptr;
    std::string archStr;

    auto *ctx = load_gguf(fname);
    if (!ctx)
        goto cleanup;

    try {
        archStr = get_arch_name(ctx);
    } catch (const std::runtime_error &) {
        goto cleanup; // cannot read key
    }

    if (is_embedding_arch(archStr) && gguf_find_key(ctx, (archStr + ".pooling_type").c_str()) < 0) {
        // old bert.cpp embedding model
    } else {
        arch = strdup(archStr.c_str());
    }

cleanup:
    gguf_free(ctx);
    return arch;
}

DLL_EXPORT bool is_arch_supported(const char *arch)
{
    return std::find(KNOWN_ARCHES.begin(), KNOWN_ARCHES.end(), std::string(arch)) < KNOWN_ARCHES.end();
}

DLL_EXPORT LLModel *construct()
{
    llama_log_set([](auto l, auto t, auto u) { llama_log_callback(l, t, u, false); }, nullptr);
#ifdef GGML_USE_CUDA
    ggml_backend_cuda_log_set_callback([](auto l, auto t, auto u) { llama_log_callback(l, t, u, true); }, nullptr);
#endif
    return new LLamaModel;
}
}
