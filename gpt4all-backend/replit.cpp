#define REPLIT_H_I_KNOW_WHAT_I_AM_DOING_WHEN_INCLUDING_THIS_FILE
#include "replit_impl.h"

#include "utils.h"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <fstream>
#include <iostream>
#include <map>
#include <stdint.h>
#include <string>
#include <thread>
#if defined(_WIN32) && defined(_MSC_VER)
    #define WIN32_LEAN_AND_MEAN
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <io.h>
    #include <stdio.h>
#else
    #include <unistd.h>
#endif
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <regex>
#include <ggml.h>
#ifdef GGML_USE_METAL
#include <ggml-metal.h>
#endif

/**
IMPORTANT: This model backend and convert script were developed for the original Huggingface
Replit model (hugginface commit hash: 9eceafb041eb8abd565dabfbfadd328869140011)
*/

using piece_t = std::pair<std::size_t, float>;
using piece_map_t = std::unordered_map<std::string, piece_t>;

namespace {
const char *modelType_ = "Replit";

const std::string ws_symbol = "\342\226\201";
}

struct replit_tokenizer {
    gpt_vocab raw_vocab;
    piece_map_t piece_map;
    std::vector<std::string> vocab;
};

std::pair<std::vector<LLModel::Token>, float> encode_word(const std::string & word, const piece_map_t & model) {
    std::vector<int> best_segmentations_starts(word.length() + 1, -1);
    best_segmentations_starts[0] = 0;

    std::vector<float> best_segmentations_scores(word.length() + 1, -std::numeric_limits<float>::infinity());
    best_segmentations_scores[0] = 1.0;

    for (size_t start_idx = 0; start_idx < word.length(); ++start_idx) {
        float best_score_at_start = best_segmentations_scores[start_idx];
        for (size_t end_idx = start_idx + 1; end_idx <= word.length(); ++end_idx) {
            std::string token = word.substr(start_idx, end_idx - start_idx);
            if (model.count(token) && best_score_at_start != -std::numeric_limits<float>::infinity()) {
                float token_score = model.at(token).second;
                float score = token_score + best_score_at_start;
                if (best_segmentations_scores[end_idx] == -std::numeric_limits<float>::infinity() ||
                    best_segmentations_scores[end_idx] > score) {
                    best_segmentations_starts[end_idx] = start_idx;
                    best_segmentations_scores[end_idx] = score;
                }
            }
        }
    }

    if (best_segmentations_scores.back() == -std::numeric_limits<float>::infinity()) {
        return std::make_pair(std::vector<LLModel::Token>{0}, 0.0f);
    }

    float score = best_segmentations_scores.back();
    int start = best_segmentations_starts.back();
    int end = word.length();
    std::vector<LLModel::Token> tokens;
    while (start != 0) {
        const auto token_id = model.at(word.substr(start, end - start)).first;
        tokens.insert(tokens.begin(), token_id);
        int next_start = best_segmentations_starts[start];
        end = start;
        start = next_start;
    }
    const auto token_id = model.at(word.substr(start, end - start)).first;
    tokens.insert(tokens.begin(), token_id);
    return std::make_pair(tokens, score);
}

bool replit_tokenizer_load(replit_tokenizer & tokenizer, std::istream & fin, int max_vocab_size) {
    std::string word;
    std::vector<char> buf(128);

    for (LLModel::Token i = 0; i < max_vocab_size; i++) {
        uint32_t len;
        fin.read((char *)&len, sizeof(len));

        buf.resize(len);
        fin.read((char *) buf.data(), len);
        word.assign(buf.data(), len);

        float score;
        fin.read((char *)&score, sizeof(score));

        tokenizer.piece_map[word] = std::make_pair(i, -score);
        tokenizer.raw_vocab.id_to_token[i] = word;
        tokenizer.raw_vocab.token_to_id[word] = i;
    }

    return true;
}

std::string replace_all(const std::string & str,    // where to work
                        const std::string & find,   // substitute 'find'
                        const std::string & replace //      by 'replace'
) {
    std::string result;
    size_t find_len = find.size();
    size_t pos, from = 0;
    while (std::string::npos != (pos = str.find(find, from))) {
        result.append(str, from, pos - from);
        result.append(replace);
        from = pos + find_len;
    }
    result.append(str, from, std::string::npos);
    return result;
}

std::vector<LLModel::Token> replit_tokenizer_tokenize(replit_tokenizer & tokenizer, const std::string & text) {
    std::vector<LLModel::Token> tokens;
    auto normalized_text = replace_all(text, " ", ws_symbol);
    auto tokenized = encode_word(normalized_text, tokenizer.piece_map);

    return tokenized.first;
}

