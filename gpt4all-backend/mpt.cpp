#include "mpt.h"
#include "llama.cpp/ggml.h"

#include "utils.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
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

static const size_t MB = 1024*1024;

// default hparams (MPT 7B)
struct mpt_hparams {
    int32_t n_vocab      = 50432;
    int32_t n_ctx        = 2048;
    int32_t n_embd       = 4096;
    int32_t n_head       = 32;
    int32_t n_layer      = 32;
    float alibi_bias_max = 8;
    float clip_qkv       = 0;
    int32_t expand       = 4;
    int32_t f16          = 1;
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

struct mpt_buffer {
    uint8_t * addr = NULL;
    size_t size = 0;

    void resize(size_t size) {
        delete[] addr;
        addr = new uint8_t[size];
        this->size = size;
    }

    ~mpt_buffer() {
        fflush(stdout);
        delete[] addr;
    }
};

struct mpt_kv_cache {
    struct ggml_tensor * k;
    struct ggml_tensor * v;

    struct ggml_context * ctx = NULL;

    mpt_buffer buf;

    int n; // number of tokens currently in the cache

    ~mpt_kv_cache() {
        if (ctx) {
            ggml_free(ctx);
        }
    }
};

struct mpt_model {
    mpt_hparams hparams;

    // normalization
    struct ggml_tensor * norm_f_w;

    struct ggml_tensor * wte; // position embedding

    // mpt does weight tying

    std::vector<mpt_layer> layers;

    struct mpt_kv_cache kv_self;
    struct ggml_context * ctx;
    std::map<std::string, struct ggml_tensor *> tensors;


    mpt_buffer buf;

