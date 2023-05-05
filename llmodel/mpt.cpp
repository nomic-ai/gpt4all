#include "mpt.h"
#include "llama.cpp/ggml.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <random>
#include <string>
#include <vector>
#include <iostream>
#include <unistd.h>
#include <sstream>
#include <thread>
#include <unordered_set>

static const size_t MB = 1024*1024;

struct mpt_hparams {
    // FIXME: for mpt
    int32_t n_vocab = 50400;
    int32_t n_ctx   = 2048;
    int32_t n_embd  = 4096;
    int32_t n_head  = 16;
    int32_t n_layer = 28;
    int32_t n_rot   = 64;
    int32_t f16     = 1;
};

struct mpt_layer {
    // FIXME
};

struct mpt_buffer {
    uint8_t * addr = NULL;
    size_t size = 0;

    void resize(size_t size) {
        delete[] addr;
        addr = new uint8_t[size];
        this->size = size;
    }

    ~mpt_buffer() {
        std::cout << "yes we are cleaning up" << std::endl;
        fflush(stdout);
        delete[] addr;
    }
};

struct mpt_kv_cache {
    struct ggml_tensor * k;
    struct ggml_tensor * v;

    struct ggml_context * ctx = NULL;

    mpt_buffer buf;

    int n; // number of tokens currently in the cache

    ~mpt_kv_cache() {
        if (ctx) {
            ggml_free(ctx);
        }
    }
};

struct mpt_model {
    mpt_hparams hparams;

    struct mpt_kv_cache kv_self;
    struct ggml_context * ctx;
    std::map<std::string, struct ggml_tensor *> tensors;

    // FIXME

    mpt_buffer buf;

    ~mpt_model() {
        if (ctx) {
            ggml_free(ctx);
        }
    }
};

static bool kv_cache_init(
        const struct mpt_hparams & hparams,
             struct mpt_kv_cache & cache,
                         ggml_type   wtype,
                               int   n_ctx) {
    const int n_embd  = hparams.n_embd;
    const int n_layer = hparams.n_layer;

    const int64_t n_mem      = (int64_t)n_layer*n_ctx;
    const int64_t n_elements = n_embd*n_mem;

    cache.buf.resize(2u*n_elements*ggml_type_size(wtype) + 2u*MB);

    struct ggml_init_params params;
    params.mem_size   = cache.buf.size;
    params.mem_buffer = cache.buf.addr;
    params.no_alloc   = false;

    cache.ctx = ggml_init(params);

    if (!cache.ctx) {
        fprintf(stderr, "%s: failed to allocate memory for kv cache\n", __func__);
        return false;
    }

    cache.k = ggml_new_tensor_1d(cache.ctx, wtype, n_elements);
    cache.v = ggml_new_tensor_1d(cache.ctx, wtype, n_elements);

    return true;
}

struct mpt_vocab {
    // FIXME
};

// load the model's weights from a stream
bool mpt_model_load(const std::string &fname, std::istream &fin, mpt_model & model, mpt_vocab & vocab) {
    // FIXME
    return false;
}

// load the model's weights from a file path
bool gptj_model_load(const std::string & fname, mpt_model & model, mpt_vocab & vocab) {

    auto fin = std::ifstream(fname, std::ios::binary);
    if (!fin) {
        fprintf(stderr, "%s: failed to open '%s'\n", __func__, fname.c_str());
        return false;
    }

    bool loaded = mpt_model_load(fname, fin, model, vocab);
    fin.close();
    return loaded;
}

bool mpt_eval(
        mpt_model & model,
        const int n_threads,
        const int n_past,
        const std::vector<int>           & embd_inp,
              std::vector<float>         & embd_w,
              size_t                     & mem_per_token) {
    // FIXME
    return false;
}

std::vector<int> mpt_tokenize(const mpt_vocab & vocab, const std::string & text) {
    // FIXME
    return std::vector<int>();
}

const std::string mpt_token_to_str(const mpt_vocab & vocab, int token) {
    // FIXME
    return std::string();
}

int mpt_sample_top_k_top_p(
        const mpt_vocab & vocab,
        const int32_t * last_n_tokens_data,
        int   last_n_tokens_size,
        const std::vector<float> logits,
        int    top_k,
        double top_p,
        double temp,
        float repeat_penalty,
        std::mt19937 & rng)
{
    // FIXME
    return 0;
}

#define MPT_MAX_RNG_STATE 64*1024

