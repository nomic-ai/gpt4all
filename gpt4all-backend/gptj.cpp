#define GPTJ_H_I_KNOW_WHAT_I_AM_DOING_WHEN_INCLUDING_THIS_FILE
#include "gptj_impl.h"

#include "utils.h"
#include "llmodel_shared.h"

#include <cassert>
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
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
#include <unordered_set>
#include <ggml.h>


namespace {
const char *modelType_ = "GPT-J";
}

// default hparams (GPT-J 6B)
struct gptj_hparams {
    int32_t n_vocab = 50400;
    int32_t n_ctx   = 2048;
    int32_t n_embd  = 4096;
    int32_t n_head  = 16;
    int32_t n_layer = 28;
    int32_t n_rot   = 64;
    float norm_eps  = 1e-5;
};

struct gptj_layer {
    // normalization
    struct ggml_tensor * ln_1_g;
    struct ggml_tensor * ln_1_b;

    // attention
    struct ggml_tensor * c_attn_q_proj_w;
    struct ggml_tensor * c_attn_k_proj_w;
    struct ggml_tensor * c_attn_v_proj_w;

    struct ggml_tensor * c_attn_proj_w;

    // ff
    struct ggml_tensor * c_mlp_fc_w;
    struct ggml_tensor * c_mlp_fc_b;

    struct ggml_tensor * c_mlp_proj_w;
    struct ggml_tensor * c_mlp_proj_b;
};

struct gptj_model {
    gptj_hparams hparams;

    // normalization
    struct ggml_tensor * ln_f_g;
    struct ggml_tensor * ln_f_b;

    struct ggml_tensor * wte; // position embedding

    struct ggml_tensor * lmh_g; // language model head
    struct ggml_tensor * lmh_b; // language model bias

    std::vector<gptj_layer> layers;

    // key + value memory
    struct llm_kv_cache kv_self;

    //
    struct ggml_context * ctx;
    std::map<std::string, struct ggml_tensor *> tensors;

    llm_buffer eval_buf;
    llm_buffer scr0_buf;
    llm_buffer scr1_buf;

    ~gptj_model() {
        if (ctx) {
            ggml_free(ctx);
        }
    }
};