std::string replit_tokenizer_detokenize(replit_tokenizer & tokenizer, const std::vector<LLModel::Token> & tokens) {
    std::string text;
    for (auto token : tokens) {
        text += tokenizer.raw_vocab.id_to_token[token];
    }
    auto denormalized_text = replace_all(text, ws_symbol, " ");
    return denormalized_text;
}

// no defaults for now
struct mpt_hparams {
    int32_t n_vocab     = 0;
    int32_t n_ctx       = 0; //d_model
    int32_t n_embd      = 0; //max_seq_len
    int32_t n_head      = 0; // n_heads
    int32_t n_layer     = 0; //n_layers
    int32_t ftype       = 0; 
};

struct replit_layer {
    // pre normalization
    struct ggml_tensor * ln_1_weight;

    // attention
    struct ggml_tensor * c_attn_wqkv_weight;

    struct ggml_tensor * c_attn_out_proj_weight;

    // post normalization
    struct ggml_tensor * ln_2_weight;

    // ff
    struct ggml_tensor * c_mlp_mlp_up_weight;

    struct ggml_tensor * c_mlp_mlp_down_weight;
};

struct replit_buffer {
    uint8_t * addr = NULL;
    size_t size = 0;

    void resize(size_t size) {
        delete[] addr;
        addr = new uint8_t[size];
        this->size = size;
    }

    ~replit_buffer() {
        fflush(stdout);
        delete[] addr;
    }
};

struct replit_kv_cache {
    struct ggml_tensor * k;
    struct ggml_tensor * v;

    struct ggml_context * ctx = NULL;

    replit_buffer buf;

    int n; // number of tokens currently in the cache

    ~replit_kv_cache() {
        if (ctx) {
            ggml_free(ctx);
        }
    }
};

struct replit_model {
    mpt_hparams hparams;

    struct ggml_tensor * wte_weight;  // position embedding
    struct ggml_tensor * ln_f_weight; // language model head

    std::vector<replit_layer> layers;

    // key + value memory
    struct replit_kv_cache kv_self;

    struct ggml_context * ctx;
    void * eval_buf;
    size_t eval_buf_size;
    void * scr0_buf;
    size_t scr0_buf_size;
    void * scr1_buf;
    size_t scr1_buf_size;
    #ifdef GGML_USE_METAL
    struct ggml_metal_context * ctx_metal;
    #endif
    std::map<std::string, struct ggml_tensor *> tensors;
};

