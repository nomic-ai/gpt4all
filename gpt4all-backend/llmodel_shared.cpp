#include "llmodel.h"

#include <cassert>
#include <iostream>
#include <unordered_set>

void LLModel::recalculateContext(PromptContext &promptCtx, std::function<bool(bool)> recalculate) {
    size_t i = 0;
    promptCtx.n_past = 0;
    while (i < promptCtx.tokens.size()) {
        size_t batch_end = std::min(i + promptCtx.n_batch, promptCtx.tokens.size());
        std::vector<int32_t> batch(promptCtx.tokens.begin() + i, promptCtx.tokens.begin() + batch_end);
        assert(promptCtx.n_past + int32_t(batch.size()) <= promptCtx.n_ctx);
        if (!evalTokens(promptCtx, batch)) {
            std::cerr << "LLModel ERROR: Failed to process prompt\n";
            goto stop_generating;
        }
        promptCtx.n_past += batch.size();
        if (!recalculate(true))
            goto stop_generating;
        i = batch_end;
    }
    assert(promptCtx.n_past == int32_t(promptCtx.tokens.size()));

stop_generating:
    recalculate(false);
}

void LLModel::prompt(const std::string &prompt,
                     std::function<bool(int32_t)> promptCallback,
                     std::function<bool(int32_t, const std::string&)> responseCallback,
                     std::function<bool(bool)> recalculateCallback,
                     PromptContext &promptCtx)
{
    if (!isModelLoaded()) {
        std::cerr << implementation().modelType() << " ERROR: prompt won't work with an unloaded model!\n";
        return;
    }

    if (!supportsCompletion()) {
        std::string errorMessage = "ERROR: this model does not support text completion or chat!\n";
        responseCallback(-1, errorMessage);
        std::cerr << implementation().modelType() << errorMessage;
        return;
    }

    // tokenize the prompt
    std::vector<Token> embd_inp = tokenize(promptCtx, prompt);

    // save the context size
    promptCtx.n_ctx = contextLength();

    if ((int) embd_inp.size() > promptCtx.n_ctx - 4) {
        responseCallback(-1, "ERROR: The prompt size exceeds the context window size and cannot be processed.");
        std::cerr << implementation().modelType() << " ERROR: The prompt is " << embd_inp.size() <<
            " tokens and the context window is " << promptCtx.n_ctx << "!\n";
        return;
    }

    promptCtx.n_predict = std::min(promptCtx.n_predict, promptCtx.n_ctx - (int) embd_inp.size());
    promptCtx.n_past = std::min(promptCtx.n_past, promptCtx.n_ctx);
    promptCtx.n_batch = std::min(promptCtx.n_batch, LLMODEL_MAX_PROMPT_BATCH);

    // process the prompt in batches
    size_t i = 0;
    while (i < embd_inp.size()) {
        size_t batch_end = std::min(i + promptCtx.n_batch, embd_inp.size());
        std::vector<Token> batch(embd_inp.begin() + i, embd_inp.begin() + batch_end);

        // Check if the context has run out...
        if (promptCtx.n_past + int32_t(batch.size()) > promptCtx.n_ctx) {
            const int32_t erasePoint = promptCtx.n_ctx * promptCtx.contextErase;
            // Erase the first percentage of context from the tokens...
            std::cerr << implementation().modelType() << ": reached the end of the context window so resizing\n";
            promptCtx.tokens.erase(promptCtx.tokens.begin(), promptCtx.tokens.begin() + erasePoint);
            promptCtx.n_past = promptCtx.tokens.size();
            recalculateContext(promptCtx, recalculateCallback);
            assert(promptCtx.n_past + int32_t(batch.size()) <= promptCtx.n_ctx);
        }

        if (!evalTokens(promptCtx, batch)) {
            std::cerr << implementation().modelType() << " ERROR: Failed to process prompt\n";
            return;
        }

        size_t tokens = batch_end - i;
        for (size_t t = 0; t < tokens; ++t) {
            if (int32_t(promptCtx.tokens.size()) == promptCtx.n_ctx)
                promptCtx.tokens.erase(promptCtx.tokens.begin());
            promptCtx.tokens.push_back(batch.at(t));
            promptCtx.n_past += 1;
            if (!promptCallback(batch.at(t)))
                return;
        }
        i = batch_end;
    }

    std::string cachedResponse;
    std::vector<Token> cachedTokens;
    std::unordered_set<std::string> reversePrompts
        = { "### Instruction", "### Prompt", "### Response", "### Human", "### Assistant", "### Context" };

    // predict next tokens
    for (int i = 0; i < promptCtx.n_predict; i++) {

        // sample next token
        auto id = sampleToken(promptCtx);

        // Check if the context has run out...
        if (promptCtx.n_past + 1 > promptCtx.n_ctx) {
            const int32_t erasePoint = promptCtx.n_ctx * promptCtx.contextErase;
            // Erase the first percentage of context from the tokens...
            std::cerr << implementation().modelType() << ": reached the end of the context window so resizing\n";
            promptCtx.tokens.erase(promptCtx.tokens.begin(), promptCtx.tokens.begin() + erasePoint);
            promptCtx.n_past = promptCtx.tokens.size();
            recalculateContext(promptCtx, recalculateCallback);
            assert(promptCtx.n_past + 1 <= promptCtx.n_ctx);
        }

        if (!evalTokens(promptCtx, { id })) {
            std::cerr << implementation().modelType() << " ERROR: Failed to predict next token\n";
            return;
        }

        // display text
        for (const auto token : endTokens()) {
            if (id == token) return;
        }

        const std::string str = tokenToString(id);

        // Check if the provided str is part of our reverse prompts
        bool foundPartialReversePrompt = false;
        const std::string completed = cachedResponse + std::string(str);
        if (reversePrompts.find(completed) != reversePrompts.end())
            return;

        // Check if it partially matches our reverse prompts and if so, cache
        for (const auto& s : reversePrompts) {
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
            if (int32_t(promptCtx.tokens.size()) == promptCtx.n_ctx)
                promptCtx.tokens.erase(promptCtx.tokens.begin());
            promptCtx.tokens.push_back(t);
            promptCtx.n_past += 1;
            //TODO: Conversion to std::string can be avoided here...
            if (!responseCallback(t, std::string(tokenToString(t))))
                return;
        }
        cachedTokens.clear();
    }
}

std::vector<float> LLModel::embedding(const std::string &/*text*/)
{
    if (!supportsCompletion()) {
        std::string errorMessage = "ERROR: this model does not support generating embeddings!\n";
        std::cerr << implementation().modelType() << errorMessage;
    }
    return std::vector<float>();
}
