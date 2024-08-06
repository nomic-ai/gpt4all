#include "llamacpp_backend.h"

#include "dlhandle.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
#   include <windows.h>
#endif

#ifdef _MSC_VER
#   include <intrin.h>
#endif

#if defined(__APPLE__) && defined(__aarch64__)
#   include "sysinfo.h" // for getSystemTotalRAMInBytes
#endif

namespace fs = std::filesystem;
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

void LlamaCppBackend::prompt(
    const std::string &prompt,
    const std::string &promptTemplate,
    std::function<bool(int32_t)> promptCallback,
    std::function<bool(int32_t, const std::string&)> responseCallback,
    bool allowContextShift,
    PromptContext &promptCtx,
    bool special,
    std::string *fakeReply
) {
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
    if (!decodePrompt(promptCallback, responseCallback, allowContextShift, promptCtx, embd_inp))
        return; // error

    // decode the assistant's reply, either generated or spoofed
    if (fakeReply == nullptr) {
        generateResponse(responseCallback, allowContextShift, promptCtx);
    } else {
        embd_inp = tokenize(promptCtx, *fakeReply, false);
        if (!decodePrompt(promptCallback, responseCallback, allowContextShift, promptCtx, embd_inp))
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
        decodePrompt(promptCallback, responseCallback, allowContextShift, promptCtx, embd_inp);
    }
}