static bool kv_cache_init(
        const struct mpt_hparams & hparams,
             struct replit_kv_cache & cache,
                         ggml_type   wtype,
                               int   n_ctx) {
    const int n_embd  = hparams.n_embd;
    const int n_layer = hparams.n_layer;

    const int64_t n_mem      = (int64_t)n_layer*n_ctx;
    const int64_t n_elements = n_embd*n_mem;
    cache.buf.resize(2u*n_elements*ggml_type_size(wtype) + 2_MiB);
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

// load the model's weights from a stream
bool replit_model_load(const std::string & fname, std::istream &fin, replit_model & model, replit_tokenizer & vocab) {
    printf("%s: loading model from '%s' - please wait ...\n", __func__, fname.c_str());

    // verify magic
    {
        uint32_t magic;
        fin.read((char *)&magic, sizeof(magic));
        if (magic != 0x7265706c) {
            fprintf(stderr, "%s: invalid model file '%s' (bad magic)\n", __func__, fname.c_str());
            return false;
        }
    }

    // load hparams
    {
        auto & hparams = model.hparams;
        fin.read((char *) &hparams.n_vocab,     sizeof(hparams.n_vocab));
        fin.read((char *) &hparams.n_ctx,       sizeof(hparams.n_ctx));
        fin.read((char *) &hparams.n_embd,      sizeof(hparams.n_embd));
        fin.read((char *) &hparams.n_head,      sizeof(hparams.n_head));
        fin.read((char *) &hparams.n_layer,     sizeof(hparams.n_layer));
        fin.read((char *) &hparams.ftype,       sizeof(hparams.ftype));

        const int32_t qntvr = hparams.ftype / GGML_QNT_VERSION_FACTOR;
        printf("%s: n_vocab    = %d\n", __func__, hparams.n_vocab);
        printf("%s: n_ctx      = %d\n", __func__, hparams.n_ctx);
        printf("%s: n_embd     = %d\n", __func__, hparams.n_embd);
        printf("%s: n_head     = %d\n", __func__, hparams.n_head);
        printf("%s: n_layer    = %d\n", __func__, hparams.n_layer);
        printf("%s: ftype      = %d\n", __func__, hparams.ftype);
        printf("%s: qntvr      = %d\n", __func__, qntvr);

        hparams.ftype %= GGML_QNT_VERSION_FACTOR;
    }

    // load vocab
    replit_tokenizer_load(vocab, fin, model.hparams.n_vocab);

    // for the big tensors, we have the option to store the data in 16-bit
    // floats or quantized in order to save memory and also to speed up the
    // computation
    ggml_type wtype = GGML_TYPE_COUNT;
    switch (model.hparams.ftype) {
        case 0: wtype = GGML_TYPE_F32;  break;
        case 1: wtype = GGML_TYPE_F16;  break;
        case 2: wtype = GGML_TYPE_Q4_0; break;
        case 3: wtype = GGML_TYPE_Q4_1; break;
        default:
                {
                    fprintf(stderr, "%s: invalid model file '%s' (bad f16 value %d)\n",
                            __func__, fname.c_str(), model.hparams.ftype);
                    return false;
                }
    }

    auto & ctx = model.ctx;

    size_t ctx_size = 0;

    {
        const auto & hparams = model.hparams;

        const int n_embd = hparams.n_embd;
        const int n_layer = hparams.n_layer;
        const int n_ctx = hparams.n_ctx;
        const int n_vocab = hparams.n_vocab;

        ctx_size += n_embd * n_vocab * ggml_type_sizef(wtype); // wte_weight
        ctx_size += n_embd * ggml_type_sizef(GGML_TYPE_F32);   // ln_f_weight

        ctx_size += n_layer * (n_embd * ggml_type_sizef(GGML_TYPE_F32));      // ln_1_weight
        ctx_size += n_layer * (3 * n_embd * n_embd * ggml_type_sizef(wtype)); // attn_Wqkv_weight
        ctx_size += n_layer * (n_embd * n_embd * ggml_type_sizef(wtype));     // attn_out_proj_weight
        ctx_size += n_layer * (n_embd * ggml_type_sizef(GGML_TYPE_F32));      // ln_2_weight
        ctx_size += n_layer * (4 * n_embd * n_embd * ggml_type_sizef(wtype)); // mlp_mlp_up_weight
        ctx_size += n_layer * (n_embd * n_embd * 4 * ggml_type_sizef(wtype)); // mlp_mlp_down_weight

        ctx_size += n_ctx * n_layer * n_embd * ggml_type_sizef(GGML_TYPE_F16); // memory_k
        ctx_size += n_ctx * n_layer * n_embd * ggml_type_sizef(GGML_TYPE_F16); // memory_v

        ctx_size += (1 + 6 * n_layer) * 512; // object overhead

        printf("%s: ggml ctx size = %6.2f MB\n", __func__, ctx_size / (1024.0 * 1024.0));
    }

    // create the ggml context
    {
        struct ggml_init_params params = {
            .mem_size = ctx_size,
            .mem_buffer = NULL,
            .no_alloc = false,
        };

        model.ctx = ggml_init(params);
        if (!model.ctx) {
            fprintf(stderr, "%s: ggml_init() failed\n", __func__);
            return false;
        }
    }

    // prepare memory for the weights
    {
        const auto & hparams = model.hparams;

        const int n_embd = hparams.n_embd;
        const int n_layer = hparams.n_layer;
        const int n_vocab = hparams.n_vocab;

        model.layers.resize(n_layer);

        model.wte_weight = ggml_new_tensor_2d(ctx, wtype, n_embd, n_vocab);
        model.ln_f_weight = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embd);

        // map by name
        model.tensors["transformer.wte.weight"] = model.wte_weight;
        model.tensors["transformer.ln_f.weight"] = model.ln_f_weight;

        for (int i = 0; i < n_layer; ++i) {
            auto & layer = model.layers[i];

            layer.ln_1_weight = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embd);
            layer.c_attn_wqkv_weight = ggml_new_tensor_2d(ctx, wtype, n_embd, 3 * n_embd);
            layer.c_attn_out_proj_weight = ggml_new_tensor_2d(ctx, wtype, n_embd, n_embd);
            layer.ln_2_weight = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embd);
            layer.c_mlp_mlp_up_weight = ggml_new_tensor_2d(ctx, wtype, n_embd, 4 * n_embd);
            layer.c_mlp_mlp_down_weight = ggml_new_tensor_2d(ctx, wtype, 4 * n_embd, n_embd);

            // map by name
            model.tensors["transformer.blocks." + std::to_string(i) + ".ln_1.weight"] = layer.ln_1_weight;
            model.tensors["transformer.blocks." + std::to_string(i) + ".attn.Wqkv.weight"] = layer.c_attn_wqkv_weight;
            model.tensors["transformer.blocks." + std::to_string(i) + ".attn.out_proj.weight"] =
                layer.c_attn_out_proj_weight;
            model.tensors["transformer.blocks." + std::to_string(i) + ".ln_2.weight"] = layer.ln_2_weight;
            model.tensors["transformer.blocks." + std::to_string(i) + ".mlp.mlp_up.weight"] = layer.c_mlp_mlp_up_weight;
            model.tensors["transformer.blocks." + std::to_string(i) + ".mlp.mlp_down.weight"] =
                layer.c_mlp_mlp_down_weight;
        }
    }

    // key + value memory
    {
        const auto & hparams = model.hparams;

        const int n_layer = hparams.n_layer;
        const int n_ctx = hparams.n_ctx;

        const int64_t n_mem = n_layer * n_ctx;

        if (!kv_cache_init(hparams, model.kv_self, GGML_TYPE_F16, model.hparams.n_ctx)) {
            fprintf(stderr, "%s: kv_cache_init() failed for self-attention cache\n", __func__);
            ggml_free(ctx);
            return false;
        }

        const size_t memory_size = ggml_nbytes(model.kv_self.k) + ggml_nbytes(model.kv_self.v);

        printf("%s: memory_size = %8.2f MB, n_mem = %lld\n", __func__, memory_size / 1024.0 / 1024.0, n_mem);
    }

    // load weights
    {
        int n_tensors = 0;
        size_t total_size = 0;

        printf("%s: ", __func__);

        while (true) {
            int32_t n_dims;
            int32_t length;
            int32_t ttype;

            fin.read(reinterpret_cast<char *>(&n_dims), sizeof(n_dims));
            fin.read(reinterpret_cast<char *>(&length), sizeof(length));
            fin.read(reinterpret_cast<char *>(&ttype), sizeof(ttype));

            if (fin.eof()) {
                break;
            }

            int32_t nelements = 1;
            int32_t ne[2] = {1, 1};
            for (int i = 0; i < n_dims; ++i) {
                fin.read(reinterpret_cast<char *>(&ne[i]), sizeof(ne[i]));
                nelements *= ne[i];
            }

            std::string name(length, 0);
            fin.read(&name[0], length);

            if (model.tensors.find(name.data()) == model.tensors.end()) {
                fprintf(stderr, "%s: unknown tensor '%s' in model file\n", __func__, name.data());
                return false;
            }

            auto tensor = model.tensors[name.data()];
            if (ggml_nelements(tensor) != nelements) {
                fprintf(stderr, "%s: tensor '%s' has wrong size in model file\n", __func__, name.data());
                return false;
            }

            if (tensor->ne[0] != ne[0] || tensor->ne[1] != ne[1]) {
                fprintf(stderr,
                        "%s: tensor '%s' has wrong shape in model file: got [%5d, "
                        "%5d], expected [%5d, %5d]\n",
                        __func__, name.data(), (int)tensor->ne[0], (int)tensor->ne[1], ne[0], ne[1]);
                return false;
            }

            // for debugging
            if (0) {
                printf("%24s - [%5d, %5d], type = %6s, %6.2f MB, %9zu bytes\n", name.data(), ne[0], ne[1],
                       ggml_type_name(ggml_type(ttype)), ggml_nbytes(tensor) / 1024.0 / 1024.0, ggml_nbytes(tensor));
            }

            const size_t bpe = ggml_type_size(ggml_type(ttype));

            if ((nelements * bpe) / ggml_blck_size(tensor->type) != ggml_nbytes(tensor)) {
                fprintf(stderr,
                        "%s: tensor '%s' has wrong size in model file: got %zu, "
                        "expected %zu\n",
                        __func__, name.data(), ggml_nbytes(tensor), nelements * bpe);
                return false;
            }

            fin.read(reinterpret_cast<char *>(tensor->data), ggml_nbytes(tensor));

            total_size += ggml_nbytes(tensor);
            if (++n_tensors % 8 == 0) {
                printf(".");
                fflush(stdout);
            }
        }

        printf(" done\n");

        printf("%s: model size = %8.2f MB / num tensors = %d\n", __func__, total_size / 1024.0 / 1024.0, n_tensors);
    }

   model.eval_buf_size = 256u * 1024 * 1024;
   model.eval_buf = malloc(model.eval_buf_size);
   model.scr0_buf_size = 256u * 1024 * 1024;
   model.scr0_buf = malloc(model.scr0_buf_size);
   model.scr1_buf_size = 256u * 1024 * 1024;
   model.scr1_buf = malloc(model.scr1_buf_size);