size_t mpt_get_state_size(const mpt_model &model)
{
    // we don't know size of rng until we actually serialize it. so reserve more than enough memory for its serialized state.
    // for reference, std::mt19937(1337) serializes to 6701 bytes.
    const size_t s_rng_size        = sizeof(size_t);
    const size_t s_rng             = MPT_MAX_RNG_STATE;
    const size_t s_kv_size         = sizeof(size_t);
    const size_t s_kv_ntok         = sizeof(int);
    const size_t s_kv              = model.kv_self.buf.size;
    const size_t s_total = (
        + s_rng_size
        + s_rng
        + s_kv_size
        + s_kv_ntok
        + s_kv
    );
    fflush(stdout);
    return s_total;
}

size_t mpt_copy_state_data(const mpt_model &model, const std::mt19937 &rng, uint8_t *dest)
{
    uint8_t * out = dest;
    fflush(stdout);
    // copy rng
    {
        std::stringstream rng_ss;
        rng_ss << rng;

        const size_t rng_size = rng_ss.str().size();
        char rng_buf[MPT_MAX_RNG_STATE];

        memset(&rng_buf[0], 0, MPT_MAX_RNG_STATE);
        memcpy(&rng_buf[0], rng_ss.str().data(), rng_ss.str().size());

        memcpy(out, &rng_size,   sizeof(rng_size));   out += sizeof(rng_size);
        memcpy(out, &rng_buf[0], MPT_MAX_RNG_STATE); out += MPT_MAX_RNG_STATE;
    }

    // copy kv cache
    {
        const size_t kv_size = model.kv_self.buf.size;
        const int    kv_ntok = model.kv_self.n;

        memcpy(out, &kv_size, sizeof(kv_size)); out += sizeof(kv_size);
        memcpy(out, &kv_ntok, sizeof(kv_ntok)); out += sizeof(kv_ntok);

        if (kv_size) {
            memcpy(out, model.kv_self.buf.addr, kv_size); out += kv_size;
        }
    }

    const size_t written  = out - dest;
    const size_t expected = mpt_get_state_size(model);
    assert(written == expected);
    fflush(stdout);
    return written;
}

size_t mpt_set_state_data(mpt_model *model, std::mt19937 *rng, const uint8_t *src)
{
    const uint8_t * in = src;

    // set rng
    {
        size_t rng_size;
        char   rng_buf[MPT_MAX_RNG_STATE];

        memcpy(&rng_size,   in, sizeof(rng_size));    in += sizeof(rng_size);
        memcpy(&rng_buf[0], in, MPT_MAX_RNG_STATE); in += MPT_MAX_RNG_STATE;

        std::stringstream rng_ss;
        rng_ss.str(std::string(&rng_buf[0], rng_size));
        rng_ss >> *rng;

        assert(rng_ss.fail() == false);
    }

    // set kv cache
    {
        size_t kv_size;
        int kv_ntok;

        memcpy(&kv_size, in, sizeof(kv_size)); in += sizeof(kv_size);
        memcpy(&kv_ntok, in, sizeof(kv_ntok)); in += sizeof(kv_ntok);

        if (kv_size) {
            assert(model->kv_self.buf.size == kv_size);

            void * k_data = model->kv_self.k->data; // remember data pointers
            void * v_data = model->kv_self.v->data; // because their value is stored in buf and overwritten by memcpy

            memcpy(model->kv_self.buf.addr, in, kv_size); in += kv_size;

            model->kv_self.k->data = k_data; // restore correct data pointers
            model->kv_self.v->data = v_data;

        }

        model->kv_self.n = kv_ntok;
    }

    const size_t nread    = in - src;
    const size_t expected = mpt_get_state_size(*model);
    assert(nread == expected);
    fflush(stdout);
    return nread;
}

struct MPTPrivate {
    const std::string modelPath;
    bool modelLoaded;
    mpt_vocab vocab;
    mpt_model *model = nullptr;
    int64_t n_threads = 0;
    size_t mem_per_token = 0;
    std::mt19937 rng;
};

MPT::MPT()
    : d_ptr(new MPTPrivate) {

    d_ptr->model = new mpt_model;
    d_ptr->modelLoaded = false;
}

bool MPT::loadModel(const std::string &modelPath) {
    std::mt19937 rng(time(NULL));
    d_ptr->rng = rng;

    auto fin = std::ifstream(modelPath, std::ios::binary);

    // load the model
    if (!mpt_model_load(modelPath, fin, *d_ptr->model, d_ptr->vocab)) {
        std::cerr << "GPT-J ERROR: failed to load model from " <<  modelPath;
        return false;
    }

    d_ptr->n_threads = std::min(4, (int32_t) std::thread::hardware_concurrency());
    d_ptr->modelLoaded = true;
    fflush(stdout);
    return true;
}

void MPT::setThreadCount(int32_t n_threads) {
    d_ptr->n_threads = n_threads;
}

int32_t MPT::threadCount() {
    return d_ptr->n_threads;
}

