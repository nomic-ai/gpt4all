#define MPT_H_I_KNOW_WHAT_I_AM_DOING_WHEN_INCLUDING_THIS_FILE
#include "mpt_impl.h"

#include "utils.h"
#include "llmodel_shared.h"

#include <cassert>
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <random>
#include <string>
#include <vector>
#include <iostream>
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
#include <sstream>
#include <thread>
#include <unordered_set>
#include <regex>
#include <ggml.h>


namespace {
const char *modelType_ = "MPT";
}

// default hparams (MPT 7B)
struct mpt_hparams {
    int32_t n_vocab      = 50432;
    int32_t n_ctx        = 2048;
    int32_t n_embd       = 4096;
    int32_t n_head       = 32;
    int32_t n_layer      = 32;
    float alibi_bias_max = 8;
    float clip_qkv       = 0;
    float norm_eps       = 1e-5;
    int32_t expand       = 4;
};

struct mpt_layer {
    // normalization
    struct ggml_tensor * norm_1_w;
    struct ggml_tensor * norm_2_w;

    // attention
    struct ggml_tensor * attn_Wqkv_w;
    struct ggml_tensor * attn_out_proj_w;

    // ff
    struct ggml_tensor * ffn_up_proj_w;
    struct ggml_tensor * ffn_down_proj_w;
};

struct mpt_model {
    mpt_hparams hparams;

    // normalization
    struct ggml_tensor * norm_f_w;

    struct ggml_tensor * wte; // position embedding

    // mpt does weight tying

    std::vector<mpt_layer> layers;

    struct llm_kv_cache kv_self;
    struct ggml_context * ctx;


    llm_buffer eval_buf;
    llm_buffer scr0_buf;
    llm_buffer scr1_buf;

    ~mpt_model() {
        if (ctx) {
            ggml_free(ctx);
        }
    }
};

enum mpt_token_type {
    MPT_TOKEN_TYPE_NORMAL  = 1,
    MPT_TOKEN_TYPE_CONTROL = 3,
};

using replit_piece_t = std::pair<std::size_t, float>;
using replit_piece_map_t = std::unordered_map<std::string, replit_piece_t>;

static const std::string replit_ws_symbol = "\342\226\201";

struct mpt_vocab {
    bool is_replit = false;
    gpt_vocab raw;
    replit_piece_map_t piece_map;
    std::vector<std::string> vocab;

    const char * end_of_text() const {
        return is_replit ? "<|endoftext|>" : "<|im_end|>";
    }
};