#ifdef GGML_USE_METAL
    model.ctx_metal = ggml_metal_init();
    void* data_ptr = ggml_get_mem_buffer(model.ctx);
    size_t data_size = ggml_get_mem_size(model.ctx);

    #define GGML_CHECK_BUF(result) if (!(result)) {                     \
        std::cerr << __func__ << ": failed to add buffer" << std::endl; \
        ggml_free(model.ctx);                                           \
        return false;                                                   \
    }

    GGML_CHECK_BUF(ggml_metal_add_buffer(model.ctx_metal, "data", data_ptr, data_size));
    GGML_CHECK_BUF(ggml_metal_add_buffer(model.ctx_metal, "kv", ggml_get_mem_buffer(model.kv_self.ctx), 
                                                                ggml_get_mem_size(model.kv_self.ctx)));
    GGML_CHECK_BUF(ggml_metal_add_buffer(model.ctx_metal, "eval", model.eval_buf, model.eval_buf_size));
    GGML_CHECK_BUF(ggml_metal_add_buffer(model.ctx_metal, "scr0", model.scr0_buf, model.scr0_buf_size));
    GGML_CHECK_BUF(ggml_metal_add_buffer(model.ctx_metal, "scr1", model.scr1_buf, model.scr1_buf_size));
#endif

    return true;
}