MPT::~MPT()
{
    delete d_ptr->model;
}

bool MPT::isModelLoaded() const
{
    return d_ptr->modelLoaded;
}

size_t MPT::stateSize() const
{
    return mpt_get_state_size(*d_ptr->model);
}

size_t MPT::saveState(uint8_t *dest) const
{
    return mpt_copy_state_data(*d_ptr->model, d_ptr->rng, dest);
}

size_t MPT::restoreState(const uint8_t *src)
{
    return mpt_set_state_data(d_ptr->model, &d_ptr->rng, src);
}

void MPT::prompt(const std::string &prompt,
        std::function<bool(int32_t)> promptCallback,
        std::function<bool(int32_t, const std::string&)> responseCallback,
        std::function<bool(bool)> recalculateCallback,
        PromptContext &promptCtx) {

    if (!isModelLoaded()) {
        std::cerr << "GPT-J ERROR: prompt won't work with an unloaded model!\n";
        return;
    }

    const int64_t t_main_start_us = ggml_time_us();

    int64_t t_sample_us  = 0;
    int64_t t_predict_us = 0;
    int64_t t_prompt_us = 0;

    // tokenize the prompt
    std::vector<int> embd_inp = mpt_tokenize(d_ptr->vocab, prompt);

    // save the context size
    promptCtx.n_ctx = d_ptr->model->hparams.n_ctx;

    if ((int) embd_inp.size() > promptCtx.n_ctx - 4) {
        responseCallback(-1, "ERROR: The prompt size exceeds the context window size and cannot be processed.");
        std::cerr << "GPT-J ERROR: The prompt is" << embd_inp.size() <<
            "tokens and the context window is" << promptCtx.n_ctx << "!\n";
        return;
    }

    promptCtx.n_predict = std::min(promptCtx.n_predict, promptCtx.n_ctx - (int) embd_inp.size());
    promptCtx.n_past = std::min(promptCtx.n_past, promptCtx.n_ctx);

    // determine the required inference memory per token:
    static bool initialized = false;
    static std::vector<int> p_instruct;
    static std::vector<int> r_instruct;
    if (!initialized) {
         mpt_eval(*d_ptr->model, d_ptr->n_threads, 0, { 0, 1, 2, 3 }, promptCtx.logits,
            d_ptr->mem_per_token);
        initialized = true;
    }

    // process the prompt in batches
    size_t i = 0;
    const int64_t t_start_prompt_us = ggml_time_us();
    while (i < embd_inp.size()) {
        size_t batch_end = std::min(i + promptCtx.n_batch, embd_inp.size());
        std::vector<int> batch(embd_inp.begin() + i, embd_inp.begin() + batch_end);

        // Check if the context has run out...
        if (promptCtx.n_past + batch.size() > promptCtx.n_ctx) {
            const int32_t erasePoint = promptCtx.n_ctx * promptCtx.contextErase;
            // Erase the first percentage of context from the tokens...
            std::cerr << "GPTJ: reached the end of the context window so resizing\n";
            promptCtx.tokens.erase(promptCtx.tokens.begin(), promptCtx.tokens.begin() + erasePoint);
            promptCtx.n_past = promptCtx.tokens.size();
            recalculateContext(promptCtx, recalculateCallback);
            assert(promptCtx.n_past + batch.size() <= promptCtx.n_ctx);
        }

        if (!mpt_eval(*d_ptr->model, d_ptr->n_threads, promptCtx.n_past, batch, promptCtx.logits,
            d_ptr->mem_per_token)) {
            std::cerr << "GPT-J ERROR: Failed to process prompt\n";
            return;
        }

        size_t tokens = batch_end - i;
        for (size_t t = 0; t < tokens; ++t) {
            if (promptCtx.tokens.size() == promptCtx.n_ctx)
                promptCtx.tokens.erase(promptCtx.tokens.begin());
            promptCtx.tokens.push_back(batch.at(t));
            if (!promptCallback(batch.at(t)))
                return;
        }
        promptCtx.n_past += batch.size();
        i = batch_end;
    }
    t_prompt_us += ggml_time_us() - t_start_prompt_us;

    int p_instructFound = 0;
    int r_instructFound = 0;

    std::string cachedResponse;
    std::vector<int> cachedTokens;
    std::unordered_set<std::string> reversePrompts
        = { "### Instruction", "### Prompt", "### Response", "### Human", "### Assistant" };

    // predict next tokens
    int32_t totalPredictions = 0;
    for (int i = 0; i < promptCtx.n_predict; i++) {

        // sample next token
        const int n_vocab = d_ptr->model->hparams.n_vocab;
        int id = 0;
        {
            const int64_t t_start_sample_us = ggml_time_us();
            id = mpt_sample_top_k_top_p(d_ptr->vocab,
                promptCtx.tokens.data() + promptCtx.n_ctx - promptCtx.n_ctx,
                promptCtx.n_ctx,
                promptCtx.logits,
                promptCtx.top_k, promptCtx.top_p, promptCtx.temp,
                promptCtx.repeat_penalty,
                d_ptr->rng);

            t_sample_us += ggml_time_us() - t_start_sample_us;
        }

        // Check if the context has run out...
        if (promptCtx.n_past + 1 > promptCtx.n_ctx) {
            const int32_t erasePoint = promptCtx.n_ctx * promptCtx.contextErase;
            // Erase the first percentage of context from the tokens...
            std::cerr << "GPTJ: reached the end of the context window so resizing\n";
            promptCtx.tokens.erase(promptCtx.tokens.begin(), promptCtx.tokens.begin() + erasePoint);
            promptCtx.n_past = promptCtx.tokens.size();
            recalculateContext(promptCtx, recalculateCallback);
            assert(promptCtx.n_past + 1 <= promptCtx.n_ctx);
        }

        const int64_t t_start_predict_us = ggml_time_us();
        if (!mpt_eval(*d_ptr->model, d_ptr->n_threads, promptCtx.n_past, { id }, promptCtx.logits,
            d_ptr->mem_per_token)) {
            std::cerr << "GPT-J ERROR: Failed to predict next token\n";
            return;
        }
        t_predict_us += ggml_time_us() - t_start_predict_us;

        promptCtx.n_past += 1;
        // display text
        ++totalPredictions;

        if (id == 50256 /*end of text*/)
            goto stop_generating;

        const std::string str = mpt_token_to_str(d_ptr->vocab, id);

        // Check if the provided str is part of our reverse prompts
        bool foundPartialReversePrompt = false;
        const std::string completed = cachedResponse + str;
        if (reversePrompts.find(completed) != reversePrompts.end()) {
            goto stop_generating;
        }

        // Check if it partially matches our reverse prompts and if so, cache
        for (auto s : reversePrompts) {
            if (s.compare(0, completed.size(), completed) == 0) {
                foundPartialReversePrompt = true;
                cachedResponse = completed;
                break;
            }
        }

        // Regardless the token gets added to our cache
        cachedTokens.push_back(id);

        // Continue if we have found a partial match
        if (foundPartialReversePrompt)
            continue;

        // Empty the cache
        for (auto t : cachedTokens) {
            if (promptCtx.tokens.size() == promptCtx.n_ctx)
                promptCtx.tokens.erase(promptCtx.tokens.begin());
            promptCtx.tokens.push_back(t);
            if (!responseCallback(t, mpt_token_to_str(d_ptr->vocab, t)))
                goto stop_generating;
        }
        cachedTokens.clear();
    }

stop_generating:

#if 0
    // report timing
    {
        const int64_t t_main_end_us = ggml_time_us();

        std::cout << "GPT-J INFO: mem per token = " << mem_per_token << " bytes\n";
        std::cout << "GPT-J INFO:   sample time = " << t_sample_us/1000.0f << " ms\n";
        std::cout << "GPT-J INFO:   prompt time = " << t_prompt_us/1000.0f << " ms\n";
        std::cout << "GPT-J INFO:  predict time = " << t_predict_us/1000.0f << " ms / " << t_predict_us/1000.0f/totalPredictions << " ms per token\n";
        std::cout << "GPT-J INFO:    total time = " << (t_main_end_us - t_main_start_us)/1000.0f << " ms\n";
        fflush(stdout);
    }
#endif

    return;
}

void MPT::recalculateContext(PromptContext &promptCtx, std::function<bool(bool)> recalculate)
{
    size_t i = 0;
    promptCtx.n_past = 0;
    while (i < promptCtx.tokens.size()) {
        size_t batch_end = std::min(i + promptCtx.n_batch, promptCtx.tokens.size());
        std::vector<int> batch(promptCtx.tokens.begin() + i, promptCtx.tokens.begin() + batch_end);

        assert(promptCtx.n_past + batch.size() <= promptCtx.n_ctx);

        if (!mpt_eval(*d_ptr->model, d_ptr->n_threads, promptCtx.n_past, batch, promptCtx.logits,
            d_ptr->mem_per_token)) {
            std::cerr << "GPTJ ERROR: Failed to process prompt\n";
            goto stop_generating;
        }
        promptCtx.n_past += batch.size();
        if (!recalculate(true))
            goto stop_generating;
        i = batch_end;
    }
    assert(promptCtx.n_past == promptCtx.tokens.size());

stop_generating:
    recalculate(false);
}