std::pair<std::vector<LLModel::Token>, float> encode_word(const std::string & word, const replit_piece_map_t & model) {
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

bool replit_tokenizer_load(mpt_vocab & tokenizer, gguf_context * ggufctx, int tokens_keyidx, int max_vocab_size) {
    int scores_keyidx = gguf_find_key(ggufctx, "tokenizer.ggml.scores");
    if (scores_keyidx == -1) {
        fprintf(stderr, "%s: llama token scores not found!\n", __func__);
        return false;
    }
    const auto *scores = reinterpret_cast<const float *>(gguf_get_arr_data(ggufctx, scores_keyidx));

    for (LLModel::Token i = 0; i < max_vocab_size; i++) {
        std::string word = gguf_get_arr_str(ggufctx, tokens_keyidx, i);
        tokenizer.piece_map[word] = std::make_pair(i, -scores[i]);
        tokenizer.raw.id_to_token[i] = word;
        tokenizer.raw.token_to_id[word] = i;
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

std::vector<LLModel::Token> replit_tokenizer_tokenize(mpt_vocab & tokenizer, const std::string & text) {
    std::vector<LLModel::Token> tokens;
    auto normalized_text = replace_all(text, " ", replit_ws_symbol);
    auto tokenized = encode_word(normalized_text, tokenizer.piece_map);

    return tokenized.first;
}

std::string replit_tokenizer_detokenize(mpt_vocab & tokenizer, const std::vector<LLModel::Token> & tokens) {
    std::string text;
    for (auto token : tokens) {
        text += tokenizer.raw.id_to_token[token];
    }
    return replace_all(text, replit_ws_symbol, " ");
}


static bool kv_cache_init(
        const struct mpt_hparams & hparams,
             struct llm_kv_cache & cache,
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

// load the model's weights from a file path. if mem_req ptr is passed the model is
// only partially parsed to estimate required memory
bool mpt_model_load(const std::string &fname, mpt_model & model, mpt_vocab & vocab, size_t * mem_req) {
    printf("%s: loading model from '%s' - please wait ...\n", __func__, fname.c_str());
    if (mem_req != nullptr) {
        *mem_req = 0;
    }

    // create the ggml context
    struct gguf_init_params params = {
        /*.no_alloc = */ false,
        /*.ctx      = */ &model.ctx,
    };

    gguf_context *ggufctx = gguf_init_from_file(fname.c_str(), params);
    if (!ggufctx) {
        fprintf(stderr, "%s: gguf_init_from_file() failed\n", __func__);
        return false;
    }

    printf("%s: gguf version     = %d\n", __func__, gguf_get_version(ggufctx));
    printf("%s: gguf alignment   = %zu\n", __func__, gguf_get_alignment(ggufctx));
    printf("%s: gguf data offset = %zu\n", __func__, gguf_get_data_offset(ggufctx));

    // print some standard metadata
    {
        int keyidx;

        keyidx = gguf_find_key(ggufctx, "general.name");
        if (keyidx != -1) { printf("%s: model name           = %s\n", __func__, gguf_get_val_str(ggufctx, keyidx)); }
        keyidx = gguf_find_key(ggufctx, "general.description");
        if (keyidx != -1) { printf("%s: model description    = %s\n", __func__, gguf_get_val_str(ggufctx, keyidx)); }
        keyidx = gguf_find_key(ggufctx, "general.author");
        if (keyidx != -1) { printf("%s: model author         = %s\n", __func__, gguf_get_val_str(ggufctx, keyidx)); }
        keyidx = gguf_find_key(ggufctx, "general.license");
        if (keyidx != -1) { printf("%s: model license        = %s\n", __func__, gguf_get_val_str(ggufctx, keyidx)); }
        keyidx = gguf_find_key(ggufctx, "general.architecture");
        if (keyidx != -1) { printf("%s: model architecture   = %s\n", __func__, gguf_get_val_str(ggufctx, keyidx)); }
        keyidx = gguf_find_key(ggufctx, "general.file_type");
        if (keyidx != -1) { printf("%s: model file type      = %" PRIu32 "\n", __func__, gguf_get_val_u32(ggufctx, keyidx)); }
        keyidx = gguf_find_key(ggufctx, "gptneox.tensor_data_layout");
        if (keyidx != -1) { printf("%s: model data layout    = %s\n", __func__, gguf_get_val_str(ggufctx, keyidx)); }
        keyidx = gguf_find_key(ggufctx, "general.source.huggingface.repository");
        if (keyidx != -1) { printf("%s: model source HF repo = %s\n", __func__, gguf_get_val_str(ggufctx, keyidx)); }
    }

    // check required metadata
    {
        // check model architecture kv
        int keyidx = gguf_find_key(ggufctx, "general.architecture");
        if (keyidx == -1) {
            fprintf(stderr, "%s: gguf model architecture not found!\n", __func__);
            return false;
        }
        if (strcmp(gguf_get_val_str(ggufctx, keyidx), "mpt") != 0) {
            fprintf(stderr, "%s: model architecture not supported!\n", __func__);
            return false;
        }
    }

    // load hparams
    {
        auto & hparams = model.hparams;

        bool ok = false;
        int keyidx;

        do {
            keyidx = gguf_find_key(ggufctx, "mpt.context_length");
            if (keyidx == -1) { break; }
            hparams.n_ctx = gguf_get_val_u32(ggufctx, keyidx);

            keyidx = gguf_find_key(ggufctx, "mpt.embedding_length");
            if (keyidx == -1) { break; }
            hparams.n_embd = gguf_get_val_u32(ggufctx, keyidx);

            keyidx = gguf_find_key(ggufctx, "mpt.attention.head_count");
            if (keyidx == -1) { break; }
            hparams.n_head = gguf_get_val_u32(ggufctx, keyidx);

            keyidx = gguf_find_key(ggufctx, "mpt.block_count");
            if (keyidx == -1) { break; }
            hparams.n_layer = gguf_get_val_u32(ggufctx, keyidx);

            keyidx = gguf_find_key(ggufctx, "mpt.attention.max_alibi_bias");
            if (keyidx == -1) { break; }
            hparams.alibi_bias_max = gguf_get_val_f32(ggufctx, keyidx);

            keyidx = gguf_find_key(ggufctx, "mpt.attention.clamp_kqv");
            if (keyidx != -1) { // optional
                hparams.clip_qkv = gguf_get_val_f32(ggufctx, keyidx);
            }

            keyidx = gguf_find_key(ggufctx, "mpt.attention.layer_norm_epsilon");
            if (keyidx == -1) { break; }
            hparams.norm_eps = gguf_get_val_f32(ggufctx, keyidx);

            ok = true;
        } while (false);

        if (!ok) {
            fprintf(stderr, "%s: required hparam missing!\n", __func__);
            return false;
        }

        printf("%s: n_ctx          = %d\n", __func__, hparams.n_ctx);
        printf("%s: n_embd         = %d\n", __func__, hparams.n_embd);
        printf("%s: n_head         = %d\n", __func__, hparams.n_head);
        printf("%s: n_layer        = %d\n", __func__, hparams.n_layer);
        printf("%s: alibi_bias_max = %f\n", __func__, hparams.alibi_bias_max);
        printf("%s: clip_qkv       = %f\n", __func__, hparams.clip_qkv);
    }

    // load vocab
    {
        auto & hparams = model.hparams;

        int tokens_keyidx = gguf_find_key(ggufctx, "tokenizer.ggml.tokens");
        if (tokens_keyidx == -1) {
            fprintf(stderr, "%s: tokenizer vocab not found!\n", __func__);
            return false;
        }

        int keyidx = gguf_find_key(ggufctx, "tokenizer.ggml.model");
        if (keyidx == -1) {
            fprintf(stderr, "%s: tokenizer model not found!\n", __func__);
            return false;
        }
        std::string tokenizer_model(gguf_get_val_str(ggufctx, keyidx));

        hparams.n_vocab = gguf_get_arr_n(ggufctx, tokens_keyidx);
        printf("%s: %s tokenizer vocab = %d\n", __func__, tokenizer_model.c_str(), int(hparams.n_vocab));

        if (tokenizer_model == "llama") { // Replit
            vocab.is_replit = true;
            if (!replit_tokenizer_load(vocab, ggufctx, tokens_keyidx, hparams.n_vocab)) {
                return false;
            }
        } else if (tokenizer_model == "gpt2") {
            int toktypes_keyidx = gguf_find_key(ggufctx, "tokenizer.ggml.token_type");
            if (toktypes_keyidx == -1) {
                fprintf(stderr, "%s: gpt2 token types not found!\n", __func__);
                return false;
            }
            const auto *toktypes = reinterpret_cast<const uint32_t *>(gguf_get_arr_data(ggufctx, toktypes_keyidx));

            for (int i = 0; i < hparams.n_vocab; i++) {
                std::string word = gguf_get_arr_str(ggufctx, tokens_keyidx, i);

                bool special = false;
                if (toktypes[i] == MPT_TOKEN_TYPE_CONTROL) {
                    special = true;
                } else if (toktypes[i] != MPT_TOKEN_TYPE_NORMAL) {
                    fprintf(stderr, "%s: unknown token type: %d\n", __func__, int(toktypes[i]));
                    return false;
                }

                vocab.raw.token_to_id[word] = i;
                vocab.raw.id_to_token[i] = word;

                if (special) {
                    vocab.raw.add_special_token(word);
                }
            }
        } else {
            fprintf(stderr, "%s: tokenizer model not supported!\n", __func__);
            return false;
        }
    }

    auto & ctx = model.ctx;

    size_t ctx_size = ggml_get_mem_size(ctx);
    printf("%s: ggml ctx size = %6.2f MB\n", __func__, ctx_size / (1024.0 * 1024.0));

    if (mem_req != nullptr) {
        *mem_req = ctx_size;
        gguf_free(ggufctx);
        return false;
    }

    // prepare memory for the weights
    {
        const auto & hparams = model.hparams;
        model.layers.resize(hparams.n_layer);

        model.wte      = ggml_get_tensor(ctx, "token_embd.weight");
        model.norm_f_w = ggml_get_tensor(ctx, "output_norm.weight");

        auto name = [](int i, std::string n) {
            static std::string key;
            key = "blk." + std::to_string(i) + "." + n;
            return key.c_str();
        };

        for (int i = 0; i < hparams.n_layer; ++i) {
            auto &layer = model.layers[i];

            layer.norm_1_w        = ggml_get_tensor(ctx, name(i, "attn_norm.weight"));
            layer.norm_2_w        = ggml_get_tensor(ctx, name(i, "ffn_norm.weight"));

            layer.attn_Wqkv_w     = ggml_get_tensor(ctx, name(i, "attn_qkv.weight"));
            layer.attn_out_proj_w = ggml_get_tensor(ctx, name(i, "attn_output.weight"));
            layer.ffn_up_proj_w   = ggml_get_tensor(ctx, name(i, "ffn_up.weight"));
            layer.ffn_down_proj_w = ggml_get_tensor(ctx, name(i, "ffn_down.weight"));
        }
    }

    // key + value memory
    {
        const auto &hparams = model.hparams;
        if (!kv_cache_init(hparams, model.kv_self, GGML_TYPE_F16, model.hparams.n_ctx)) {
            fprintf(stderr, "%s: kv_cache_init() failed for self-attention cache\n", __func__);
            ggml_free(ctx);
            return false;
        }

        const size_t memory_size = ggml_nbytes(model.kv_self.k) + ggml_nbytes(model.kv_self.v);
        printf("%s: kv self size  = %7.2f MB\n", __func__, memory_size / 1024.0 / 1024.0);
    }

    model.scr0_buf.resize(256u * 1024 * 1024);
    model.scr1_buf.resize(256u * 1024 * 1024);

    return true;
}

bool mpt_eval(
        mpt_model & model,
        const int n_threads,
        const int n_past,
        const std::vector<int>           & embd_inp,
              std::vector<float>         & embd_w,
              size_t                     & mem_per_token) {
    const int N = embd_inp.size();

    const auto & hparams = model.hparams;

    const int n_embd  = hparams.n_embd;
    const int n_layer = hparams.n_layer;
    const int n_ctx   = hparams.n_ctx;
    const int n_head  = hparams.n_head;
    const int n_vocab = hparams.n_vocab;

    const size_t init_buf_size = 1024_MiB;
    if (!model.eval_buf.addr || model.eval_buf.size < init_buf_size)
        model.eval_buf.resize(init_buf_size);

    if (mem_per_token > 0 && mem_per_token*N > model.eval_buf.size) {
        const size_t buf_size_new = 1.1*(mem_per_token*N); // add 10% to account for ggml object overhead
        // printf("\n%s: reallocating buffer from %zu to %zu bytes\n", __func__, model.buf.size, buf_size_new);

        // reallocate
        model.eval_buf.resize(buf_size_new);
        if (model.eval_buf.addr == nullptr) {
            fprintf(stderr, "%s: failed to allocate %zu bytes\n", __func__, model.eval_buf.size);
            return false;
        }
    }

    struct ggml_init_params params = {
        .mem_size   = model.eval_buf.size,
        .mem_buffer = model.eval_buf.addr,
        .no_alloc = false
    };

    struct ggml_context * ctx0 = ggml_init(params);
    struct ggml_cgraph gf = {};

    struct ggml_tensor * embd = ggml_new_tensor_1d(ctx0, GGML_TYPE_I32, N);
    memcpy(embd->data, embd_inp.data(), N*ggml_element_size(embd));

    // wte
    struct ggml_tensor * inpL = ggml_get_rows(ctx0, model.wte, embd);

    for (int il = 0; il < n_layer; ++il) {
        ggml_set_scratch(ctx0, {0, model.scr0_buf.size, model.scr0_buf.addr, });

        struct ggml_tensor * inpSA = inpL;
        struct ggml_tensor * cur = inpSA;
        // self-attention
        {

            // norm1
            cur = ggml_norm(ctx0, cur, model.hparams.norm_eps);
            cur = ggml_mul(ctx0,
                    ggml_repeat(ctx0, model.layers[il].norm_1_w, cur),
                    cur);
            // compute QKV
            cur = ggml_mul_mat(ctx0,
                    model.layers[il].attn_Wqkv_w,
                    cur);

            // TODO: clip_qkv
            struct ggml_tensor * Qcur = ggml_cont(ctx0, ggml_view_2d(ctx0, cur, n_embd, N, cur->nb[1], 0*ggml_element_size(cur)*n_embd));
            struct ggml_tensor * Kcur = ggml_cont(ctx0, ggml_view_2d(ctx0, cur, n_embd, N, cur->nb[1], 1*ggml_element_size(cur)*n_embd));
            struct ggml_tensor * Vcur = ggml_cont(ctx0, ggml_view_2d(ctx0, cur, n_embd, N, cur->nb[1], 2*ggml_element_size(cur)*n_embd));

            // TODO: qk_ln? (seems to be False in MPT-7B configs)
            {
                Vcur = ggml_transpose(ctx0, Vcur);

                struct ggml_tensor * k = ggml_view_1d(ctx0, model.kv_self.k, N*n_embd, (ggml_element_size(model.kv_self.k)*n_embd)*(il*n_ctx + n_past));
                struct ggml_tensor * v = ggml_view_2d(ctx0, model.kv_self.v, N, n_embd,
                                        (   n_ctx)*ggml_element_size(model.kv_self.v),
                                        (il*n_ctx)*ggml_element_size(model.kv_self.v)*n_embd + n_past*ggml_element_size(model.kv_self.v));

                ggml_build_forward_expand(&gf, ggml_cpy(ctx0, Kcur, k));
                ggml_build_forward_expand(&gf, ggml_cpy(ctx0, Vcur, v));
            }
            // Q = Qcur.contiguous().view(n_embd/n_head, n_head, N).permute(0, 2, 1, 3)
            struct ggml_tensor * Q =
                ggml_permute(ctx0,
                        ggml_reshape_3d(ctx0, Qcur, n_embd/n_head, n_head, N),
                        0, 2, 1, 3);

            struct ggml_tensor * K =
                ggml_permute(ctx0,
                        ggml_reshape_3d(ctx0,
                            ggml_view_1d(ctx0, model.kv_self.k, (n_past + N)*n_embd, il*n_ctx*ggml_element_size(model.kv_self.k)*n_embd),
                            n_embd/n_head, n_head, n_past + N),
                        0, 2, 1, 3);

            // K * Q
            struct ggml_tensor * KQ = ggml_mul_mat(ctx0, K, Q);

            // KQ_scaled = KQ / sqrt(n_embd/n_head)
            struct ggml_tensor * KQ_scaled =
                ggml_scale(ctx0,
                        KQ,
                        ggml_new_f32(ctx0, 1.0f/sqrt(float(n_embd)/n_head))
                        );


            // Alibi
            struct ggml_tensor * KQ_scaled_biased = ggml_alibi(
                ctx0, ggml_cont(ctx0, KQ_scaled), n_past, n_head, model.hparams.alibi_bias_max
            );

            // KQ_masked = mask_past(KQ_scaled)
            struct ggml_tensor * KQ_masked = ggml_diag_mask_inf(ctx0, KQ_scaled_biased, n_past);

            // KQ = soft_max(KQ_masked)
            struct ggml_tensor * KQ_soft_max = ggml_soft_max(ctx0, KQ_masked);

            // V_trans = Vmem.view(n_embd/n_head, n_head, n_past + N).permute(1, 2, 0, 3).contiguous()
            struct ggml_tensor * V =
                ggml_view_3d(ctx0, model.kv_self.v,
                        n_past + N, n_embd/n_head, n_head,
                        n_ctx*ggml_element_size(model.kv_self.v),
                        n_ctx*ggml_element_size(model.kv_self.v)*n_embd/n_head,
                        il*n_ctx*ggml_element_size(model.kv_self.v)*n_embd);

            // KQV = transpose(V) * KQ_soft_max
            struct ggml_tensor * KQV = ggml_mul_mat(ctx0, V, KQ_soft_max);

            // KQV_merged = KQV.permute(0, 2, 1, 3)
            struct ggml_tensor * KQV_merged = ggml_permute(ctx0, KQV, 0, 2, 1, 3);

            // cur = KQV_merged.contiguous().view(n_embd, N)
            cur = ggml_cpy(ctx0,
                    KQV_merged,
                    ggml_new_tensor_2d(ctx0, GGML_TYPE_F32, n_embd, N));

            // projection (no bias)
            cur = ggml_mul_mat(ctx0,
                    model.layers[il].attn_out_proj_w,
                    cur);
        }

        ggml_set_scratch(ctx0, {0, model.scr1_buf.size, model.scr1_buf.addr, });
        // residual
        struct ggml_tensor * resSA = ggml_add(ctx0, cur, inpSA);
        // feed-forward network
        {
            cur = resSA;
            // norm2
            cur = ggml_norm(ctx0, cur, model.hparams.norm_eps);
            cur = ggml_mul(ctx0,
                    ggml_repeat(ctx0, model.layers[il].norm_2_w, cur),
                    cur);
            // ffn
            cur = ggml_mul_mat(ctx0,
                    model.layers[il].ffn_up_proj_w,
                    cur);
            cur = ggml_gelu(ctx0, cur);
            cur = ggml_mul_mat(ctx0,
                    model.layers[il].ffn_down_proj_w,
                    cur);

        }

        // self-attention + FF
        inpL = ggml_add(ctx0, cur, resSA);
    }
    ggml_set_scratch(ctx0, {0, model.scr0_buf.size, model.scr0_buf.addr, });

    struct ggml_tensor * out = inpL;
    // -> logits
    {
        out = ggml_norm(ctx0, out, model.hparams.norm_eps);
        out = ggml_mul(ctx0,
                    ggml_repeat(ctx0, model.norm_f_w, out),
                    out);
        ggml_set_scratch(ctx0, { 0, 0, nullptr, });
        out = ggml_mul_mat(ctx0, model.wte, out);
    }

    ggml_build_forward_expand(&gf, out);


    // run the computation
    {
        std::unique_ptr<uint8_t []> data;
        auto plan = ggml_graph_plan(&gf, n_threads);
        if (plan.work_size > 0) {
            data.reset(new uint8_t[plan.work_size]);
            plan.work_data = data.get();
        }
        ggml_graph_compute(&gf, &plan);
    }


    // return result for just the last token
    embd_w.resize(n_vocab);
    memcpy(embd_w.data(), (float *) ggml_get_data(out) + (n_vocab*(N-1)), sizeof(float)*n_vocab);

    if (mem_per_token == 0) {
        mem_per_token = ggml_used_mem(ctx0)/N;
    }
    //printf("used_mem = %zu\n", ggml_used_mem(ctx0));

    ggml_free(ctx0);

    return true;
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
    assert(written == mpt_get_state_size(model));
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
    assert(nread == mpt_get_state_size(*model));
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
    bool has_end_of_text = false;
};

MPT::MPT()
    : d_ptr(new MPTPrivate) {
    d_ptr->model = new mpt_model;
    d_ptr->model->ctx = nullptr;
    d_ptr->modelLoaded = false;
}

size_t MPT::requiredMem(const std::string &modelPath) {
    mpt_model dummy_model;
    mpt_vocab dummy_vocab;
    size_t mem_req;
    mpt_model_load(modelPath, dummy_model, dummy_vocab, &mem_req);
    return mem_req;
}

bool MPT::loadModel(const std::string &modelPath) {
    std::mt19937 rng(time(NULL));
    d_ptr->rng = rng;

    // load the model
    if (!mpt_model_load(modelPath, *d_ptr->model, d_ptr->vocab, nullptr)) {
        std::cerr << "MPT ERROR: failed to load model from " <<  modelPath;
        return false;
    }

    d_ptr->n_threads = std::min(4, (int32_t) std::thread::hardware_concurrency());
    d_ptr->modelLoaded = true;
    const auto & vocab = d_ptr->vocab;
    d_ptr->has_end_of_text = vocab.raw.token_to_id.find(vocab.end_of_text()) != vocab.raw.token_to_id.end();
    fflush(stdout);
    return true;
}

void MPT::setThreadCount(int32_t n_threads) {
    d_ptr->n_threads = n_threads;
}

int32_t MPT::threadCount() const
{
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

std::vector<LLModel::Token> MPT::tokenize(PromptContext &, const std::string &str) const
{
    if (d_ptr->vocab.is_replit) {
        return replit_tokenizer_tokenize(d_ptr->vocab, str);
    }
    return ::gpt_tokenize(d_ptr->vocab.raw, str);
}

std::string MPT::tokenToString(Token id) const
{
    if (d_ptr->vocab.is_replit) {
        return replit_tokenizer_detokenize(d_ptr->vocab, {id});
    }
    return d_ptr->vocab.raw.id_to_token[id];
}

LLModel::Token MPT::sampleToken(PromptContext &promptCtx) const
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

bool MPT::evalTokens(PromptContext &ctx, const std::vector<int32_t> &tokens) const
{
    // determine the required inference memory per token:
    static bool initialized = false;
    if (!initialized) {
        mpt_eval(*d_ptr->model, d_ptr->n_threads, 0, { 0, 1, 2, 3 }, ctx.logits,
            d_ptr->mem_per_token);
        initialized = true;
    }

    return mpt_eval(*d_ptr->model, d_ptr->n_threads, ctx.n_past, tokens, ctx.logits, d_ptr->mem_per_token);
}

int32_t MPT::contextLength() const
{
    return d_ptr->model->hparams.n_ctx;
}

const std::vector<LLModel::Token> &MPT::endTokens() const
{
    static std::vector<LLModel::Token> fres;
    if (fres.empty()) {
        fres = {0, d_ptr->vocab.raw.token_to_id[d_ptr->vocab.end_of_text()]};
    }
    return fres;
}

std::string get_arch_name(gguf_context *ctx_gguf) {
    std::string arch_name;
    const int kid = gguf_find_key(ctx_gguf, "general.architecture");
    enum gguf_type ktype = gguf_get_kv_type(ctx_gguf, kid);
    if (ktype != GGUF_TYPE_STRING) {
        throw std::runtime_error("ERROR: Can't get general architecture from gguf file.");
    }
    return gguf_get_val_str(ctx_gguf, kid);
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

DLL_EXPORT bool magic_match(const char * fname) {
    struct ggml_context * ctx_meta = NULL;
    struct gguf_init_params params = {
        /*.no_alloc = */ true,
        /*.ctx      = */ &ctx_meta,
    };
    gguf_context *ctx_gguf = gguf_init_from_file(fname, params);
    if (!ctx_gguf)
        return false;

    bool isValid = gguf_get_version(ctx_gguf) <= 2;
    isValid = isValid && get_arch_name(ctx_gguf) == "mpt";

    gguf_free(ctx_gguf);
    return isValid;
}

DLL_EXPORT LLModel *construct() {
    return new MPT;
}
}