// load the model's weights from a file path
bool replit_model_load(const std::string & fname, replit_model & model, replit_tokenizer & vocab) {

    auto fin = std::ifstream(fname, std::ios::binary);
    if (!fin) {
        fprintf(stderr, "%s: failed to open '%s'\n", __func__, fname.c_str());
        return false;
    }

    bool loaded = replit_model_load(fname, fin, model, vocab);
    fin.close();
    return loaded;
}

// evaluate the transformer
//
//   - model:     the model
//   - n_threads: number of threads to use
//   - n_past:    the context size so far
//   - embd_inp:  the embeddings of the tokens in the context
//   - embd_w:    the predicted logits for the next token
//
bool replit_eval(const replit_model & model, const int n_threads, const int n_past,
                 const std::vector<gpt_vocab::id> & embd_inp, std::vector<float> & embd_w, size_t & mem_per_token) {
    const int N = embd_inp.size();

    const auto & hparams = model.hparams;

    const int n_embd = hparams.n_embd;
    const int n_layer = hparams.n_layer;
    const int n_ctx = hparams.n_ctx;
    const int n_head = hparams.n_head;
    const int n_vocab = hparams.n_vocab;

   struct ggml_init_params eval_ctx_params = {
        .mem_size = model.eval_buf_size,
        .mem_buffer = model.eval_buf,
        .no_alloc = false,
    };
    struct ggml_context * ctx0 = ggml_init(eval_ctx_params);
    struct ggml_cgraph gf = {.n_threads = n_threads};

    struct ggml_tensor * embd = ggml_new_tensor_1d(ctx0, GGML_TYPE_I32, N);
    memcpy(embd->data, embd_inp.data(), N * ggml_element_size(embd));

    struct ggml_tensor * inpL = ggml_get_rows(ctx0, model.wte_weight, embd);

    for (int il = 0; il < n_layer; ++il) {
        ggml_set_scratch(ctx0, {0, model.scr0_buf_size, model.scr0_buf, });
        struct ggml_tensor * cur;

        // a = self.ln_1(x)
        {
            cur = ggml_norm(ctx0, inpL);

            cur = ggml_mul(ctx0, ggml_repeat(ctx0, model.layers[il].ln_1_weight, cur), cur);
        }

        // self-attention
        //  b, _, past_key_value = self.attn(a, past_key_value=past_key_value,
        //  attn_bias=attn_bias, attention_mask=attention_mask,
        //  is_causal=is_causal)
        {

            // compute QKV
            { cur = ggml_mul_mat(ctx0, model.layers[il].c_attn_wqkv_weight, cur); }

            struct ggml_tensor * Qcur = ggml_view_2d(ctx0, cur, n_embd, N, cur->nb[1], 0 * sizeof(float) * n_embd);
            struct ggml_tensor * Kcur = ggml_view_2d(ctx0, cur, n_embd, N, cur->nb[1], 1 * sizeof(float) * n_embd);
            struct ggml_tensor * Vcur = ggml_view_2d(ctx0, cur, n_embd, N, cur->nb[1], 2 * sizeof(float) * n_embd);

            // store key and value to memory
            {
                struct ggml_tensor * k =
                    ggml_view_1d(ctx0, model.kv_self.k, N * n_embd,
                                 (ggml_element_size(model.kv_self.k) * n_embd) * (il * n_ctx + n_past));
                struct ggml_tensor * v =
                    ggml_view_1d(ctx0, model.kv_self.v, N * n_embd,
                                 (ggml_element_size(model.kv_self.v) * n_embd) * (il * n_ctx + n_past));

                ggml_build_forward_expand(&gf, ggml_cpy(ctx0, Kcur, k));
                ggml_build_forward_expand(&gf, ggml_cpy(ctx0, Vcur, v));
            }

            // Q = Qcur.contiguous().view(n_embd/n_head, n_head, N).permute(0,
            // 2, 1, 3) [64, N, 12]
            struct ggml_tensor * Q = ggml_permute(
                ctx0, ggml_cpy(ctx0, Qcur, ggml_new_tensor_3d(ctx0, GGML_TYPE_F32, n_embd / n_head, n_head, N)), 0, 2,
                1, 3);

            // K = Kmem.view(n_embd/n_head, n_head, n_past + N).permute(0, 2, 1,
            // 3) [64, n_past + N, 12]
            struct ggml_tensor * K =
                ggml_permute(ctx0,
                             ggml_reshape_3d(ctx0,
                                             ggml_view_1d(ctx0, model.kv_self.k, (n_past + N) * n_embd,
                                                          il * n_ctx * ggml_element_size(model.kv_self.v) * n_embd),
                                             n_embd / n_head, n_head, n_past + N),
                             0, 2, 1, 3);
            // K * Q
            struct ggml_tensor * KQ = ggml_mul_mat(ctx0, K, Q);

            // KQ_scaled = KQ / sqrt(n_embd/n_head)
            struct ggml_tensor * KQ_scaled =
                ggml_scale(ctx0, KQ, ggml_new_f32(ctx0, 1.0f / sqrt(float(n_embd) / n_head)));

            // Alibi
            struct ggml_tensor * KQ_scaled_alibi = ggml_alibi(ctx0, KQ_scaled, n_past, n_head, 8.0f);

            // KQ_masked = mask_past(KQ_scaled)
            struct ggml_tensor * KQ_masked = ggml_diag_mask_inf(ctx0, KQ_scaled_alibi, n_past);

            // KQ = soft_max(KQ_masked)
            struct ggml_tensor * KQ_soft_max = ggml_soft_max(ctx0, KQ_masked);

            // V_trans = Vmem.view(n_embd/n_head, n_head, n_past + N).permute(1,
            // 2, 0, 3).contiguous() [n_past + N, 64, 12]
            struct ggml_tensor * V_trans = ggml_cpy(
                ctx0,
                ggml_permute(ctx0,
                             ggml_reshape_3d(ctx0,
                                             ggml_view_1d(ctx0, model.kv_self.v, (n_past + N) * n_embd,
                                                          il * n_ctx * ggml_element_size(model.kv_self.v) * n_embd),
                                             n_embd / n_head, n_head, n_past + N),
                             1, 2, 0, 3),
                ggml_new_tensor_3d(ctx0, model.kv_self.v->type, n_past + N, n_embd / n_head, n_head));

            // KQV = transpose(V) * KQ_soft_max
            struct ggml_tensor * KQV = ggml_mul_mat(ctx0, V_trans, KQ_soft_max);

            // KQV_merged = KQV.permute(0, 2, 1, 3)
            struct ggml_tensor * KQV_merged = ggml_permute(ctx0, KQV, 0, 2, 1, 3);

            // cur = KQV_merged.contiguous().view(n_embd, N)
            cur = ggml_cpy(ctx0, KQV_merged, ggml_new_tensor_2d(ctx0, GGML_TYPE_F32, n_embd, N));

            // projection
            { cur = ggml_mul_mat(ctx0, model.layers[il].c_attn_out_proj_weight, cur); }
        }
        ggml_set_scratch(ctx0, {0, model.scr1_buf_size, model.scr1_buf, });

        inpL = ggml_add(ctx0, inpL, cur);

        // m = self.ln_2(x)
        {
            cur = ggml_norm(ctx0, inpL);

            cur = ggml_mul(ctx0, ggml_repeat(ctx0, model.layers[il].ln_2_weight, cur), cur);
        }

        // n = self.mlp(m)
        {

            cur = ggml_mul_mat(ctx0, model.layers[il].c_mlp_mlp_up_weight, cur);

            // GELU activation
            cur = ggml_gelu(ctx0, cur);

            // projection
            // cur = proj_w*cur + proj_b
            cur = ggml_mul_mat(ctx0, model.layers[il].c_mlp_mlp_down_weight, cur);
        }

        // x = x + n
        inpL = ggml_add(ctx0, inpL, cur);
    }
    ggml_set_scratch(ctx0, {0, model.scr0_buf_size, model.scr0_buf, });
    // norm
    {
        inpL = ggml_norm(ctx0, inpL);
        // inpL = ln_f_g*inpL
        inpL = ggml_mul(ctx0, ggml_repeat(ctx0, model.ln_f_weight, inpL), inpL);
    }

    ggml_set_scratch(ctx0, {0, 0, nullptr, });
    // output embedding weight tied to input embedding
    inpL = ggml_mul_mat(ctx0, model.wte_weight, inpL);

    // logits -> probs
    // inpL = ggml_soft_max(ctx0, inpL);

    // run the computation
    ggml_build_forward_expand(&gf, inpL);
#ifdef GGML_USE_METAL
    if (N == 1) {
        // llama.cpp doesn't use metal for batch/prompt processing presently
        // pending changes to the metal matmul kernel - only use it for generation (N=1)
        ggml_metal_graph_compute(model.ctx_metal, &gf);
        ggml_metal_get_tensor(model.ctx_metal, inpL);
    } else {
        // We need to sync the GPU KV cache with the CPU KV cache
        ggml_metal_get_tensor(model.ctx_metal, model.kv_self.k);
        ggml_metal_get_tensor(model.ctx_metal, model.kv_self.v);

        ggml_graph_compute(ctx0, &gf);
    }
#else
    ggml_graph_compute(ctx0, &gf);
#endif

    // std::cout << "Qcur" << std::endl;
    // print_tensor(Qcur);

    // if (n_past%100 == 0) {
    // ggml_graph_print(&gf);
    // ggml_graph_dump_dot(&gf, NULL, "replit-model.dot");
    // }

    // return result for just the last token
    embd_w.resize(n_vocab);
    memcpy(embd_w.data(), (float *)ggml_get_data(inpL) + (n_vocab * (N - 1)), sizeof(float) * n_vocab);

    if (mem_per_token == 0) {
        mem_per_token = ggml_used_mem(ctx0) / N;
    }
    // printf("used_mem = %zu\n", ggml_used_mem(ctx0));

    ggml_free(ctx0);

    return true;
}