static bool kv_cache_init(
        const struct gptj_hparams & hparams,
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

// load the model's weights from a file path
bool gptj_model_load(const std::string &fname, gptj_model & model, gpt_vocab & vocab, size_t * mem_req = nullptr) {
    printf("%s: loading model from '%s' - please wait ...\n", __func__, fname.c_str());
    if(mem_req != nullptr) {
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

    // load hparams
    {
        auto & hparams = model.hparams;

        bool ok = false;
        int keyidx;

        do {
            keyidx = gguf_find_key(ggufctx, "gptj.context_length");
            if (keyidx == -1) { break; }
            hparams.n_ctx = gguf_get_val_u32(ggufctx, keyidx);

            keyidx = gguf_find_key(ggufctx, "gptj.embedding_length");
            if (keyidx == -1) { break; }
            hparams.n_embd = gguf_get_val_u32(ggufctx, keyidx);

            keyidx = gguf_find_key(ggufctx, "gptj.attention.head_count");
            if (keyidx == -1) { break; }
            hparams.n_head = gguf_get_val_u32(ggufctx, keyidx);

            keyidx = gguf_find_key(ggufctx, "gptj.block_count");
            if (keyidx == -1) { break; }
            hparams.n_layer = gguf_get_val_u32(ggufctx, keyidx);

            keyidx = gguf_find_key(ggufctx, "gptj.rope.dimension_count");
            if (keyidx == -1) { break; }
            hparams.n_rot = gguf_get_val_u32(ggufctx, keyidx);

            keyidx = gguf_find_key(ggufctx, "gptj.attention.layer_norm_epsilon");
            if (keyidx == -1) { break; }
            hparams.norm_eps = gguf_get_val_f32(ggufctx, keyidx);

            ok = true;
        } while (false);

        if (!ok) {
            fprintf(stderr, "%s: required hparam missing!\n", __func__);
            return false;
        }

        printf("%s: n_ctx   = %d\n", __func__, hparams.n_ctx);
        printf("%s: n_embd  = %d\n", __func__, hparams.n_embd);
        printf("%s: n_head  = %d\n", __func__, hparams.n_head);
        printf("%s: n_layer = %d\n", __func__, hparams.n_layer);
        printf("%s: n_rot   = %d\n", __func__, hparams.n_rot);
    }

    // load vocab
    {
        auto & hparams = model.hparams;

        int keyidx = gguf_find_key(ggufctx, "tokenizer.ggml.model");
        if (keyidx == -1) {
            fprintf(stderr, "%s: tokenizer model not found!\n", __func__);
            return false;
        }
        if (strcmp(gguf_get_val_str(ggufctx, keyidx), "gpt2") != 0) {
            fprintf(stderr, "%s: tokenizer model not supported!\n", __func__);
            return false;
        }

        int tokens_keyidx = gguf_find_key(ggufctx, "tokenizer.ggml.tokens");
        if (tokens_keyidx == -1) {
            fprintf(stderr, "%s: gpt2 tokenizer vocab not found!\n", __func__);
            return false;
        }

        hparams.n_vocab = gguf_get_arr_n(ggufctx, tokens_keyidx);
        printf("%s: gpt2 tokenizer vocab = %d\n", __func__, int(hparams.n_vocab));

        for (int i = 0; i < hparams.n_vocab; i++) {
            std::string word = gguf_get_arr_str(ggufctx, tokens_keyidx, i);
            vocab.token_to_id[word] = i;
            vocab.id_to_token[i] = word;
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

        model.wte    = ggml_get_tensor(ctx, "token_embd.weight");

        model.ln_f_g = ggml_get_tensor(ctx, "output_norm.weight");
        model.ln_f_b = ggml_get_tensor(ctx, "output_norm.bias");

        model.lmh_g  = ggml_get_tensor(ctx, "output.weight");
        model.lmh_b  = ggml_get_tensor(ctx, "output.bias");

        auto name = [](int i, std::string n) {
            static std::string key;
            key = "blk." + std::to_string(i) + "." + n;
            return key.c_str();
        };

        for (int i = 0; i < hparams.n_layer; ++i) {
            auto & layer = model.layers[i];

            layer.ln_1_g          = ggml_get_tensor(ctx, name(i, "attn_norm.weight"));
            layer.ln_1_b          = ggml_get_tensor(ctx, name(i, "attn_norm.bias"));

            layer.c_attn_q_proj_w = ggml_get_tensor(ctx, name(i, "attn_q.weight"));
            layer.c_attn_k_proj_w = ggml_get_tensor(ctx, name(i, "attn_k.weight"));
            layer.c_attn_v_proj_w = ggml_get_tensor(ctx, name(i, "attn_v.weight"));

            layer.c_attn_proj_w   = ggml_get_tensor(ctx, name(i, "attn_output.weight"));

            layer.c_mlp_fc_w      = ggml_get_tensor(ctx, name(i, "ffn_up.weight"));
            layer.c_mlp_fc_b      = ggml_get_tensor(ctx, name(i, "ffn_up.bias"));

            layer.c_mlp_proj_w    = ggml_get_tensor(ctx, name(i, "ffn_down.weight"));
            layer.c_mlp_proj_b    = ggml_get_tensor(ctx, name(i, "ffn_down.bias"));
        }
    }

    // key + value memory
    {
        const auto & hparams = model.hparams;
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

// evaluate the transformer
//
//   - model:     the model
//   - n_threads: number of threads to use
//   - n_past:    the context size so far
//   - embd_inp:  the embeddings of the tokens in the context
//   - embd_w:    the predicted logits for the next token
//
// The GPT-J model requires about 16MB of memory per input token.
//
bool gptj_eval(
        gptj_model & model,
        const int n_threads,
        const int n_past,
        const std::vector<gpt_vocab::id> & embd_inp,
              std::vector<float>         & embd_w,
              size_t                     & mem_per_token) {
    const int N = embd_inp.size();

    const auto & hparams = model.hparams;

    const int n_embd  = hparams.n_embd;
    const int n_layer = hparams.n_layer;
    const int n_ctx   = hparams.n_ctx;
    const int n_head  = hparams.n_head;
    const int n_vocab = hparams.n_vocab;
    const int n_rot   = hparams.n_rot;

    const size_t init_buf_size = 1024_MiB;
    if (!model.eval_buf.addr || model.eval_buf.size < init_buf_size)
        model.eval_buf.resize(init_buf_size);

    if (mem_per_token > 0 && mem_per_token*N > model.eval_buf.size) {
        const size_t buf_size_new = 1.1*(mem_per_token*N); // add 10% to account for ggml object overhead
        printf("\n%s: reallocating buffer from %zu to %zu bytes\n", __func__, model.eval_buf.size, buf_size_new);

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
    struct ggml_cgraph * gf = ggml_new_graph(ctx0);

    // KQ_pos - contains the positions
    struct ggml_tensor * KQ_pos = ggml_new_tensor_1d(ctx0, GGML_TYPE_I32, N);
    int * data = (int *) KQ_pos->data;
    for (int i = 0; i < N; ++i) {
        data[i] = n_past + i;
    }

    struct ggml_tensor * embd = ggml_new_tensor_1d(ctx0, GGML_TYPE_I32, N);
    memcpy(embd->data, embd_inp.data(), N*ggml_element_size(embd));

    // wte
    struct ggml_tensor * inpL = ggml_get_rows(ctx0, model.wte, embd);

    for (int il = 0; il < n_layer; ++il) {
        struct ggml_tensor * cur;
        ggml_set_scratch(ctx0, {0, model.scr0_buf.size, model.scr0_buf.addr, });
        // norm
        {
            cur = ggml_norm(ctx0, inpL, model.hparams.norm_eps);

            // cur = ln_1_g*cur + ln_1_b
            cur = ggml_add(ctx0,
                    ggml_mul(ctx0,
                        ggml_repeat(ctx0, model.layers[il].ln_1_g, cur),
                        cur),
                    ggml_repeat(ctx0, model.layers[il].ln_1_b, cur));
        }

        struct ggml_tensor * inpSA = cur;

        // self-attention
        {
            struct ggml_tensor * Qcur = ggml_rope(
                ctx0, ggml_reshape_3d(ctx0, ggml_mul_mat(ctx0, model.layers[il].c_attn_q_proj_w, cur), n_embd/n_head, n_head, N),
                KQ_pos, n_rot, 0, 0
            );
            struct ggml_tensor * Kcur = ggml_rope(
                ctx0, ggml_reshape_3d(ctx0, ggml_mul_mat(ctx0, model.layers[il].c_attn_k_proj_w, cur), n_embd/n_head, n_head, N),
                KQ_pos, n_rot, 0, 0
            );

            // store key and value to memory
            {
                struct ggml_tensor * Vcur = ggml_transpose(ctx0, ggml_mul_mat(ctx0, model.layers[il].c_attn_v_proj_w, cur));

                struct ggml_tensor * k = ggml_view_1d(ctx0, model.kv_self.k, N*n_embd, (ggml_element_size(model.kv_self.k)*n_embd)*(il*n_ctx + n_past));
                struct ggml_tensor * v = ggml_view_2d(ctx0, model.kv_self.v, N, n_embd,
                        (   n_ctx)*ggml_element_size(model.kv_self.v),
                        (il*n_ctx)*ggml_element_size(model.kv_self.v)*n_embd + n_past*ggml_element_size(model.kv_self.v));

                ggml_build_forward_expand(gf, ggml_cpy(ctx0, Kcur, k));
                ggml_build_forward_expand(gf, ggml_cpy(ctx0, Vcur, v));
            }

            // Q = Qcur.contiguous().view(n_embd/n_head, n_head, N).permute(0, 2, 1, 3)
            struct ggml_tensor * Q = ggml_permute(ctx0, Qcur, 0, 2, 1, 3);

            // K = Kmem.view(n_embd/n_head, n_head, n_past + N).permute(0, 2, 1, 3)
            struct ggml_tensor * K =
                ggml_permute(ctx0,
                        ggml_reshape_3d(ctx0,
                            ggml_view_1d(ctx0, model.kv_self.k, (n_past + N)*n_embd, il*n_ctx*ggml_element_size(model.kv_self.k)*n_embd),
                            n_embd/n_head, n_head, n_past + N),
                        0, 2, 1, 3);

            // K * Q
            struct ggml_tensor * KQ = ggml_mul_mat(ctx0, K, Q);

            // KQ_scaled = KQ / sqrt(n_embd/n_head)
            struct ggml_tensor * KQ_scaled = ggml_scale(ctx0, KQ, 1.0f/sqrt(float(n_embd)/n_head));

            // KQ_masked = mask_past(KQ_scaled)
            struct ggml_tensor * KQ_masked = ggml_diag_mask_inf(ctx0, KQ_scaled, n_past);

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
                    model.layers[il].c_attn_proj_w,
                    cur);
        }

        struct ggml_tensor * inpFF = cur;

        ggml_set_scratch(ctx0, {0, model.scr1_buf.size, model.scr1_buf.addr, });
        // feed-forward network
        // this is independent of the self-attention result, so it could be done in parallel to the self-attention
        {
            // note here we pass inpSA instead of cur
            cur = ggml_mul_mat(ctx0,
                    model.layers[il].c_mlp_fc_w,
                    inpSA);

            cur = ggml_add(ctx0,
                    ggml_repeat(ctx0, model.layers[il].c_mlp_fc_b, cur),
                    cur);

            // GELU activation
            cur = ggml_gelu(ctx0, cur);

            // projection
            // cur = proj_w*cur + proj_b
            cur = ggml_mul_mat(ctx0,
                    model.layers[il].c_mlp_proj_w,
                    cur);

            cur = ggml_add(ctx0,
                    ggml_repeat(ctx0, model.layers[il].c_mlp_proj_b, cur),
                    cur);
        }

        // self-attention + FF
        cur  = ggml_add(ctx0, cur, inpFF);

        // input for next layer
        inpL = ggml_add(ctx0, cur, inpL);
    }

    ggml_set_scratch(ctx0, {0, model.scr0_buf.size, model.scr0_buf.addr, });

    // norm
    {
        inpL = ggml_norm(ctx0, inpL, model.hparams.norm_eps);

        // inpL = ln_f_g*inpL + ln_f_b
        inpL = ggml_add(ctx0,
                ggml_mul(ctx0,
                    ggml_repeat(ctx0, model.ln_f_g, inpL),
                    inpL),
                ggml_repeat(ctx0, model.ln_f_b, inpL));
    }

    ggml_set_scratch(ctx0, { 0, 0, nullptr, });

    // lm_head
    {
        inpL = ggml_mul_mat(ctx0, model.lmh_g, inpL);

        inpL = ggml_add(ctx0,
                ggml_repeat(ctx0, model.lmh_b, inpL),
                inpL);
    }

    // logits -> probs
    //inpL = ggml_soft_max(ctx0, inpL);

    ggml_build_forward_expand(gf, inpL);

    // run the computation
    {
        std::unique_ptr<uint8_t []> data;
        auto plan = ggml_graph_plan(gf, n_threads);
        if (plan.work_size > 0) {
            data.reset(new uint8_t[plan.work_size]);
            plan.work_data = data.get();
        }
        ggml_graph_compute(gf, &plan);
    }

    //if (n_past%100 == 0) {
    //    ggml_graph_print   (gf);
    //    ggml_graph_dump_dot(gf, NULL, "gpt-2.dot");
    //}

    //embd_w.resize(n_vocab*N);
    //memcpy(embd_w.data(), ggml_get_data(inpL), sizeof(float)*n_vocab*N);

    // return result for just the last token
    embd_w.resize(n_vocab);
    memcpy(embd_w.data(), (float *) ggml_get_data(inpL) + (n_vocab*(N-1)), sizeof(float)*n_vocab);

    if (mem_per_token == 0) {
        mem_per_token = ggml_used_mem(ctx0)/N;
    }
    //printf("used_mem = %zu\n", ggml_used_mem(ctx0));

    ggml_free(ctx0);

    return true;
}

#define GPTJ_MAX_RNG_STATE 64*1024

size_t gptj_get_state_size(const gptj_model &model)
{
    // we don't know size of rng until we actually serialize it. so reserve more than enough memory for its serialized state.
    // for reference, std::mt19937(1337) serializes to 6701 bytes.
    const size_t s_rng_size        = sizeof(size_t);
    const size_t s_rng             = GPTJ_MAX_RNG_STATE;
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

size_t gptj_copy_state_data(const gptj_model &model, const std::mt19937 &rng, uint8_t *dest)
{
    uint8_t * out = dest;
    fflush(stdout);
    // copy rng
    {
        std::stringstream rng_ss;
        rng_ss << rng;

        const size_t rng_size = rng_ss.str().size();
        char rng_buf[GPTJ_MAX_RNG_STATE];

        memset(&rng_buf[0], 0, GPTJ_MAX_RNG_STATE);
        memcpy(&rng_buf[0], rng_ss.str().data(), rng_ss.str().size());

        memcpy(out, &rng_size,   sizeof(rng_size));   out += sizeof(rng_size);
        memcpy(out, &rng_buf[0], GPTJ_MAX_RNG_STATE); out += GPTJ_MAX_RNG_STATE;
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
    assert(written == gptj_get_state_size(model));
    fflush(stdout);
    return written;
}

size_t gptj_set_state_data(gptj_model *model, std::mt19937 *rng, const uint8_t *src)
{
    const uint8_t * in = src;

    // set rng
    {
        size_t rng_size;
        char   rng_buf[GPTJ_MAX_RNG_STATE];

        memcpy(&rng_size,   in, sizeof(rng_size));    in += sizeof(rng_size);
        memcpy(&rng_buf[0], in, GPTJ_MAX_RNG_STATE); in += GPTJ_MAX_RNG_STATE;

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
    assert(nread == gptj_get_state_size(*model));
    fflush(stdout);
    return nread;
}

struct GPTJPrivate {
    const std::string modelPath;
    bool modelLoaded;
    gpt_vocab vocab;
    gptj_model *model = nullptr;
    int64_t n_threads = 0;
    size_t mem_per_token = 0;
    std::mt19937 rng;
};

GPTJ::GPTJ()
    : d_ptr(new GPTJPrivate) {
    d_ptr->model = new gptj_model;
    d_ptr->model->ctx = nullptr;
    d_ptr->modelLoaded = false;
}

size_t GPTJ::requiredMem(const std::string &modelPath, int n_ctx, int ngl) {
    (void)n_ctx;
    (void)ngl;
    gptj_model dummy_model;
    gpt_vocab dummy_vocab;
    size_t mem_req;
    gptj_model_load(modelPath, dummy_model, dummy_vocab, &mem_req);
    return mem_req;
}

bool GPTJ::loadModel(const std::string &modelPath, int n_ctx, int ngl) {
    (void)n_ctx;
    (void)ngl;
    d_ptr->modelLoaded = false;

    std::mt19937 rng(time(NULL));
    d_ptr->rng = rng;

    // load the model
    bool ok = gptj_model_load(modelPath, *d_ptr->model, d_ptr->vocab);
    fflush(stdout);
    if (!ok) {
        std::cerr << "GPT-J ERROR: failed to load model from " <<  modelPath;
        return false;
    }

    d_ptr->n_threads = std::min(4, (int32_t) std::thread::hardware_concurrency());
    d_ptr->modelLoaded = true;
    return true;
}

void GPTJ::setThreadCount(int32_t n_threads) {
    d_ptr->n_threads = n_threads;
}

int32_t GPTJ::threadCount() const
{
    return d_ptr->n_threads;
}

GPTJ::~GPTJ()
{
    delete d_ptr->model;
}

bool GPTJ::isModelLoaded() const
{
    return d_ptr->modelLoaded;
}

size_t GPTJ::stateSize() const
{
    return gptj_get_state_size(*d_ptr->model);
}

size_t GPTJ::saveState(uint8_t *dest) const
{
    return gptj_copy_state_data(*d_ptr->model, d_ptr->rng, dest);
}

size_t GPTJ::restoreState(const uint8_t *src)
{
    return gptj_set_state_data(d_ptr->model, &d_ptr->rng, src);
}

std::vector<LLModel::Token> GPTJ::tokenize(PromptContext &, const std::string &str) const
{
    return ::gpt_tokenize(d_ptr->vocab, str);
}

LLModel::Token GPTJ::sampleToken(PromptContext &promptCtx) const
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

std::string GPTJ::tokenToString(Token id) const
{
    return d_ptr->vocab.id_to_token[id];
}

bool GPTJ::evalTokens(PromptContext &ctx, const std::vector<int32_t> &tokens) const
{
    // determine the required inference memory per token:
    static bool initialized = false;
    if (!initialized) {
        gptj_eval(*d_ptr->model, d_ptr->n_threads, 0, { 0, 1, 2, 3 }, ctx.logits,
            d_ptr->mem_per_token);
        initialized = true;
    }

    return gptj_eval(*d_ptr->model, d_ptr->n_threads, ctx.n_past, tokens, ctx.logits, d_ptr->mem_per_token);
}

int32_t GPTJ::contextLength() const
{
    return d_ptr->model->hparams.n_ctx;
}

const std::vector<LLModel::Token> &GPTJ::endTokens() const
{
    static const std::vector<LLModel::Token> fres = {50256};
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

    bool isValid = gguf_get_version(ctx_gguf) <= 3;
    isValid = isValid && get_arch_name(ctx_gguf) == "gptj";

    gguf_free(ctx_gguf);
    return isValid;
}

DLL_EXPORT LLModel *construct() {
    return new GPTJ;
}
}
