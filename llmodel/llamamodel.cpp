#include "llamamodel.h"

#include "llama.cpp/examples/common.h"
#include "llama.cpp/llama.h"
#include "llama.cpp/ggml.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <unistd.h>
#include <random>
#include <thread>

struct LLamaPrivate {
    const std::string modelPath;
    bool modelLoaded;
    llama_context *ctx = nullptr;
    llama_context_params params;
    int64_t n_threads = 0;
};

LLamaModel::LLamaModel()
    : d_ptr(new LLamaPrivate) {

    d_ptr->modelLoaded = false;
}

bool LLamaModel::loadModel(const std::string &modelPath, std::istream &fin)
{
    std::cerr << "LLAMA ERROR: loading llama model from stream unsupported!\n";
    return false;
}

bool LLamaModel::loadModel(const std::string &modelPath)
{
    // load the model
    d_ptr->params = llama_context_default_params();

    gpt_params params;
    d_ptr->params.n_ctx      = 2048;
    d_ptr->params.n_parts    = params.n_parts;
    d_ptr->params.seed       = params.seed;
    d_ptr->params.f16_kv     = params.memory_f16;
    d_ptr->params.use_mmap   = params.use_mmap;
    d_ptr->params.use_mlock  = params.use_mlock;

    d_ptr->ctx = llama_init_from_file(modelPath.c_str(), d_ptr->params);
    if (!d_ptr->ctx) {
        std::cerr << "LLAMA ERROR: failed to load model from " <<  modelPath << std::endl;
        return false;
    }

    d_ptr->n_threads = std::min(4, (int32_t) std::thread::hardware_concurrency());
    d_ptr->modelLoaded = true;
    fflush(stderr);
    return true;
}

void LLamaModel::setThreadCount(int32_t n_threads) {
    d_ptr->n_threads = n_threads;
}

int32_t LLamaModel::threadCount() {
    return d_ptr->n_threads;
}

LLamaModel::~LLamaModel()
{
}

bool LLamaModel::isModelLoaded() const
{
    return d_ptr->modelLoaded;
}

void LLamaModel::prompt(const std::string &prompt,
        std::function<bool(int32_t, const std::string&)> response,
        std::function<bool(bool)> recalculate,
        PromptContext &promptCtx) {

    if (!isModelLoaded()) {
        std::cerr << "LLAMA ERROR: prompt won't work with an unloaded model!\n";
        return;
    }

    gpt_params params;
    params.prompt = prompt;

    // Add a space in front of the first character to match OG llama tokenizer behavior
    params.prompt.insert(0, 1, ' ');

    // tokenize the prompt
    auto embd_inp = ::llama_tokenize(d_ptr->ctx, params.prompt, false);

    // save the context size
    promptCtx.n_ctx = llama_n_ctx(d_ptr->ctx);

    if ((int) embd_inp.size() > promptCtx.n_ctx - 4) {
        std::cerr << "LLAMA ERROR: prompt is too long\n";
        return;
    }

    promptCtx.n_predict = std::min(promptCtx.n_predict, promptCtx.n_ctx - (int) embd_inp.size());
    promptCtx.n_past = std::min(promptCtx.n_past, promptCtx.n_ctx);

    // number of tokens to keep when resetting context
    params.n_keep = (int)embd_inp.size();

    // process the prompt in batches
    size_t i = 0;
    const int64_t t_start_prompt_us = ggml_time_us();
    while (i < embd_inp.size()) {
        size_t batch_end = std::min(i + promptCtx.n_batch, embd_inp.size());
        std::vector<llama_token> batch(embd_inp.begin() + i, embd_inp.begin() + batch_end);

        // Check if the context has run out...
        if (promptCtx.n_past + batch.size() > promptCtx.n_ctx) {
            const int32_t erasePoint = promptCtx.n_ctx * promptCtx.contextErase;
            // Erase the first percentage of context from the tokens...
            std::cerr << "LLAMA: reached the end of the context window so resizing\n";
            promptCtx.tokens.erase(promptCtx.tokens.begin(), promptCtx.tokens.begin() + erasePoint);
            promptCtx.n_past = promptCtx.tokens.size();
            recalculateContext(promptCtx, recalculate);
            assert(promptCtx.n_past + batch.size() <= promptCtx.n_ctx);
        }

        if (llama_eval(d_ptr->ctx, batch.data(), batch.size(), promptCtx.n_past, d_ptr->n_threads)) {
            std::cerr << "LLAMA ERROR: Failed to process prompt\n";
            return;
        }

        // We pass a null string for each token to see if the user has asked us to stop...
        size_t tokens = batch_end - i;
        for (size_t t = 0; t < tokens; ++t)
            if (!response(batch.at(t), ""))
                return;
        promptCtx.n_past += batch.size();
        i = batch_end;
    }

    // predict next tokens
    int32_t totalPredictions = 0;
    for (int i = 0; i < promptCtx.n_predict; i++) {
        // sample next token
        llama_token id = llama_sample_top_p_top_k(d_ptr->ctx,
            promptCtx.tokens.data() + promptCtx.n_ctx - promptCtx.repeat_last_n,
            promptCtx.repeat_last_n, promptCtx.top_k, promptCtx.top_p, promptCtx.temp,
            promptCtx.repeat_penalty);

        // Check if the context has run out...
        if (promptCtx.n_past + 1 > promptCtx.n_ctx) {
            const int32_t erasePoint = promptCtx.n_ctx * promptCtx.contextErase;
            // Erase the first percentage of context from the tokens...
            std::cerr << "LLAMA: reached the end of the context window so resizing\n";
            promptCtx.tokens.erase(promptCtx.tokens.begin(), promptCtx.tokens.begin() + erasePoint);
            promptCtx.n_past = promptCtx.tokens.size();
            recalculateContext(promptCtx, recalculate);
            assert(promptCtx.n_past + batch.size() <= promptCtx.n_ctx);
        }

        if (llama_eval(d_ptr->ctx, &id, 1, promptCtx.n_past, d_ptr->n_threads)) {
            std::cerr << "LLAMA ERROR: Failed to predict next token\n";
            return;
        }

        promptCtx.n_past += 1;
        // display text
        ++totalPredictions;
        if (id == llama_token_eos() || !response(id, llama_token_to_str(d_ptr->ctx, id)))
            return;
    }
}

void LLamaModel::recalculateContext(PromptContext &promptCtx, std::function<bool(bool)> recalculate)
{
    size_t i = 0;
    promptCtx.n_past = 0;
    while (i < promptCtx.tokens.size()) {
        size_t batch_end = std::min(i + promptCtx.n_batch, promptCtx.tokens.size());
        std::vector<llama_token> batch(promptCtx.tokens.begin() + i, promptCtx.tokens.begin() + batch_end);

        assert(promptCtx.n_past + batch.size() <= promptCtx.n_ctx);

        if (llama_eval(d_ptr->ctx, batch.data(), batch.size(), promptCtx.n_past, d_ptr->n_threads)) {
            std::cerr << "LLAMA ERROR: Failed to process prompt\n";
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