#define REPLIT_MAX_RNG_STATE 64*1024

size_t replit_get_state_size(const replit_model &model)
{
    // we don't know size of rng until we actually serialize it. so reserve more than enough memory for its serialized state.
    // for reference, std::mt19937(1337) serializes to 6701 bytes.
    const size_t s_rng_size        = sizeof(size_t);
    const size_t s_rng             = REPLIT_MAX_RNG_STATE;
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

size_t replit_copy_state_data(const replit_model &model, const std::mt19937 &rng, uint8_t *dest)
{
    uint8_t * out = dest;
    fflush(stdout);
    // copy rng
    {
        std::stringstream rng_ss;
        rng_ss << rng;

        const size_t rng_size = rng_ss.str().size();
        char rng_buf[REPLIT_MAX_RNG_STATE];

        memset(&rng_buf[0], 0, REPLIT_MAX_RNG_STATE);
        memcpy(&rng_buf[0], rng_ss.str().data(), rng_ss.str().size());

        memcpy(out, &rng_size,   sizeof(rng_size));   out += sizeof(rng_size);
        memcpy(out, &rng_buf[0], REPLIT_MAX_RNG_STATE); out += REPLIT_MAX_RNG_STATE;
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
    const size_t expected = replit_get_state_size(model);
    assert(written == expected);
    fflush(stdout);
    return written;
}

size_t replit_set_state_data(replit_model *model, std::mt19937 *rng, const uint8_t *src)
{
    const uint8_t * in = src;

    // set rng
    {
        size_t rng_size;
        char   rng_buf[REPLIT_MAX_RNG_STATE];

        memcpy(&rng_size,   in, sizeof(rng_size));    in += sizeof(rng_size);
        memcpy(&rng_buf[0], in, REPLIT_MAX_RNG_STATE); in += REPLIT_MAX_RNG_STATE;

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
    const size_t expected = replit_get_state_size(*model);
    assert(nread == expected);
    fflush(stdout);
    return nread;
}

struct ReplitPrivate {
    const std::string modelPath;
    bool modelLoaded;
    replit_tokenizer vocab;
    replit_model *model = nullptr;
    int64_t n_threads = 0;
    size_t mem_per_token = 0;
    std::mt19937 rng;
    bool has_end_of_text = false;
};

Replit::Replit()
    : d_ptr(new ReplitPrivate) {

    d_ptr->model = new replit_model;
    d_ptr->model->ctx = nullptr;
    d_ptr->modelLoaded = false;
}

bool Replit::loadModel(const std::string &modelPath) {
    std::mt19937 rng(time(NULL));
    d_ptr->rng = rng;

    auto fin = std::ifstream(modelPath, std::ios::binary);

    // load the model
    if (!replit_model_load(modelPath, fin, *d_ptr->model, d_ptr->vocab)) {
        std::cerr << "Replit ERROR: failed to load model from " <<  modelPath;
        return false;
    }

    d_ptr->n_threads = std::min(4, (int32_t) std::thread::hardware_concurrency());
    d_ptr->modelLoaded = true;
    d_ptr->has_end_of_text = d_ptr->vocab.raw_vocab.token_to_id.find("<|endoftext|>") != d_ptr->vocab.raw_vocab.token_to_id.end();
    fflush(stdout);
    return true;
}

void Replit::setThreadCount(int32_t n_threads) {
    d_ptr->n_threads = n_threads;
}

int32_t Replit::threadCount() const
{
    return d_ptr->n_threads;
}

Replit::~Replit()
{
    if(d_ptr->model->ctx) {
        ggml_free(d_ptr->model->ctx);
        d_ptr->model->ctx = nullptr;
    }
    if(d_ptr->model->eval_buf) {
        free(d_ptr->model->eval_buf);
    }
    if(d_ptr->model->scr0_buf) {
        free(d_ptr->model->scr0_buf);
    }
    if(d_ptr->model->scr1_buf) {
        free(d_ptr->model->scr1_buf);
    }
    delete d_ptr->model;
}

bool Replit::isModelLoaded() const
{
    return d_ptr->modelLoaded;
}

size_t Replit::stateSize() const
{
    return replit_get_state_size(*d_ptr->model);
}

size_t Replit::saveState(uint8_t *dest) const
{
    return replit_copy_state_data(*d_ptr->model, d_ptr->rng, dest);
}

size_t Replit::restoreState(const uint8_t *src)
{
    return replit_set_state_data(d_ptr->model, &d_ptr->rng, src);
}

std::vector<LLModel::Token> Replit::tokenize(PromptContext &, const std::string &str) const
{
    return replit_tokenizer_tokenize(d_ptr->vocab, str);
}

std::string Replit::tokenToString(LLModel::Token id) const
{
    return replit_tokenizer_detokenize(d_ptr->vocab, {id});
}

LLModel::Token Replit::sampleToken(PromptContext &promptCtx) const
{
    const size_t n_prev_toks = std::min((size_t) promptCtx.repeat_last_n, promptCtx.tokens.size());
    return gpt_sample_top_k_top_p(d_ptr->model->hparams.n_vocab,
        promptCtx.tokens.data() + promptCtx.tokens.size() - n_prev_toks,
        n_prev_toks,
        promptCtx.logits,
        promptCtx.top_k, promptCtx.top_p, promptCtx.temp,
        promptCtx.repeat_penalty,
        d_ptr->rng);
}

bool Replit::evalTokens(PromptContext &ctx, const std::vector<int32_t> &tokens) const
{
    return replit_eval(*d_ptr->model, d_ptr->n_threads, ctx.n_past, tokens, ctx.logits, d_ptr->mem_per_token);
}

int32_t Replit::contextLength() const
{
    return d_ptr->model->hparams.n_ctx;
}

const std::vector<LLModel::Token> &Replit::endTokens() const
{
    static const std::vector<LLModel::Token> fres = {0, d_ptr->vocab.raw_vocab.token_to_id["<|endoftext|>"]};
    return fres;
}

#if defined(_WIN32)
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT __attribute__ ((visibility ("default")))
#endif

extern "C" {
DLL_EXPORT bool is_g4a_backend_model_implementation() {
    return true;
}

DLL_EXPORT const char *get_model_type() {
    return modelType_;
}

DLL_EXPORT const char *get_build_variant() {
    return GGML_BUILD_VARIANT;
}

DLL_EXPORT bool magic_match(std::istream& f) {
    uint32_t magic = 0;
    f.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != 0x7265706c) return false;
    #ifdef GGML_USE_METAL
    off_t offset = sizeof(uint32_t) * 5; // n_vocab, n_ctx, n_embd, n_head, n_layer
    f.seekg(offset, std::ios_base::cur);
    uint32_t ftype;
    f.read(reinterpret_cast<char*>(&ftype), sizeof(ftype)); // ftype
    const int32_t qntvr = ftype / GGML_QNT_VERSION_FACTOR;
    ftype %= GGML_QNT_VERSION_FACTOR;
    switch (ftype) {
        case 1: return true; // GGML_TYPE_F16
        case 2: // GGML_TYPE_Q4_0
            if (qntvr != GGML_QNT_VERSION)
            {
                std::cerr << "replit: not using metal (unsupported qnt ver)" << std::endl;
                return false;
            }
            return true;
        default: return false;
    }
    #else
    return true;
    #endif
}

DLL_EXPORT LLModel *construct() {
    return new Replit;
}
}
