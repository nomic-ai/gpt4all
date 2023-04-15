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
    d_ptr->ctx = llama_init_from_file(modelPath.c_str(), d_ptr->params);
    if (!d_ptr->ctx) {
        std::cerr << "LLAMA ERROR: failed to load model from " <<  modelPath << std::endl;
        return false;
    }

    d_ptr->n_threads = std::min(4, (int32_t) std::thread::hardware_concurrency());
    d_ptr->modelLoaded = true;
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

void LLamaModel::prompt(const std::string &prompt, std::function<bool(const std::string&)> response,
        PromptContext &promptCtx, int32_t n_predict, int32_t top_k, float top_p, float temp, int32_t n_batch) {

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
    const int n_ctx = llama_n_ctx(d_ptr->ctx);

    if ((int) embd_inp.size() > n_ctx - 4) {
        std::cerr << "LLAMA ERROR: prompt is too long\n";
        return;
    }

    n_predict = std::min(n_predict, n_ctx - (int) embd_inp.size());
    promptCtx.n_past = std::min(promptCtx.n_past, n_ctx);

    // number of tokens to keep when resetting context
    params.n_keep = (int)embd_inp.size();

    // process the prompt in batches
    size_t i = 0;
    const int64_t t_start_prompt_us = ggml_time_us();
    while (i < embd_inp.size()) {
        size_t batch_end = std::min(i + n_batch, embd_inp.size());
        std::vector<llama_token> batch(embd_inp.begin() + i, embd_inp.begin() + batch_end);

        if (promptCtx.n_past + batch.size() > n_ctx) {
            std::cerr << "eval n_ctx " << n_ctx << " n_past " << promptCtx.n_past << std::endl;
            promptCtx.n_past = std::min(promptCtx.n_past, int(n_ctx - batch.size()));
            std::cerr << "after n_ctx " << n_ctx << " n_past " << promptCtx.n_past << std::endl;
        }

        if (llama_eval(d_ptr->ctx, batch.data(), batch.size(), promptCtx.n_past, d_ptr->n_threads)) {
            std::cerr << "LLAMA ERROR: Failed to process prompt\n";
            return;
        }
        // We pass a null string for each token to see if the user has asked us to stop...
        size_t tokens = batch_end - i;
        for (size_t t = 0; t < tokens; ++t)
            if (!response(""))
                return;
        promptCtx.n_past += batch.size();
        i = batch_end;
    }

    std::vector<llama_token> cachedTokens;

    // predict next tokens
    int32_t totalPredictions = 0;
    for (int i = 0; i < n_predict; i++) {
        // sample next token
        llama_token id = llama_sample_top_p_top_k(d_ptr->ctx, {}, 0, top_k, top_p, temp, 1.0f);

        if (promptCtx.n_past + 1 > n_ctx) {
            std::cerr << "eval 2 n_ctx " << n_ctx << " n_past " << promptCtx.n_past << std::endl;
            promptCtx.n_past = std::min(promptCtx.n_past, n_ctx - 1);
            std::cerr << "after 2 n_ctx " << n_ctx << " n_past " << promptCtx.n_past << std::endl;
        }

        if (llama_eval(d_ptr->ctx, &id, 1, promptCtx.n_past, d_ptr->n_threads)) {
            std::cerr << "LLAMA ERROR: Failed to predict next token\n";
            return;
        }
        cachedTokens.emplace_back(id);

        for (int j = 0; j < cachedTokens.size(); ++j) {
            llama_token cachedToken = cachedTokens.at(j);
            promptCtx.n_past += 1;
            // display text
            ++totalPredictions;
            if (id == llama_token_eos() || !response(llama_token_to_str(d_ptr->ctx, cachedToken)))
                goto stop_generating;
        }
        cachedTokens.clear();
    }

stop_generating:
    return;
}