    ~mpt_model() {
        if (ctx) {
            ggml_free(ctx);
        }
    }
};

static bool kv_cache_init(
        const struct mpt_hparams & hparams,
             struct mpt_kv_cache & cache,
                         ggml_type   wtype,
                               int   n_ctx) {
    const int n_embd  = hparams.n_embd;
    const int n_layer = hparams.n_layer;

    const int64_t n_mem      = (int64_t)n_layer*n_ctx;
    const int64_t n_elements = n_embd*n_mem;

    cache.buf.resize(2u*n_elements*ggml_type_size(wtype) + 2u*MB);

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
bool mpt_model_load(const std::string &fname, std::istream &fin, mpt_model & model, gpt_vocab & vocab) {
    printf("%s: loading model from '%s' - please wait ...\n", __func__, fname.c_str());

    // verify magic
    {
        uint32_t magic;
        fin.read((char *) &magic, sizeof(magic));
        if (magic != 0x67676d6d) {
            fprintf(stderr, "%s: invalid model file '%s' (bad magic)\n", __func__, fname.c_str());
            return false;
        }
    }

    // load hparams
    {
        auto & hparams = model.hparams;

        fin.read((char *) &hparams.n_vocab, sizeof(hparams.n_vocab));
        fin.read((char *) &hparams.n_ctx,   sizeof(hparams.n_ctx));
        fin.read((char *) &hparams.n_layer, sizeof(hparams.n_layer));
        fin.read((char *) &hparams.n_head,  sizeof(hparams.n_head));
        fin.read((char *) &hparams.n_embd,  sizeof(hparams.n_embd));
        fin.read((char *) &hparams.alibi_bias_max,  sizeof(hparams.alibi_bias_max));
        fin.read((char *) &hparams.clip_qkv,  sizeof(hparams.clip_qkv));
        fin.read((char *) &hparams.f16,   sizeof(hparams.f16));

        printf("%s: n_vocab        = %d\n", __func__, hparams.n_vocab);
        printf("%s: n_ctx          = %d\n", __func__, hparams.n_ctx);
        printf("%s: n_embd         = %d\n", __func__, hparams.n_embd);
        printf("%s: n_head         = %d\n", __func__, hparams.n_head);
        printf("%s: n_layer        = %d\n", __func__, hparams.n_layer);
        printf("%s: alibi_bias_max = %f\n", __func__, hparams.alibi_bias_max);
        printf("%s: clip_qkv       = %f\n", __func__, hparams.clip_qkv);
        printf("%s: ftype          = %d\n", __func__, hparams.f16);
    }

    // load vocab
    {
        int32_t n_vocab = model.hparams.n_vocab;
        fin.read((char *) &n_vocab, sizeof(n_vocab));

        if (n_vocab != model.hparams.n_vocab) {
            fprintf(stderr, "%s: invalid model file '%s' (bad vocab size %d != %d)\n",
                    __func__, fname.c_str(), n_vocab, model.hparams.n_vocab);
            return false;
        }

        std::string word;
        for (int i = 0; i < n_vocab; i++) {
            uint32_t len;
            fin.read((char *) &len, sizeof(len));
            bool special = false;
            if (len & (1<<31)) {
                len = len &~ (1<<31);
                special = true;
            }

            if (len > 0) {
                word.resize(len);
                fin.read((char *) word.data(), len);
                vocab.token_to_id[word] = i;
                vocab.id_to_token[i] = word;
            }

            if(special) {
                vocab.add_special_token(word);
            }
        }
    }

    // for the big tensors, we have the option to store the data in 16-bit floats or quantized
    // in order to save memory and also to speed up the computation
    ggml_type wtype = GGML_TYPE_COUNT;
    switch (model.hparams.f16) {
        case 0: wtype = GGML_TYPE_F32;  break;
        case 1: wtype = GGML_TYPE_F16;  break;
        case 2: wtype = GGML_TYPE_Q4_0; break;
        case 3: wtype = GGML_TYPE_Q4_1; break;
        case 5: wtype = GGML_TYPE_Q4_2; break;
        default:
                {
                    fprintf(stderr, "%s: invalid model file '%s' (bad f16 value %d)\n",
                            __func__, fname.c_str(), model.hparams.f16);
                    return false;
                }
    }

    auto & ctx = model.ctx;

    size_t ctx_size = 0;

    {
        const auto & hparams = model.hparams;

        const int n_embd  = hparams.n_embd;
        const int n_layer = hparams.n_layer;
        const int n_ctx   = hparams.n_ctx;
        const int n_vocab = hparams.n_vocab;
        const int expand  = hparams.expand;


        ctx_size += n_embd*ggml_type_sizef(GGML_TYPE_F32); // ln_f_w

        ctx_size += n_embd*n_vocab*ggml_type_sizef(GGML_TYPE_F32); // wte

        ctx_size += n_layer*(n_embd*ggml_type_sizef(GGML_TYPE_F32)); // norm_1_w
        ctx_size += n_layer*(n_embd*ggml_type_sizef(GGML_TYPE_F32)); // norm_2_w

        ctx_size += n_layer*(3*n_embd*n_embd*ggml_type_sizef(wtype)); // attn_Wqkv_w
        ctx_size += n_layer*(n_embd*n_embd*ggml_type_sizef(wtype)); // attn_out_proj_w

        ctx_size += n_layer*(expand*n_embd*n_embd*ggml_type_sizef(wtype));  // ffn_up_proj_w
        ctx_size += n_layer*(expand*n_embd*n_embd*ggml_type_sizef(wtype)); // ffn_down_proj_w

        ctx_size += n_ctx*n_layer*n_embd*ggml_type_sizef(GGML_TYPE_F16); // memory_k
        ctx_size += n_ctx*n_layer*n_embd*ggml_type_sizef(GGML_TYPE_F16); // memory_v

        // TODO probably less now?
        ctx_size += (5 + 10*n_layer)*256; // object overhead

        printf("%s: ggml ctx size = %6.2f MB\n", __func__, ctx_size/(1024.0*1024.0));
    }

    // create the ggml context
    {
        struct ggml_init_params params = {
            .mem_size   = ctx_size,
            .mem_buffer = NULL,
            .no_alloc   = false,
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

        const int n_embd  = hparams.n_embd;
        const int n_layer = hparams.n_layer;
        const int n_ctx   = hparams.n_ctx;
        const int n_vocab = hparams.n_vocab;
        const int expand  = hparams.expand;

        model.layers.resize(n_layer);

        model.wte    = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, n_embd, n_vocab);
        model.norm_f_w = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embd);

        // map by name
        model.tensors["transformer.wte.weight"] = model.wte;
        model.tensors["transformer.norm_f.weight"] = model.norm_f_w;

        for (int i = 0; i < n_layer; ++i) {
            auto & layer = model.layers[i];

            layer.norm_1_w        = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embd);
            layer.norm_2_w        = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embd);

            layer.attn_Wqkv_w     = ggml_new_tensor_2d(ctx, wtype,        n_embd, n_embd * 3);
            layer.attn_out_proj_w = ggml_new_tensor_2d(ctx, wtype,        n_embd, n_embd);
            layer.ffn_up_proj_w   = ggml_new_tensor_2d(ctx, wtype,        n_embd, expand*n_embd);
            layer.ffn_down_proj_w = ggml_new_tensor_2d(ctx, wtype, expand*n_embd, n_embd);

            // map by name
            model.tensors["transformer.blocks." + std::to_string(i) + ".norm_1.weight"]        = layer.norm_1_w;
            model.tensors["transformer.blocks." + std::to_string(i) + ".norm_2.weight"]        = layer.norm_2_w;
            model.tensors["transformer.blocks." + std::to_string(i) + ".attn.Wqkv.weight"]     = layer.attn_Wqkv_w;
            model.tensors["transformer.blocks." + std::to_string(i) + ".attn.out_proj.weight"] = layer.attn_out_proj_w;

            model.tensors["transformer.blocks." + std::to_string(i) + ".ffn.up_proj.weight"]   = layer.ffn_up_proj_w;
            model.tensors["transformer.blocks." + std::to_string(i) + ".ffn.down_proj.weight"] = layer.ffn_down_proj_w;
        }
    }

    // key + value memory
    {
        const auto & hparams = model.hparams;

        const int n_embd  = hparams.n_embd;
        const int n_layer = hparams.n_layer;
        const int n_ctx   = hparams.n_ctx;

        const int n_mem      = n_layer*n_ctx;
        const int n_elements = n_embd*n_mem;

        if (!kv_cache_init(hparams, model.kv_self, GGML_TYPE_F16, model.hparams.n_ctx)) {
            fprintf(stderr, "%s: kv_cache_init() failed for self-attention cache\n", __func__);
            ggml_free(ctx);
            return false;
        }

        const size_t memory_size = ggml_nbytes(model.kv_self.k) + ggml_nbytes(model.kv_self.v);
        printf("%s: kv self size  = %7.2f MB\n", __func__, memory_size / 1024.0 / 1024.0);
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
            fin.read(reinterpret_cast<char *>(&ttype),  sizeof(ttype));

            if (fin.eof()) {
                break;
            }

            int32_t nelements = 1;
            int32_t ne[2] = { 1, 1 };
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
                fprintf(stderr, "%s: tensor '%s' has wrong shape in model file: got [%d, %d], expected [%d, %d]\n",
                        __func__, name.data(), (int) tensor->ne[0], (int) tensor->ne[1], ne[0], ne[1]);
                return false;
            }

            // for debugging
            if (0) {
                printf("%24s - [%5d, %5d], type = %6s, %6.2f MB, %9zu bytes\n", name.data(), ne[0], ne[1], ggml_type_name(ggml_type(ttype)), ggml_nbytes(tensor)/1024.0/1024.0, ggml_nbytes(tensor));
            }

            const size_t bpe = ggml_type_size(ggml_type(ttype));

            if ((nelements*bpe)/ggml_blck_size(tensor->type) != ggml_nbytes(tensor)) {
                fprintf(stderr, "%s: tensor '%s' has wrong size in model file: got %zu, expected %zu\n",
                        __func__, name.data(), ggml_nbytes(tensor), nelements*bpe);
                return false;
            }

            fin.read(reinterpret_cast<char *>(tensor->data), ggml_nbytes(tensor));

            //printf("%42s - [%5d, %5d], type = %6s, %6.2f MB\n", name.data(), ne[0], ne[1], ttype == 0 ? "float" : "f16", ggml_nbytes(tensor)/1024.0/1024.0);
            total_size += ggml_nbytes(tensor);
            if (++n_tensors % 8 == 0) {
                printf(".");
                fflush(stdout);
            }
        }

        printf(" done\n");

        printf("%s: model size = %8.2f MB / num tensors = %d\n", __func__, total_size/1024.0/1024.0, n_tensors);
    }

    return true;
}

