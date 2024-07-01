#include "llmodel.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

// TODO(cebtenzzre): replace this with llama_kv_cache_seq_shift for llamamodel (GPT-J needs this as-is)
void LLModel::recalculateContext(PromptContext &promptCtx, std::function<bool(bool)> recalculate)
{
    int n_keep = shouldAddBOS();
    const int32_t n_discard = (promptCtx.n_ctx - n_keep) * promptCtx.contextErase;

    // Erase the first percentage of context from the tokens
    std::cerr << implementation().modelType() << ": reached the end of the context window so resizing\n";
    promptCtx.tokens.erase(promptCtx.tokens.begin() + n_keep, promptCtx.tokens.begin() + n_keep + n_discard);

    size_t i = n_keep;
    promptCtx.n_past = n_keep;
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

static bool parsePromptTemplate(const std::string &tmpl, std::vector<std::smatch> &placeholders, std::string &err)
{
    static const std::regex placeholderRegex(R"(%[1-2](?![0-9]))");

    auto it = std::sregex_iterator(tmpl.begin(), tmpl.end(), placeholderRegex);
    placeholders.clear();
    placeholders.insert(placeholders.end(), it, std::sregex_iterator());

    if (placeholders.size() > 2) {
        err = "ERROR: expected at most two placeholders, got " + std::to_string(placeholders.size());
        return false;
    }
    if (placeholders.size() >= 1 && placeholders[0].str() != "%1") {
        err = "ERROR: first placeholder must be %1, got " + placeholders[0].str();
        return false;
    }
    if (placeholders.size() >= 2 && placeholders[1].str() != "%2") {
        err = "ERROR: second placeholder must be %2, got " + placeholders[1].str();
        return false;
    }
    return true;
}

void LLModel::prompt(const std::string &prompt,
                     const std::string &promptTemplate,
                     std::function<bool(int32_t)> promptCallback,
                     std::function<bool(int32_t, const std::string&)> responseCallback,
                     std::function<bool(bool)> recalculateCallback,
                     PromptContext &promptCtx,
                     bool special,
                     std::string *fakeReply)
{
    if (!isModelLoaded()) {
        std::cerr << implementation().modelType() << " ERROR: prompt won't work with an unloaded model!\n";
        return;
    }

    if (!supportsCompletion()) {
        std::string errorMessage = "ERROR: this model does not support text completion or chat!";
        responseCallback(-1, errorMessage);
        std::cerr << implementation().modelType() << " " << errorMessage << "\n";
        return;
    }

    // parse the prompt template
    std::vector<std::smatch> placeholders;
    {
        std::string err;
        if (!parsePromptTemplate(promptTemplate, placeholders, err)) {
            responseCallback(-1, err);
            std::cerr << err << "\n";
            return;
        }
    }

    auto old_n_past = promptCtx.n_past; // prepare to fake n_past for tokenize

    // tokenize the user prompt
    std::vector<Token> embd_inp;
    if (placeholders.empty()) {
        // this is unusual, but well-defined
        std::cerr << __func__ << ": prompt template has no placeholder\n";
        embd_inp = tokenize(promptCtx, promptTemplate, true);
    } else {
        // template: beginning of user prompt
        const auto &phUser = placeholders[0];
        std::string userPrefix(phUser.prefix());
        if (!userPrefix.empty()) {
            embd_inp = tokenize(promptCtx, userPrefix, true);
            promptCtx.n_past += embd_inp.size();
        }

        // user input (shouldn't have special token processing)
        auto tokens = tokenize(promptCtx, prompt, special);
        embd_inp.insert(embd_inp.end(), tokens.begin(), tokens.end());
        promptCtx.n_past += tokens.size();

        // template: end of user prompt + start of assistant prompt
        size_t start = phUser.position() + phUser.length();
        size_t end = placeholders.size() >= 2 ? placeholders[1].position() : promptTemplate.length();
        auto userToAsst = promptTemplate.substr(start, end - start);
        if (!userToAsst.empty()) {
            tokens = tokenize(promptCtx, userToAsst, true);
            embd_inp.insert(embd_inp.end(), tokens.begin(), tokens.end());
            promptCtx.n_past += tokens.size();
        }
    }

    promptCtx.n_past = old_n_past; // restore n_past so decodePrompt can increment it

    // decode the user prompt
    if (!decodePrompt(promptCallback, responseCallback, recalculateCallback, promptCtx, embd_inp))
        return; // error

    // decode the assistant's reply, either generated or spoofed
    if (fakeReply == nullptr) {
        generateResponse(responseCallback, recalculateCallback, promptCtx);
    } else {
        embd_inp = tokenize(promptCtx, *fakeReply, false);
        if (!decodePrompt(promptCallback, responseCallback, recalculateCallback, promptCtx, embd_inp))
            return; // error
    }

    // decode the rest of the prompt template
    // template: end of assistant prompt
    std::string asstSuffix;
    if (placeholders.size() >= 2) {
        size_t start = placeholders[1].position() + placeholders[1].length();
        asstSuffix = promptTemplate.substr(start);
    } else {
        asstSuffix = "\n\n"; // default to a blank link, good for e.g. Alpaca
    }
    if (!asstSuffix.empty()) {
        embd_inp = tokenize(promptCtx, asstSuffix, true);
        decodePrompt(promptCallback, responseCallback, recalculateCallback, promptCtx, embd_inp);
    }
}

// returns false on error
bool LLModel::decodePrompt(std::function<bool(int32_t)> promptCallback,
                           std::function<bool(int32_t, const std::string&)> responseCallback,
                           std::function<bool(bool)> recalculateCallback,
                           PromptContext &promptCtx,
                           std::vector<Token> embd_inp) {
    // save the context size
    promptCtx.n_ctx = contextLength();

    if ((int) embd_inp.size() > promptCtx.n_ctx - 4) {
        responseCallback(-1, "ERROR: The prompt size exceeds the context window size and cannot be processed.");
        std::cerr << implementation().modelType() << " ERROR: The prompt is " << embd_inp.size() <<
            " tokens and the context window is " << promptCtx.n_ctx << "!\n";
        return false;
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
            recalculateContext(promptCtx, recalculateCallback);
            assert(promptCtx.n_past + int32_t(batch.size()) <= promptCtx.n_ctx);
        }

        if (!evalTokens(promptCtx, batch)) {
            std::cerr << implementation().modelType() << " ERROR: Failed to process prompt\n";
            return false;
        }

        size_t tokens = batch_end - i;
        for (size_t t = 0; t < tokens; ++t) {
            if (int32_t(promptCtx.tokens.size()) == promptCtx.n_ctx)
                promptCtx.tokens.erase(promptCtx.tokens.begin());
            promptCtx.tokens.push_back(batch.at(t));
            promptCtx.n_past += 1;
            if (!promptCallback(batch.at(t)))
                return false;
        }
        i = batch_end;
    }

    return true;
}

void LLModel::generateResponse(std::function<bool(int32_t, const std::string&)> responseCallback,
                               std::function<bool(bool)> recalculateCallback,
                               PromptContext &promptCtx) {
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

void LLModel::embed(
    const std::vector<std::string> &texts, float *embeddings, std::optional<std::string> prefix, int dimensionality,
    size_t *tokenCount, bool doMean, bool atlas, EmbedCancelCallback *cancelCb
) {
    (void)texts;
    (void)embeddings;
    (void)prefix;
    (void)dimensionality;
    (void)tokenCount;
    (void)doMean;
    (void)atlas;
    (void)cancelCb;
    throw std::logic_error(std::string(implementation().modelType()) + " does not support embeddings");
}

void LLModel::embed(
    const std::vector<std::string> &texts, float *embeddings, bool isRetrieval, int dimensionality, size_t *tokenCount,
    bool doMean, bool atlas
) {
    (void)texts;
    (void)embeddings;
    (void)isRetrieval;
    (void)dimensionality;
    (void)tokenCount;
    (void)doMean;
    (void)atlas;
    throw std::logic_error(std::string(implementation().modelType()) + " does not support embeddings");
}
