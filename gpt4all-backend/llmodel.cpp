#include "llmodel.h"
#include "dlhandle.h"

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <unordered_set>
#include <filesystem>
#include <cassert>
#include <cstdlib>

std::string LLModel::m_implementations_search_path = ".";

static bool requires_avxonly() {
#ifdef __x86_64__
    #ifndef _MSC_VER
        return !__builtin_cpu_supports("avx2");
    #else
        int cpuInfo[4];
        __cpuidex(cpuInfo, 7, 0);
        return !(cpuInfo[1] & (1 << 5));
    #endif
#else
    return false; // Don't know how to handle non-x86_64
#endif
}

LLModel::Implementation::Implementation(Dlhandle &&dlhandle_) : dlhandle(new Dlhandle(std::move(dlhandle_))) {
    auto get_model_type = dlhandle->get<const char *()>("get_model_type");
    assert(get_model_type);
    modelType = get_model_type();
    auto get_build_variant = dlhandle->get<const char *()>("get_build_variant");
    assert(get_build_variant);
    buildVariant = get_build_variant();
    magicMatch = dlhandle->get<bool(std::ifstream&)>("magic_match");
    assert(magicMatch);
    construct_ = dlhandle->get<LLModel *()>("construct");
    assert(construct_);
}

LLModel::Implementation::Implementation(Implementation &&o)
    : construct_(o.construct_)
    , modelType(o.modelType)
    , buildVariant(o.buildVariant)
    , magicMatch(o.magicMatch)
    , dlhandle(o.dlhandle) {
    o.dlhandle = nullptr;
}

LLModel::Implementation::~Implementation() {
    if (dlhandle) delete dlhandle;
}

bool LLModel::Implementation::isImplementation(const Dlhandle &dl) {
    return dl.get<bool(uint32_t)>("is_g4a_backend_model_implementation");
}

void LLModel::prompt(const std::string &prompt,
                     std::function<bool(int32_t)> promptCallback,
                     std::function<bool(int32_t, const std::string&)> responseCallback,
                     std::function<bool(bool)> recalculateCallback,
                     PromptContext &promptCtx)
{
    if (!isModelLoaded()) {
        std::cerr << implementation().modelType << " ERROR: prompt won't work with an unloaded model!\n";
        return;
    }

    // tokenize the prompt
    std::vector<Token> embd_inp = tokenize(prompt);

    // save the context size
    promptCtx.n_ctx = getContextLength();

    if ((int) embd_inp.size() > promptCtx.n_ctx - 4) {
        responseCallback(-1, "ERROR: The prompt size exceeds the context window size and cannot be processed.");
        std::cerr << implementation().modelType << " ERROR: The prompt is" << embd_inp.size() <<
            "tokens and the context window is" << promptCtx.n_ctx << "!\n";
        return;
    }

    promptCtx.n_predict = std::min(promptCtx.n_predict, promptCtx.n_ctx - (int) embd_inp.size());
    promptCtx.n_past = std::min(promptCtx.n_past, promptCtx.n_ctx);

    // process the prompt in batches
    size_t i = 0;
    while (i < embd_inp.size()) {
        size_t batch_end = std::min(i + promptCtx.n_batch, embd_inp.size());
        std::vector<Token> batch(embd_inp.begin() + i, embd_inp.begin() + batch_end);

        // Check if the context has run out...
        if (promptCtx.n_past + int32_t(batch.size()) > promptCtx.n_ctx) {
            const int32_t erasePoint = promptCtx.n_ctx * promptCtx.contextErase;
            // Erase the first percentage of context from the tokens...
            std::cerr << implementation().modelType << ": reached the end of the context window so resizing\n";
            promptCtx.tokens.erase(promptCtx.tokens.begin(), promptCtx.tokens.begin() + erasePoint);
            promptCtx.n_past = promptCtx.tokens.size();
            recalculateContext(promptCtx, recalculateCallback);
            assert(promptCtx.n_past + int32_t(batch.size()) <= promptCtx.n_ctx);
        }

        if (!evalTokens(promptCtx, batch)) {
            std::cerr << implementation().modelType << " ERROR: Failed to process prompt\n";
            return;
        }

        size_t tokens = batch_end - i;
        for (size_t t = 0; t < tokens; ++t) {
            if (int32_t(promptCtx.tokens.size()) == promptCtx.n_ctx)
                promptCtx.tokens.erase(promptCtx.tokens.begin());
            promptCtx.tokens.push_back(batch.at(t));
            if (!promptCallback(batch.at(t)))
                return;
        }
        promptCtx.n_past += batch.size();
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
            std::cerr << implementation().modelType << ": reached the end of the context window so resizing\n";
            promptCtx.tokens.erase(promptCtx.tokens.begin(), promptCtx.tokens.begin() + erasePoint);
            promptCtx.n_past = promptCtx.tokens.size();
            recalculateContext(promptCtx, recalculateCallback);
            assert(promptCtx.n_past + 1 <= promptCtx.n_ctx);
        }

        if (!evalTokens(promptCtx, { id })) {
            std::cerr << implementation().modelType << " ERROR: Failed to predict next token\n";
            return;
        }

        promptCtx.n_past += 1;

        // display text
        for (const auto token : getEndTokens()) {
            if (id == token) return;
        }

        const std::string_view str = tokenToString(id);

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
            //TODO: Conversion to std::string can be avoided here...
            if (!responseCallback(t, std::string(tokenToString(t))))
                return;
        }
        cachedTokens.clear();
    }
}

const std::vector<LLModel::Implementation> &LLModel::implementationList() {
    // NOTE: allocated on heap so we leak intentionally on exit so we have a chance to clean up the
    // individual models without the cleanup of the static list interfering
    static auto* libs = new std::vector<LLModel::Implementation>([] () {
        std::vector<LLModel::Implementation> fres;

        auto search_in_directory = [&](const std::filesystem::path& path) {
            // Iterate over all libraries
            for (const auto& f : std::filesystem::directory_iterator(path)) {
                const std::filesystem::path& p = f.path();
                if (p.extension() != LIB_FILE_EXT) continue;
                // Add to list if model implementation
                try {
                    Dlhandle dl(p.string());
                    if (!Implementation::isImplementation(dl)) {
                        continue;
                    }
                    fres.emplace_back(Implementation(std::move(dl)));
                } catch (...) {}
            }
        };

        search_in_directory(m_implementations_search_path);
#if defined(__APPLE__)
        search_in_directory("../../../");
#endif
        return fres;
    }());
    // Return static result
    return *libs;
}

const LLModel::Implementation* LLModel::implementation(std::ifstream& f, const std::string& buildVariant) {
    for (const auto& i : implementationList()) {
        f.seekg(0);
        if (!i.magicMatch(f)) continue;
        if (buildVariant != i.buildVariant) continue;
        return &i;
    }
    return nullptr;
}

LLModel *LLModel::construct(const std::string &modelPath, std::string buildVariant) {
    //TODO: Auto-detect CUDA/OpenCL
    if (buildVariant == "auto") {
        if (requires_avxonly()) {
            buildVariant = "avxonly";
        } else {
            buildVariant = "default";
        }
    }
    // Read magic
    std::ifstream f(modelPath, std::ios::binary);
    if (!f) return nullptr;
    // Get correct implementation
    auto impl = implementation(f, buildVariant);
    if (!impl) return nullptr;
    f.close();
    // Construct and return llmodel implementation
    return impl->construct();
}