// load the model's weights from a file path
bool mpt_model_load(const std::string & fname, mpt_model & model, gpt_vocab & vocab) {

    auto fin = std::ifstream(fname, std::ios::binary);
    if (!fin) {
        fprintf(stderr, "%s: failed to open '%s'\n", __func__, fname.c_str());
        return false;
    }

    bool loaded = mpt_model_load(fname, fin, model, vocab);
    fin.close();
    return loaded;
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
    const int expand  = hparams.expand;

    const int d_key = n_embd/n_head;

    const size_t init_buf_size = 1024u*MB;
    if (!model.buf.addr || model.buf.size < init_buf_size)
        model.buf.resize(init_buf_size);

    if (mem_per_token > 0 && mem_per_token*N > model.buf.size) {
        const size_t buf_size_new = 1.1*(mem_per_token*N); // add 10% to account for ggml object overhead
        // printf("\n%s: reallocating buffer from %zu to %zu bytes\n", __func__, model.buf.size, buf_size_new);

        // reallocate
        model.buf.resize(buf_size_new);
        if (model.buf.addr == nullptr) {
            fprintf(stderr, "%s: failed to allocate %zu bytes\n", __func__, model.buf.size);
            return false;
        }
    }

    struct ggml_init_params params = {
        .mem_size   = model.buf.size,
        .mem_buffer = model.buf.addr,
    };

    struct ggml_context * ctx0 = ggml_init(params);
    struct ggml_cgraph gf = { .n_threads = n_threads };

    struct ggml_tensor * embd = ggml_new_tensor_1d(ctx0, GGML_TYPE_I32, N);
    memcpy(embd->data, embd_inp.data(), N*ggml_element_size(embd));

    // wte
    struct ggml_tensor * inpL = ggml_get_rows(ctx0, model.wte, embd);

    for (int il = 0; il < n_layer; ++il) {

        struct ggml_tensor * inpSA = inpL;
        struct ggml_tensor * cur = inpSA;
        // self-attention
        {

            // norm1
            cur = ggml_norm(ctx0, cur);
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
            struct ggml_tensor * KQ_scaled_biased = ggml_alibi(ctx0, ggml_cont(ctx0, KQ_scaled), n_past, n_head);

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


        // residual
        struct ggml_tensor * resSA = ggml_add(ctx0, cur, inpSA);
        // feed-forward network
        {
            cur = resSA;
            // norm2
            cur = ggml_norm(ctx0, cur);
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

    struct ggml_tensor * out = inpL;
    // -> logits
    {
        out = ggml_norm(ctx0, out);
        out = ggml_mul(ctx0,
                    ggml_repeat(ctx0, model.norm_f_w, out),
                    out);
        out = ggml_mul_mat(ctx0, model.wte, out);
    }


    // run the computation
    ggml_build_forward_expand(&gf, out);
    ggml_graph_compute       (ctx0, &gf);


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
    const size_t expected = mpt_get_state_size(model);
    assert(written == expected);
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
    const size_t expected = mpt_get_state_size(*model);
    assert(nread == expected);
    fflush(stdout);
    return nread;
}

struct MPTPrivate {
    const std::string modelPath;
    bool modelLoaded;
    gpt_vocab vocab;
    mpt_model *model = nullptr;
    int64_t n_threads = 0;
    size_t mem_per_token = 0;
    std::mt19937 rng;
    bool has_im_end = false;
};

MPT::MPT()
    : d_ptr(new MPTPrivate) {

    d_ptr->model = new mpt_model;
    d_ptr->modelLoaded = false;
}

bool MPT::loadModel(const std::string &modelPath) {
    std::mt19937 rng(time(NULL));
    d_ptr->rng = rng;

    auto fin = std::ifstream(modelPath, std::ios::binary);

    // load the model
    if (!mpt_model_load(modelPath, fin, *d_ptr->model, d_ptr->vocab)) {
        std::cerr << "GPT-J ERROR: failed to load model from " <<  modelPath;
        return false;
    }

    d_ptr->n_threads = std::min(4, (int32_t) std::thread::hardware_concurrency());
    d_ptr->modelLoaded = true;
    d_ptr->has_im_end = d_ptr->vocab.token_to_id.find("<|im_end|>") != d_ptr->vocab.token_to_id.end();
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

void MPT::prompt(const std::string &prompt,
        std::function<bool(int32_t)> promptCallback,
        std::function<bool(int32_t, const std::string&)> responseCallback,
        std::function<bool(bool)> recalculateCallback,
        PromptContext &promptCtx) {

    if (!isModelLoaded()) {
        std::cerr << "GPT-J ERROR: prompt won't work with an unloaded model!\n";
        return;
    }

    const int64_t t_main_start_us = ggml_time_us();

    int64_t t_sample_us  = 0;
    int64_t t_predict_us = 0;
    int64_t t_prompt_us = 0;

    // tokenize the prompt
    std::vector<int> embd_inp = gpt_tokenize(d_ptr->vocab, prompt);

    // save the context size
    promptCtx.n_ctx = d_ptr->model->hparams.n_ctx;

    if ((int) embd_inp.size() > promptCtx.n_ctx - 4) {
        responseCallback(-1, "ERROR: The prompt size exceeds the context window size and cannot be processed.");
        std::cerr << "GPT-J ERROR: The prompt is" << embd_inp.size() <<
            "tokens and the context window is" << promptCtx.n_ctx << "!\n";
        return;
    }

    promptCtx.n_predict = std::min(promptCtx.n_predict, promptCtx.n_ctx - (int) embd_inp.size());
    promptCtx.n_past = std::min(promptCtx.n_past, promptCtx.n_ctx);

    // determine the required inference memory per token:
    static bool initialized = false;
    static std::vector<int> p_instruct;
    static std::vector<int> r_instruct;
    if (!initialized) {
         mpt_eval(*d_ptr->model, d_ptr->n_threads, 0, { 0, 1, 2, 3 }, promptCtx.logits,
            d_ptr->mem_per_token);
        initialized = true;
    }

    // process the prompt in batches
    size_t i = 0;
    const int64_t t_start_prompt_us = ggml_time_us();
    while (i < embd_inp.size()) {
        size_t batch_end = std::min(i + promptCtx.n_batch, embd_inp.size());
        std::vector<int> batch(embd_inp.begin() + i, embd_inp.begin() + batch_end);

        // Check if the context has run out...
        if (promptCtx.n_past + batch.size() > promptCtx.n_ctx) {
            const int32_t erasePoint = promptCtx.n_ctx * promptCtx.contextErase;
            // Erase the first percentage of context from the tokens...
            std::cerr << "MPT: reached the end of the context window so resizing\n";
            promptCtx.tokens.erase(promptCtx.tokens.begin(), promptCtx.tokens.begin() + erasePoint);
            promptCtx.n_past = promptCtx.tokens.size();
            recalculateContext(promptCtx, recalculateCallback);
            assert(promptCtx.n_past + batch.size() <= promptCtx.n_ctx);
        }

        if (!mpt_eval(*d_ptr->model, d_ptr->n_threads, promptCtx.n_past, batch, promptCtx.logits,
            d_ptr->mem_per_token)) {
            std::cerr << "GPT-J ERROR: Failed to process prompt\n";
            return;
        }

        size_t tokens = batch_end - i;
        for (size_t t = 0; t < tokens; ++t) {
            if (promptCtx.tokens.size() == promptCtx.n_ctx)
                promptCtx.tokens.erase(promptCtx.tokens.begin());
            promptCtx.tokens.push_back(batch.at(t));
            if (!promptCallback(batch.at(t)))
                return;
        }
        promptCtx.n_past += batch.size();
        i = batch_end;
    }
    t_prompt_us += ggml_time_us() - t_start_prompt_us;

    int p_instructFound = 0;
    int r_instructFound = 0;

    std::string cachedResponse;
    std::vector<int> cachedTokens;
    std::unordered_set<std::string> reversePrompts
        = { "### Instruction", "### Prompt", "### Response", "### Human", "### Assistant", "### Context" };

    // predict next tokens
    int32_t totalPredictions = 0;
    for (int i = 0; i < promptCtx.n_predict; i++) {

        // sample next token
        const int n_vocab = d_ptr->model->hparams.n_vocab;
        int id = 0;
        {
            const int64_t t_start_sample_us = ggml_time_us();
            const size_t n_prev_toks = std::min((size_t) promptCtx.repeat_last_n, promptCtx.tokens.size());
            id = gpt_sample_top_k_top_p(d_ptr->vocab, n_vocab,
                promptCtx.tokens.data() + promptCtx.tokens.size() - n_prev_toks,
                n_prev_toks,
                promptCtx.logits,
                promptCtx.top_k, promptCtx.top_p, promptCtx.temp,
                promptCtx.repeat_penalty,
                d_ptr->rng);

            t_sample_us += ggml_time_us() - t_start_sample_us;
        }

        // Check if the context has run out...
        if (promptCtx.n_past + 1 > promptCtx.n_ctx) {
            const int32_t erasePoint = promptCtx.n_ctx * promptCtx.contextErase;
            // Erase the first percentage of context from the tokens...
            std::cerr << "MPT: reached the end of the context window so resizing\n";
            promptCtx.tokens.erase(promptCtx.tokens.begin(), promptCtx.tokens.begin() + erasePoint);
            promptCtx.n_past = promptCtx.tokens.size();
            recalculateContext(promptCtx, recalculateCallback);
            assert(promptCtx.n_past + 1 <= promptCtx.n_ctx);
        }

        const int64_t t_start_predict_us = ggml_time_us();
        if (!mpt_eval(*d_ptr->model, d_ptr->n_threads, promptCtx.n_past, { id }, promptCtx.logits,
            d_ptr->mem_per_token)) {
            std::cerr << "GPT-J ERROR: Failed to predict next token\n";
            return;
        }
        t_predict_us += ggml_time_us() - t_start_predict_us;

        promptCtx.n_past += 1;
        // display text
        ++totalPredictions;

        // mpt-7b-chat has special token for end
        if (d_ptr->has_im_end && id == d_ptr->vocab.token_to_id["<|im_end|>"])
            goto stop_generating;

        if (id == 0 /*end of text*/)
            goto stop_generating;

        const std::string str = d_ptr->vocab.id_to_token[id];

        // Check if the provided str is part of our reverse prompts
        bool foundPartialReversePrompt = false;
        const std::string completed = cachedResponse + str;
        if (reversePrompts.find(completed) != reversePrompts.end()) {
            goto stop_generating;
        }

        // Check if it partially matches our reverse prompts and if so, cache
        for (auto s : reversePrompts) {
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
            if (promptCtx.tokens.size() == promptCtx.n_ctx)
                promptCtx.tokens.erase(promptCtx.tokens.begin());
            promptCtx.tokens.push_back(t);
            if (!responseCallback(t, d_ptr->vocab.id_to_token[t]))
                goto stop_generating;
        }
        cachedTokens.clear();
    }

stop_generating:

#if 0
    // report timing
    {
        const int64_t t_main_end_us = ggml_time_us();

        std::cout << "GPT-J INFO: mem per token = " << mem_per_token << " bytes\n";
        std::cout << "GPT-J INFO:   sample time = " << t_sample_us/1000.0f << " ms\n";
        std::cout << "GPT-J INFO:   prompt time = " << t_prompt_us/1000.0f << " ms\n";
        std::cout << "GPT-J INFO:  predict time = " << t_predict_us/1000.0f << " ms / " << t_predict_us/1000.0f/totalPredictions << " ms per token\n";
        std::cout << "GPT-J INFO:    total time = " << (t_main_end_us - t_main_start_us)/1000.0f << " ms\n";
        fflush(stdout);
    }
#endif

    return;
}

void MPT::recalculateContext(PromptContext &promptCtx, std::function<bool(bool)> recalculate)
{
    size_t i = 0;
    promptCtx.n_past = 0;
    while (i < promptCtx.tokens.size()) {
        size_t batch_end = std::min(i + promptCtx.n_batch, promptCtx.tokens.size());
        std::vector<int> batch(promptCtx.tokens.begin() + i, promptCtx.tokens.begin() + batch_end);

        assert(promptCtx.n_past + batch.size() <= promptCtx.n_ctx);

        if (!mpt_eval(*d_ptr->model, d_ptr->n_threads, promptCtx.n_past, batch, promptCtx.logits,
            d_ptr->mem_per_token)) {
            std::cerr << "MPT ERROR: Failed to process prompt\n";
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
