#include "llmodel.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <iterator>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ranges = std::ranges;

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
                     bool allowContextShift,
                     PromptContext &promptCtx,
                     bool special,
                     std::optional<std::string_view> fakeReply)
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

    // sanity checks
    if (promptCtx.n_past > contextLength()) {
        std::ostringstream ss;
        ss << "n_past=" << promptCtx.n_past << " is past end of context length=" << contextLength();
        throw std::out_of_range(ss.str());
    }
    if (promptCtx.n_past > promptCtx.tokens.size()) {
        std::ostringstream ss;
        ss << "n_past=" << promptCtx.n_past << " is past end of token cache length=" << promptCtx.tokens.size();
        throw std::out_of_range(ss.str());
    }

    promptCtx.n_ctx = contextLength();
    promptCtx.n_batch = std::min(promptCtx.n_batch, LLMODEL_MAX_PROMPT_BATCH);

    if (promptCtx.n_past < promptCtx.tokens.size())
        promptCtx.tokens.resize(promptCtx.n_past);
    m_tokenize_last_token = promptCtx.tokens.empty() ? -1 : promptCtx.tokens.back(); // not serialized

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

    // tokenize the user prompt
    std::vector<Token> embd_inp;
    if (placeholders.empty()) {
        // this is unusual, but well-defined
        std::cerr << __func__ << ": prompt template has no placeholder\n";
        embd_inp = tokenize(promptTemplate, true);
    } else {
        // template: beginning of user prompt
        const auto &phUser = placeholders[0];
        std::string userPrefix(phUser.prefix());
        if (!userPrefix.empty())
            embd_inp = tokenize(userPrefix, true);

        // user input (shouldn't have special token processing)
        auto tokens = tokenize(prompt, special);
        embd_inp.insert(embd_inp.end(), tokens.begin(), tokens.end());

        // template: end of user prompt + start of assistant prompt
        size_t start = phUser.position() + phUser.length();
        size_t end = placeholders.size() >= 2 ? placeholders[1].position() : promptTemplate.length();
        auto userToAsst = promptTemplate.substr(start, end - start);
        if (!userToAsst.empty()) {
            tokens = tokenize(userToAsst, true);
            embd_inp.insert(embd_inp.end(), tokens.begin(), tokens.end());
        }
    }

    // decode the user prompt
    if (!decodePrompt(promptCallback, responseCallback, allowContextShift, promptCtx, embd_inp))
        return; // error

    // decode the assistant's reply, either generated or spoofed
    if (!fakeReply) {
        generateResponse(responseCallback, allowContextShift, promptCtx);
    } else {
        embd_inp = tokenize(*fakeReply, false);
        if (!decodePrompt(promptCallback, responseCallback, allowContextShift, promptCtx, embd_inp, true))
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
        embd_inp = tokenize(asstSuffix, true);
        decodePrompt(promptCallback, responseCallback, allowContextShift, promptCtx, embd_inp);
    }
}

// returns false on error
bool LLModel::decodePrompt(std::function<bool(int32_t)> promptCallback,
                           std::function<bool(int32_t, const std::string&)> responseCallback,
                           bool allowContextShift,
                           PromptContext &promptCtx,
                           std::vector<Token> embd_inp,
                           bool isResponse) {
    if ((int) embd_inp.size() > promptCtx.n_ctx - 4) {
        // FIXME: (Adam) We should find a way to bubble these strings to the UI level to allow for
        // translation
        responseCallback(-1, "Your message was too long and could not be processed. Please try again with something shorter.");
        std::cerr << implementation().modelType() << " ERROR: The prompt is " << embd_inp.size() <<
            " tokens and the context window is " << promptCtx.n_ctx << "!\n";
        return false;
    }

    // FIXME(jared): There are mitigations for this situation, such as making room before
    // copying the prompt context, or restoring the KV cache when we restore the prompt
    // context.
    if (!allowContextShift && promptCtx.n_past + embd_inp.size() > promptCtx.n_ctx) {
        std::cerr << "LLModel Warning: Not enough space, n_past=" << promptCtx.n_past << ", n_eval=" << embd_inp.size()
                  << ", n_ctx=" << promptCtx.n_ctx << "\n";
        return false;
    }

    // process the prompt in batches
    size_t i = 0;
    while (i < embd_inp.size()) {
        size_t batch_end = std::min(i + promptCtx.n_batch, embd_inp.size());
        std::span<const Token> batch(embd_inp.begin() + i, embd_inp.begin() + batch_end);

        // Check if the context has run out...
        if (promptCtx.n_past + int32_t(batch.size()) > promptCtx.n_ctx) {
            assert(allowContextShift);
            shiftContext(promptCtx);
            assert(promptCtx.n_past + int32_t(batch.size()) <= promptCtx.n_ctx);
        }

        if (!evalTokens(promptCtx, batch)) {
            std::cerr << implementation().modelType() << " ERROR: Failed to process prompt\n";
            return false;
        }

        size_t tokens = batch_end - i;
        for (size_t t = 0; t < tokens; ++t) {
            Token tok = batch[t];
            promptCtx.tokens.push_back(tok);
            promptCtx.n_past += 1;
            bool res = isResponse ? responseCallback(tok, tokenToString(tok)) : promptCallback(tok);
            if (!res)
                return false;
        }
        i = batch_end;
    }

    return true;
}

/*
 * If string s overlaps with the string key such that some prefix of the key is at the end
 * of the string, return the position in s where the first match starts. Otherwise, return
 * std::string::npos. Examples:
 * s = "bfo",  key = "foo" -> 1
 * s = "fooa", key = "foo" -> npos
 */