// returns false on error
bool LlamaCppBackend::decodePrompt(
    std::function<bool(int32_t)> promptCallback,
    std::function<bool(int32_t, const std::string&)> responseCallback,
    bool allowContextShift,
    PromptContext &promptCtx,
    std::vector<Token> embd_inp
) {
    if ((int) embd_inp.size() > promptCtx.n_ctx - 4) {
        responseCallback(-1, "ERROR: The prompt size exceeds the context window size and cannot be processed.");
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
        std::vector<Token> batch(embd_inp.begin() + i, embd_inp.begin() + batch_end);

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
            promptCtx.tokens.push_back(batch.at(t));
            promptCtx.n_past += 1;
            if (!promptCallback(batch.at(t)))
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

void LlamaCppBackend::generateResponse(
    std::function<bool(int32_t, const std::string&)> responseCallback,
    bool allowContextShift,
    PromptContext &promptCtx
) {
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

    std::string cachedResponse;
    std::vector<Token> cachedTokens;
    int n_predicted = 0;

    // Predict next tokens
    for (bool stop = false; !stop;) {
        // Sample next token
        std::optional<Token> new_tok = sampleToken(promptCtx);
        std::string new_piece = tokenToString(new_tok.value());
        cachedTokens.push_back(new_tok.value());
        cachedResponse += new_piece;

        auto accept = [this, &promptCtx, &cachedTokens, &new_tok, allowContextShift]() -> bool {
            // Shift context if out of space
            if (promptCtx.n_past >= promptCtx.n_ctx) {
                (void)allowContextShift;
                assert(allowContextShift);
                shiftContext(promptCtx);
                assert(promptCtx.n_past < promptCtx.n_ctx);
            }

            // Accept the token
            Token tok = std::exchange(new_tok, std::nullopt).value();
            if (!evalTokens(promptCtx, { tok })) {
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
        } else if (ranges::contains(stopSequences, new_piece)) {
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

/* *********************************
 * Backend implementation management
 * ********************************* */

#ifndef __APPLE__
static const std::string DEFAULT_BACKENDS[] = {"kompute", "cpu"};
#elif defined(__aarch64__)
static const std::string DEFAULT_BACKENDS[] = {"metal", "cpu"};
#else
static const std::string DEFAULT_BACKENDS[] = {"cpu"};
#endif

std::string s_implementations_search_path = ".";

#if !(defined(__x86_64__) || defined(_M_X64))
    // irrelevant on non-x86_64
    #define cpu_supports_avx()  -1
    #define cpu_supports_avx2() -1
#elif defined(_MSC_VER)
    // MSVC
    static int get_cpu_info(int func_id, int reg_id) {
        int info[4];
        __cpuid(info, func_id);
        return info[reg_id];
    }

    // AVX via EAX=1: Processor Info and Feature Bits, bit 28 of ECX
    #define cpu_supports_avx()  !!(get_cpu_info(1, 2) & (1 << 28))
    // AVX2 via EAX=7, ECX=0: Extended Features, bit 5 of EBX
    #define cpu_supports_avx2() !!(get_cpu_info(7, 1) & (1 <<  5))
#else
    // gcc/clang
    #define cpu_supports_avx()  !!__builtin_cpu_supports("avx")
    #define cpu_supports_avx2() !!__builtin_cpu_supports("avx2")
#endif

LlamaCppBackend::Implementation::Implementation(Dlhandle &&dlhandle_)
    : m_dlhandle(new Dlhandle(std::move(dlhandle_))) {
    auto get_model_type = m_dlhandle->get<const char *()>("get_model_type");
    assert(get_model_type);
    m_modelType = get_model_type();
    auto get_build_variant = m_dlhandle->get<const char *()>("get_build_variant");
    assert(get_build_variant);
    m_buildVariant = get_build_variant();
    m_getFileArch = m_dlhandle->get<char *(const char *)>("get_file_arch");
    assert(m_getFileArch);
    m_isArchSupported = m_dlhandle->get<bool(const char *)>("is_arch_supported");
    assert(m_isArchSupported);
    m_construct = m_dlhandle->get<LlamaCppBackend *()>("construct");
    assert(m_construct);
}

LlamaCppBackend::Implementation::Implementation(Implementation &&o)
    : m_getFileArch(o.m_getFileArch)
    , m_isArchSupported(o.m_isArchSupported)
    , m_construct(o.m_construct)
    , m_modelType(o.m_modelType)
    , m_buildVariant(o.m_buildVariant)
    , m_dlhandle(o.m_dlhandle) {
    o.m_dlhandle = nullptr;
}

LlamaCppBackend::Implementation::~Implementation()
{
    delete m_dlhandle;
}

static bool isImplementation(const Dlhandle &dl)
{
    return dl.get<bool(uint32_t)>("is_g4a_backend_model_implementation");
}

// Add the CUDA Toolkit to the DLL search path on Windows.
// This is necessary for chat.exe to find CUDA when started from Qt Creator.
static void addCudaSearchPath()
{
#ifdef _WIN32
    if (const auto *cudaPath = _wgetenv(L"CUDA_PATH")) {
        auto libDir = std::wstring(cudaPath) + L"\\bin";
        if (!AddDllDirectory(libDir.c_str())) {
            auto err = GetLastError();
            std::wcerr << L"AddDllDirectory(\"" << libDir << L"\") failed with error 0x" << std::hex << err << L"\n";
        }
    }
#endif
}

const std::vector<LlamaCppBackend::Implementation> &LlamaCppBackend::Implementation::implementationList()
{
    if (cpu_supports_avx() == 0) {
        throw std::runtime_error("CPU does not support AVX");
    }

    // NOTE: allocated on heap so we leak intentionally on exit so we have a chance to clean up the
    // individual models without the cleanup of the static list interfering
    static auto* libs = new std::vector<Implementation>([] () {
        std::vector<Implementation> fres;

        addCudaSearchPath();

        std::string impl_name_re = "llamacpp-(cpu|metal|kompute|vulkan|cuda)";
        if (cpu_supports_avx2() == 0) {
            impl_name_re += "-avxonly";
        }
        std::regex re(impl_name_re);
        auto search_in_directory = [&](const std::string& paths) {
            std::stringstream ss(paths);
            std::string path;
            // Split the paths string by the delimiter and process each path.
            while (std::getline(ss, path, ';')) {
                std::u8string u8_path(path.begin(), path.end());
                // Iterate over all libraries
                for (const auto &f : fs::directory_iterator(u8_path)) {
                    const fs::path &p = f.path();

                    if (p.extension() != LIB_FILE_EXT) continue;
                    if (!std::regex_search(p.stem().string(), re)) {
                        std::cerr << "did not match regex: " << p.stem().string() << "\n";
                        continue;
                    }

                    // Add to list if model implementation
                    Dlhandle dl;
                    try {
                        dl = Dlhandle(p);
                    } catch (const Dlhandle::Exception &e) {
                        std::cerr << "Failed to load " << p.filename().string() << ": " << e.what() << "\n";
                        continue;
                    }
                    if (!isImplementation(dl)) {
                        std::cerr << "Not an implementation: " << p.filename().string() << "\n";
                        continue;
                    }
                    fres.emplace_back(Implementation(std::move(dl)));
                }
            }
        };

        search_in_directory(s_implementations_search_path);

        return fres;
    }());
    // Return static result
    return *libs;
}

static std::string applyCPUVariant(const std::string &buildVariant)
{
    if (buildVariant != "metal" && cpu_supports_avx2() == 0) {
        return buildVariant + "-avxonly";
    }
    return buildVariant;
}

const LlamaCppBackend::Implementation* LlamaCppBackend::Implementation::implementation(
    const char *fname,
    const std::string& buildVariant
) {
    bool buildVariantMatched = false;
    std::optional<std::string> archName;
    for (const auto& i : implementationList()) {
        if (buildVariant != i.m_buildVariant) continue;
        buildVariantMatched = true;

        char *arch = i.m_getFileArch(fname);
        if (!arch) continue;
        archName = arch;

        bool archSupported = i.m_isArchSupported(arch);
        free(arch);
        if (archSupported) return &i;
    }

    if (!buildVariantMatched)
        return nullptr;
    if (!archName)
        throw UnsupportedModelError("Unsupported file format");

    throw BadArchError(std::move(*archName));
}

LlamaCppBackend *LlamaCppBackend::Implementation::construct(
    const std::string &modelPath,
    const std::string &backend,
    int n_ctx
) {
    std::vector<std::string> desiredBackends;
    if (backend != "auto") {
        desiredBackends.push_back(backend);
    } else {
        desiredBackends.insert(desiredBackends.end(), DEFAULT_BACKENDS, std::end(DEFAULT_BACKENDS));
    }

    for (const auto &desiredBackend: desiredBackends) {
        const auto *impl = implementation(modelPath.c_str(), applyCPUVariant(desiredBackend));

        if (impl) {
            // Construct llmodel implementation
            auto *fres = impl->m_construct();
            fres->m_implementation = impl;

#if defined(__APPLE__) && defined(__aarch64__) // FIXME: See if metal works for intel macs
            /* TODO(cebtenzzre): after we fix requiredMem, we should change this to happen at
             * load time, not construct time. right now n_ctx is incorrectly hardcoded 2048 in
             * most (all?) places where this is called, causing underestimation of required
             * memory. */
            if (backend == "auto" && desiredBackend == "metal") {
                // on a 16GB M2 Mac a 13B q4_0 (0.52) works for me but a 13B q4_K_M (0.55) does not
                size_t req_mem = fres->requiredMem(modelPath, n_ctx, 100);
                if (req_mem >= size_t(0.53f * getSystemTotalRAMInBytes())) {
                    delete fres;
                    continue;
                }
            }
#else
            (void)n_ctx;
#endif

            return fres;
        }
    }

    throw MissingImplementationError("Could not find any implementations for backend: " + backend);
}

LlamaCppBackend *LlamaCppBackend::Implementation::constructGlobalLlama(const std::optional<std::string> &backend)
{
    static std::unordered_map<std::string, std::unique_ptr<LlamaCppBackend>> implCache;

    const std::vector<Implementation> *impls;
    try {
        impls = &implementationList();
    } catch (const std::runtime_error &e) {
        std::cerr << __func__ << ": implementationList failed: " << e.what() << "\n";
        return nullptr;
    }

    std::vector<std::string> desiredBackends;
    if (backend) {
        desiredBackends.push_back(backend.value());
    } else {
        desiredBackends.insert(desiredBackends.end(), DEFAULT_BACKENDS, std::end(DEFAULT_BACKENDS));
    }

    const Implementation *impl = nullptr;

    for (const auto &desiredBackend: desiredBackends) {
        auto cacheIt = implCache.find(desiredBackend);
        if (cacheIt != implCache.end())
            return cacheIt->second.get(); // cached

        for (const auto &i: *impls) {
            if (i.m_modelType == "LLaMA" && i.m_buildVariant == applyCPUVariant(desiredBackend)) {
                impl = &i;
                break;
            }
        }

        if (impl) {
            auto *fres = impl->m_construct();
            fres->m_implementation = impl;
            implCache[desiredBackend] = std::unique_ptr<LlamaCppBackend>(fres);
            return fres;
        }
    }

    std::cerr << __func__ << ": could not find Llama implementation for backend: " << backend.value_or("default")
              << "\n";
    return nullptr;
}

std::vector<LlamaCppBackend::GPUDevice> LlamaCppBackend::Implementation::availableGPUDevices(size_t memoryRequired)
{
    std::vector<LlamaCppBackend::GPUDevice> devices;
#ifndef __APPLE__
    static const std::string backends[] = {"kompute", "cuda"};
    for (const auto &backend: backends) {
        auto *llama = constructGlobalLlama(backend);
        if (llama) {
            auto backendDevs = llama->availableGPUDevices(memoryRequired);
            devices.insert(devices.end(), backendDevs.begin(), backendDevs.end());
        }
    }
#endif
    return devices;
}

int32_t LlamaCppBackend::Implementation::maxContextLength(const std::string &modelPath)
{
    auto *llama = constructGlobalLlama();
    return llama ? llama->maxContextLength(modelPath) : -1;
}

int32_t LlamaCppBackend::Implementation::layerCount(const std::string &modelPath)
{
    auto *llama = constructGlobalLlama();
    return llama ? llama->layerCount(modelPath) : -1;
}

bool LlamaCppBackend::Implementation::isEmbeddingModel(const std::string &modelPath)
{
    auto *llama = constructGlobalLlama();
    return llama && llama->isEmbeddingModel(modelPath);
}

void LlamaCppBackend::Implementation::setImplementationsSearchPath(const std::string& path)
{
    s_implementations_search_path = path;
}

const std::string& LlamaCppBackend::Implementation::implementationsSearchPath()
{
    return s_implementations_search_path;
}

bool LlamaCppBackend::Implementation::hasSupportedCPU()
{
    return cpu_supports_avx() != 0;
}

int LlamaCppBackend::Implementation::cpuSupportsAVX2()
{
    return cpu_supports_avx2();
}