static std::string::size_type stringsOverlap(const std::string &s, const std::string &key)
{
    if (s.empty() || key.empty())
        throw std::invalid_argument("arguments to stringsOverlap must not be empty");

    for (int start = std::max(0, int(s.size()) - int(key.size())); start < s.size(); start++) {
        if (s.compare(start, s.size(), key, 0, s.size() - start) == 0)
            return start;
    }
    return std::string::npos;
}

void LLModel::generateResponse(std::function<bool(int32_t, const std::string&)> responseCallback,
                               bool allowContextShift,
                               PromptContext &promptCtx) {
    static const char *stopSequences[] {
        "### Instruction", "### Prompt", "### Response", "### Human", "### Assistant", "### Context",
    };

    // Don't even start if there is no room
    if (!promptCtx.n_predict)
        return;
    if (!allowContextShift && promptCtx.n_past >= promptCtx.n_ctx) {
        std::cerr << "LLModel Warning: Not enough space, n_past=" << promptCtx.n_past << ", n_ctx=" << promptCtx.n_ctx
                  << "\n";
        return;
    }

    initSampler(promptCtx);

    std::string cachedResponse;
    std::vector<Token> cachedTokens;
    int n_predicted = 0;

    // Predict next tokens
    for (bool stop = false; !stop;) {
        // Sample next token
        std::optional<Token> new_tok = sampleToken();
        std::string new_piece = tokenToString(new_tok.value());
        cachedTokens.push_back(new_tok.value());
        cachedResponse += new_piece;

        auto accept = [this, &promptCtx, &new_tok, allowContextShift]() -> bool {
            // Shift context if out of space
            if (promptCtx.n_past >= promptCtx.n_ctx) {
                (void)allowContextShift;
                assert(allowContextShift);
                shiftContext(promptCtx);
                assert(promptCtx.n_past < promptCtx.n_ctx);
            }

            // Accept the token
            Token tok = std::exchange(new_tok, std::nullopt).value();
            if (!evalTokens(promptCtx, { &tok, 1 })) {
                // TODO(jared): raise an exception
                std::cerr << implementation().modelType() << " ERROR: Failed to predict next token\n";
                return false;
            }

            promptCtx.tokens.push_back(tok);
            promptCtx.n_past += 1;
            return true;
        };

        // Check for EOS
        auto lengthLimit = std::string::npos;
        for (const auto token : endTokens()) {
            if (new_tok == token) {
                stop = true;
                lengthLimit = cachedResponse.size() - new_piece.size();
            }
        }

        if (lengthLimit != std::string::npos) {
            // EOS matched
        } else if (!isSpecialToken(new_tok.value())) {
            // Check if the response contains a stop sequence
            for (const auto &p : stopSequences) {
                auto match = cachedResponse.find(p);
                if (match != std::string::npos) stop = true;
                lengthLimit = std::min(lengthLimit, match);
                if (match == 0) break;
            }

            // Check if the response matches the start of a stop sequence
            if (lengthLimit == std::string::npos) {
                for (const auto &p : stopSequences) {
                    auto match = stringsOverlap(cachedResponse, p);
                    lengthLimit = std::min(lengthLimit, match);
                    if (match == 0) break;
                }
            }
        } else if (ranges::find(stopSequences, new_piece) < std::end(stopSequences)) {
            // Special tokens must exactly match a stop sequence
            stop = true;
            lengthLimit = cachedResponse.size() - new_piece.size();
        }

        // Optionally stop if the context will run out
        if (!allowContextShift && promptCtx.n_past + cachedTokens.size() >= promptCtx.n_ctx) {
            std::cerr << "LLModel Warning: Not enough space, n_past=" << promptCtx.n_past << ", n_ctx="
                      << promptCtx.n_ctx << "\n";
            stop = true;
        }

        // Empty the cache, up to the length limit
        std::string::size_type responseLength = 0;
        while (!cachedTokens.empty()) {
            Token tok = cachedTokens.front();
            std::string piece = tokenToString(tok);

            // Stop if the piece (or part of it) does not fit within the length limit
            if (responseLength + (stop ? 1 : piece.size()) > lengthLimit)
                break;

            // Remove token from cache
            assert(cachedResponse.starts_with(piece));
            cachedTokens.erase(cachedTokens.begin(), cachedTokens.begin() + 1);
            cachedResponse.erase(cachedResponse.begin(), cachedResponse.begin() + piece.size());

            // Accept the token, if needed (not cached)
            if (cachedTokens.empty() && new_tok && !accept())
                return;

            // Send the token
            if (!responseCallback(tok, piece) || ++n_predicted >= promptCtx.n_predict) {
                stop = true;
                break;
            }

            // FIXME(jared): we could avoid printing partial stop sequences if we didn't have to
            // output token IDs and could cache a partial token for the next prompt call
            responseLength += piece.size();
        }
        assert(cachedTokens.empty() == cachedResponse.empty());

        // Accept the token, if needed (in cache)
        if (new_tok) {
            assert(!cachedTokens.empty() && cachedTokens.back() == new_tok);
            if (stop) {
                cachedTokens.pop_back();
            } else if (!accept()) {
                return;
            }
        }
    }

    auto &tokens = promptCtx.tokens;
    if (tokens.size() < cachedTokens.size()) {
        /* This is theoretically possible if the longest stop sequence is greater than
         * n_ctx * contextErase tokens. */
        throw std::runtime_error("shifted too much context, can't go back");
    }

    auto discard_start = tokens.end() - cachedTokens.size();
    assert(std::equal(discard_start, tokens.end(), cachedTokens.begin()));
    tokens.erase(discard_start, tokens.end());

    promptCtx.n_past -= cachedTokens.size();
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
