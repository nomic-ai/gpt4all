#include "gptj.h"
#include "llama.cpp/ggml.h"

#include "utils.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
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

// default hparams (GPT-J 6B)
static const size_t MB = 1024*1024;

struct gptj_hparams {
    int32_t n_vocab = 50400;
    int32_t n_ctx   = 2048;
    int32_t n_embd  = 4096;
    int32_t n_head  = 16;
    int32_t n_layer = 28;
    int32_t n_rot   = 64;
    int32_t f16     = 1;
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

struct gptj_buffer {
    uint8_t * addr = NULL;
    size_t size = 0;

    void resize(size_t size) {
        delete[] addr;
        addr = new uint8_t[size];
        this->size = size;
    }

    ~gptj_buffer() {
        fflush(stdout);
        delete[] addr;
    }
};

struct gptj_kv_cache {
    struct ggml_tensor * k;
    struct ggml_tensor * v;

    struct ggml_context * ctx = NULL;

    gptj_buffer buf;

    int n; // number of tokens currently in the cache

    ~gptj_kv_cache() {
        if (ctx) {
            ggml_free(ctx);
        }
    }
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
    struct gptj_kv_cache kv_self;

    //
    struct ggml_context * ctx;
    std::map<std::string, struct ggml_tensor *> tensors;

    gptj_buffer buf;

    ~gptj_model() {
        if (ctx) {
            ggml_free(ctx);
        }
    }
};

static bool kv_cache_init(
        const struct gptj_hparams & hparams,
             struct gptj_kv_cache & cache,
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
bool gptj_model_load(const std::string &fname, std::istream &fin, gptj_model & model, gpt_vocab & vocab) {
    printf("%s: loading model from '%s' - please wait ...\n", __func__, fname.c_str());

    // verify magic
    {
        uint32_t magic;
        fin.read((char *) &magic, sizeof(magic));
        if (magic != 0x67676d6c) {
            fprintf(stderr, "%s: invalid model file '%s' (bad magic)\n", __func__, fname.c_str());
            return false;
        }
    }

    // load hparams
    {
        auto & hparams = model.hparams;

        fin.read((char *) &hparams.n_vocab, sizeof(hparams.n_vocab));
        fin.read((char *) &hparams.n_ctx,   sizeof(hparams.n_ctx));
        fin.read((char *) &hparams.n_embd,  sizeof(hparams.n_embd));
        fin.read((char *) &hparams.n_head,  sizeof(hparams.n_head));
        fin.read((char *) &hparams.n_layer, sizeof(hparams.n_layer));
        fin.read((char *) &hparams.n_rot,   sizeof(hparams.n_rot));
        fin.read((char *) &hparams.f16,     sizeof(hparams.f16));

        printf("%s: n_vocab = %d\n", __func__, hparams.n_vocab);
        printf("%s: n_ctx   = %d\n", __func__, hparams.n_ctx);
        printf("%s: n_embd  = %d\n", __func__, hparams.n_embd);
        printf("%s: n_head  = %d\n", __func__, hparams.n_head);
        printf("%s: n_layer = %d\n", __func__, hparams.n_layer);
        printf("%s: n_rot   = %d\n", __func__, hparams.n_rot);
        printf("%s: f16     = %d\n", __func__, hparams.f16);
    }

    // load vocab
    {
        int32_t n_vocab = 0;
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

            word.resize(len);
            fin.read((char *) word.data(), len);

            vocab.token_to_id[word] = i;
            vocab.id_to_token[i] = word;
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

    const ggml_type wtype2 = GGML_TYPE_F32;

    auto & ctx = model.ctx;

    size_t ctx_size = 0;

    {
        const auto & hparams = model.hparams;

        const int n_embd  = hparams.n_embd;
        const int n_layer = hparams.n_layer;
        const int n_ctx   = hparams.n_ctx;
        const int n_vocab = hparams.n_vocab;

        ctx_size += n_embd*ggml_type_sizef(GGML_TYPE_F32); // ln_f_g
        ctx_size += n_embd*ggml_type_sizef(GGML_TYPE_F32); // ln_f_b

        ctx_size += n_embd*n_vocab*ggml_type_sizef(wtype); // wte

        ctx_size += n_embd*n_vocab*ggml_type_sizef(wtype);         // lmh_g
        ctx_size +=        n_vocab*ggml_type_sizef(GGML_TYPE_F32); // lmh_b

        ctx_size += n_layer*(n_embd*ggml_type_sizef(GGML_TYPE_F32)); // ln_1_g
        ctx_size += n_layer*(n_embd*ggml_type_sizef(GGML_TYPE_F32)); // ln_1_b

        ctx_size += n_layer*(n_embd*n_embd*ggml_type_sizef(wtype)); // c_attn_q_proj_w
        ctx_size += n_layer*(n_embd*n_embd*ggml_type_sizef(wtype)); // c_attn_k_proj_w
        ctx_size += n_layer*(n_embd*n_embd*ggml_type_sizef(wtype)); // c_attn_v_proj_w

        ctx_size += n_layer*(n_embd*n_embd*ggml_type_sizef(wtype)); // c_attn_proj_w

        ctx_size += n_layer*(4*n_embd*n_embd*ggml_type_sizef(wtype));         // c_mlp_fc_w
        ctx_size += n_layer*(       4*n_embd*ggml_type_sizef(GGML_TYPE_F32)); // c_mlp_fc_b

        ctx_size += n_layer*(4*n_embd*n_embd*ggml_type_sizef(wtype));         // c_mlp_proj_w
        ctx_size += n_layer*(         n_embd*ggml_type_sizef(GGML_TYPE_F32)); // c_mlp_proj_b

        ctx_size += n_ctx*n_layer*n_embd*ggml_type_sizef(GGML_TYPE_F32); // memory_k
        ctx_size += n_ctx*n_layer*n_embd*ggml_type_sizef(GGML_TYPE_F32); // memory_v

        ctx_size += (5 + 10*n_layer)*256; // object overhead

        printf("%s: ggml ctx size = %6.2f MB\n", __func__, ctx_size/(1024.0*1024.0));
    }

    // create the ggml context
    {
        struct ggml_init_params params = {
            .mem_size   = ctx_size,
            .mem_buffer = NULL,
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

        model.layers.resize(n_layer);

        model.wte    = ggml_new_tensor_2d(ctx, wtype,         n_embd, n_vocab);

        model.ln_f_g = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embd);
        model.ln_f_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embd);

        model.lmh_g  = ggml_new_tensor_2d(ctx, wtype,         n_embd, n_vocab);
        model.lmh_b  = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_vocab);

        // map by name
        model.tensors["transformer.wte.weight"] = model.wte;

        model.tensors["transformer.ln_f.weight"] = model.ln_f_g;
        model.tensors["transformer.ln_f.bias"]   = model.ln_f_b;

        model.tensors["lm_head.weight"] = model.lmh_g;
        model.tensors["lm_head.bias"]   = model.lmh_b;

        for (int i = 0; i < n_layer; ++i) {
            auto & layer = model.layers[i];

            layer.ln_1_g          = ggml_new_tensor_1d(ctx, GGML_TYPE_F32,   n_embd);
            layer.ln_1_b          = ggml_new_tensor_1d(ctx, GGML_TYPE_F32,   n_embd);

            layer.c_attn_q_proj_w = ggml_new_tensor_2d(ctx, wtype,           n_embd,   n_embd);
            layer.c_attn_k_proj_w = ggml_new_tensor_2d(ctx, wtype,           n_embd,   n_embd);
            layer.c_attn_v_proj_w = ggml_new_tensor_2d(ctx, wtype,           n_embd,   n_embd);

            layer.c_attn_proj_w   = ggml_new_tensor_2d(ctx, wtype,           n_embd,   n_embd);

            layer.c_mlp_fc_w      = ggml_new_tensor_2d(ctx, wtype,           n_embd, 4*n_embd);
            layer.c_mlp_fc_b      = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, 4*n_embd);

            layer.c_mlp_proj_w    = ggml_new_tensor_2d(ctx, wtype,         4*n_embd,   n_embd);
            layer.c_mlp_proj_b    = ggml_new_tensor_1d(ctx, GGML_TYPE_F32,   n_embd);

            // map by name
            model.tensors["transformer.h." + std::to_string(i) + ".ln_1.weight"]          = layer.ln_1_g;
            model.tensors["transformer.h." + std::to_string(i) + ".ln_1.bias"]            = layer.ln_1_b;

            model.tensors["transformer.h." + std::to_string(i) + ".attn.q_proj.weight"]   = layer.c_attn_q_proj_w;
            model.tensors["transformer.h." + std::to_string(i) + ".attn.k_proj.weight"]   = layer.c_attn_k_proj_w;
            model.tensors["transformer.h." + std::to_string(i) + ".attn.v_proj.weight"]   = layer.c_attn_v_proj_w;

            model.tensors["transformer.h." + std::to_string(i) + ".attn.out_proj.weight"] = layer.c_attn_proj_w;

            model.tensors["transformer.h." + std::to_string(i) + ".mlp.fc_in.weight"]     = layer.c_mlp_fc_w;
            model.tensors["transformer.h." + std::to_string(i) + ".mlp.fc_in.bias"]       = layer.c_mlp_fc_b;

            model.tensors["transformer.h." + std::to_string(i) + ".mlp.fc_out.weight"]    = layer.c_mlp_proj_w;
            model.tensors["transformer.h." + std::to_string(i) + ".mlp.fc_out.bias"]      = layer.c_mlp_proj_b;
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
            int32_t ftype;

            fin.read(reinterpret_cast<char *>(&n_dims), sizeof(n_dims));
            fin.read(reinterpret_cast<char *>(&length), sizeof(length));
            fin.read(reinterpret_cast<char *>(&ftype),  sizeof(ftype));

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
                fprintf(stderr, "%s: tensor '%s' has wrong shape in model file: got [%lu, %lu], expected [%d, %d]\n",
                        __func__, name.data(), tensor->ne[0], tensor->ne[1], ne[0], ne[1]);
                return false;
            }

            if (0) {
                static const char * ftype_str[] = { "f32", "f16", "q4_0", "q4_1", };
                printf("%24s - [%5d, %5d], type = %6s, %6.2f MB, %9zu bytes\n", name.data(), ne[0], ne[1], ftype_str[ftype], ggml_nbytes(tensor)/1024.0/1024.0, ggml_nbytes(tensor));
            }

            size_t bpe = 0;

            switch (ftype) {
                case 0: bpe = ggml_type_size(GGML_TYPE_F32);  break;
                case 1: bpe = ggml_type_size(GGML_TYPE_F16);  break;
                case 2: bpe = ggml_type_size(GGML_TYPE_Q4_0); assert(ne[0] % 64 == 0); break;
                case 3: bpe = ggml_type_size(GGML_TYPE_Q4_1); assert(ne[0] % 64 == 0); break;
                default:
                        {
                            fprintf(stderr, "%s: unknown ftype %d in model file\n", __func__, ftype);
                            return false;
                        }
            };

            if ((nelements*bpe)/ggml_blck_size(tensor->type) != ggml_nbytes(tensor)) {
                fprintf(stderr, "%s: tensor '%s' has wrong size in model file: got %zu, expected %zu\n",
                        __func__, name.data(), ggml_nbytes(tensor), nelements*bpe);
                return false;
            }

            fin.read(reinterpret_cast<char *>(tensor->data), ggml_nbytes(tensor));

            //printf("%42s - [%5d, %5d], type = %6s, %6.2f MB\n", name.data(), ne[0], ne[1], ftype == 0 ? "float" : "f16", ggml_nbytes(tensor)/1024.0/1024.0);
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
bool gptj_model_load(const std::string & fname, gptj_model & model, gpt_vocab & vocab) {

    auto fin = std::ifstream(fname, std::ios::binary);
    if (!fin) {
        fprintf(stderr, "%s: failed to open '%s'\n", __func__, fname.c_str());
        return false;
    }

    bool loaded = gptj_model_load(fname, fin, model, vocab);
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

    const int d_key = n_embd/n_head;

    const size_t init_buf_size = 1024u*MB;
    if (!model.buf.addr || model.buf.size < init_buf_size)
        model.buf.resize(init_buf_size);

    if (mem_per_token > 0 && mem_per_token*N > model.buf.size) {
        const size_t buf_size_new = 1.1*(mem_per_token*N); // add 10% to account for ggml object overhead
        printf("\n%s: reallocating buffer from %zu to %zu bytes\n", __func__, model.buf.size, buf_size_new);

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
        struct ggml_tensor * cur;

        // norm
        {
            cur = ggml_norm(ctx0, inpL);

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
            struct ggml_tensor * Qcur = ggml_mul_mat(ctx0, model.layers[il].c_attn_q_proj_w, cur);
            struct ggml_tensor * Kcur = ggml_mul_mat(ctx0, model.layers[il].c_attn_k_proj_w, cur);
            struct ggml_tensor * Vcur = ggml_mul_mat(ctx0, model.layers[il].c_attn_v_proj_w, cur);

            // store key and value to memory
            {
                struct ggml_tensor * k = ggml_view_1d(ctx0, model.kv_self.k, N*n_embd, (ggml_element_size(model.kv_self.k)*n_embd)*(il*n_ctx + n_past));
                struct ggml_tensor * v = ggml_view_1d(ctx0, model.kv_self.v, N*n_embd, (ggml_element_size(model.kv_self.v)*n_embd)*(il*n_ctx + n_past));

                ggml_build_forward_expand(&gf, ggml_cpy(ctx0, Kcur, k));
                ggml_build_forward_expand(&gf, ggml_cpy(ctx0, Vcur, v));
            }

            // Q = Qcur.contiguous().view(n_embd/n_head, n_head, N).permute(0, 2, 1, 3)
            struct ggml_tensor * Q =
                ggml_permute(ctx0,
                        ggml_rope(ctx0,
                            ggml_cpy(ctx0,
                                Qcur,
                                ggml_new_tensor_3d(ctx0, GGML_TYPE_F32, n_embd/n_head, n_head, N)),
                            n_past, n_rot, 0),
                        0, 2, 1, 3);

            // K = Kmem.view(n_embd/n_head, n_head, n_past + N).permute(0, 2, 1, 3)
            struct ggml_tensor * K =
                ggml_permute(ctx0,
                        ggml_rope(ctx0,
                            ggml_reshape_3d(ctx0,
                                ggml_view_1d(ctx0, model.kv_self.k, (n_past + N)*n_embd, il*n_ctx*ggml_element_size(model.kv_self.k)*n_embd),
                                n_embd/n_head, n_head, n_past + N),
                            n_past, n_rot, 1),
                        0, 2, 1, 3);

            // K * Q
            struct ggml_tensor * KQ = ggml_mul_mat(ctx0, K, Q);

            // KQ_scaled = KQ / sqrt(n_embd/n_head)
            struct ggml_tensor * KQ_scaled =
                ggml_scale(ctx0,
                        KQ,
                        ggml_new_f32(ctx0, 1.0f/sqrt(float(n_embd)/n_head))
                        );

            // KQ_masked = mask_past(KQ_scaled)
            struct ggml_tensor * KQ_masked = ggml_diag_mask_inf(ctx0, KQ_scaled, n_past);

            // KQ = soft_max(KQ_masked)
            struct ggml_tensor * KQ_soft_max = ggml_soft_max(ctx0, KQ_masked);

            // V_trans = Vmem.view(n_embd/n_head, n_head, n_past + N).permute(1, 2, 0, 3).contiguous()
            struct ggml_tensor * V_trans =
                ggml_cpy(ctx0,
                        ggml_permute(ctx0,
                            ggml_reshape_3d(ctx0,
                                ggml_view_1d(ctx0, model.kv_self.v, (n_past + N)*n_embd, il*n_ctx*ggml_element_size(model.kv_self.v)*n_embd),
                                n_embd/n_head, n_head, n_past + N),
                            1, 2, 0, 3),
                        ggml_new_tensor_3d(ctx0, model.kv_self.v->type, n_past + N, n_embd/n_head, n_head));

            // KQV = transpose(V) * KQ_soft_max
            struct ggml_tensor * KQV = ggml_mul_mat(ctx0, V_trans, KQ_soft_max);

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

    // norm
    {
        inpL = ggml_norm(ctx0, inpL);

        // inpL = ln_f_g*inpL + ln_f_b
        inpL = ggml_add(ctx0,
                ggml_mul(ctx0,
                    ggml_repeat(ctx0, model.ln_f_g, inpL),
                    inpL),
                ggml_repeat(ctx0, model.ln_f_b, inpL));
    }

    // lm_head
    {
        inpL = ggml_mul_mat(ctx0, model.lmh_g, inpL);

        inpL = ggml_add(ctx0,
                ggml_repeat(ctx0, model.lmh_b, inpL),
                inpL);
    }

    // logits -> probs
    //inpL = ggml_soft_max(ctx0, inpL);

    // run the computation
    ggml_build_forward_expand(&gf, inpL);
    ggml_graph_compute       (ctx0, &gf);

    //if (n_past%100 == 0) {
    //    ggml_graph_print   (&gf);
    //    ggml_graph_dump_dot(&gf, NULL, "gpt-2.dot");
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
    const size_t expected = gptj_get_state_size(model);
    assert(written == expected);
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
    const size_t expected = gptj_get_state_size(*model);
    assert(nread == expected);
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
    d_ptr->modelLoaded = false;
}

bool GPTJ::loadModel(const std::string &modelPath) {
    std::mt19937 rng(time(NULL));
    d_ptr->rng = rng;

    auto fin = std::ifstream(modelPath, std::ios::binary);

    // load the model
    if (!gptj_model_load(modelPath, fin, *d_ptr->model, d_ptr->vocab)) {
        std::cerr << "GPT-J ERROR: failed to load model from " <<  modelPath;
        return false;
    }

    d_ptr->n_threads = std::min(4, (int32_t) std::thread::hardware_concurrency());
    d_ptr->modelLoaded = true;
    fflush(stdout);
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

void GPTJ::prompt(const std::string &prompt,
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
    std::vector<gpt_vocab::id> embd_inp = ::gpt_tokenize(d_ptr->vocab, prompt);

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
    static std::vector<gpt_vocab::id> p_instruct;
    static std::vector<gpt_vocab::id> r_instruct;
    if (!initialized) {
        gptj_eval(*d_ptr->model, d_ptr->n_threads, 0, { 0, 1, 2, 3 }, promptCtx.logits,
            d_ptr->mem_per_token);
        initialized = true;
    }

    // process the prompt in batches
    size_t i = 0;
    const int64_t t_start_prompt_us = ggml_time_us();
    while (i < embd_inp.size()) {
        size_t batch_end = std::min(i + promptCtx.n_batch, embd_inp.size());
        std::vector<gpt_vocab::id> batch(embd_inp.begin() + i, embd_inp.begin() + batch_end);

        // Check if the context has run out...
        if (promptCtx.n_past + batch.size() > promptCtx.n_ctx) {
            const int32_t erasePoint = promptCtx.n_ctx * promptCtx.contextErase;
            // Erase the first percentage of context from the tokens...
            std::cerr << "GPTJ: reached the end of the context window so resizing\n";
            promptCtx.tokens.erase(promptCtx.tokens.begin(), promptCtx.tokens.begin() + erasePoint);
            promptCtx.n_past = promptCtx.tokens.size();
            recalculateContext(promptCtx, recalculateCallback);
            assert(promptCtx.n_past + batch.size() <= promptCtx.n_ctx);
        }

        if (!gptj_eval(*d_ptr->model, d_ptr->n_threads, promptCtx.n_past, batch, promptCtx.logits,
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
    std::vector<gpt_vocab::id> cachedTokens;
    std::unordered_set<std::string> reversePrompts
        = { "### Instruction", "### Prompt", "### Response", "### Human", "### Assistant", "### Context" };

    // predict next tokens
    int32_t totalPredictions = 0;
    for (int i = 0; i < promptCtx.n_predict; i++) {

        // sample next token
        const int n_vocab = d_ptr->model->hparams.n_vocab;
        gpt_vocab::id id = 0;
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
            std::cerr << "GPTJ: reached the end of the context window so resizing\n";
            promptCtx.tokens.erase(promptCtx.tokens.begin(), promptCtx.tokens.begin() + erasePoint);
            promptCtx.n_past = promptCtx.tokens.size();
            recalculateContext(promptCtx, recalculateCallback);
            assert(promptCtx.n_past + 1 <= promptCtx.n_ctx);
        }

        const int64_t t_start_predict_us = ggml_time_us();
        if (!gptj_eval(*d_ptr->model, d_ptr->n_threads, promptCtx.n_past, { id }, promptCtx.logits,
            d_ptr->mem_per_token)) {
            std::cerr << "GPT-J ERROR: Failed to predict next token\n";
            return;
        }
        t_predict_us += ggml_time_us() - t_start_predict_us;

        promptCtx.n_past += 1;
        // display text
        ++totalPredictions;

        if (id == 50256 /*end of text*/)
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

void GPTJ::recalculateContext(PromptContext &promptCtx, std::function<bool(bool)> recalculate)
{
    size_t i = 0;
    promptCtx.n_past = 0;
    while (i < promptCtx.tokens.size()) {
        size_t batch_end = std::min(i + promptCtx.n_batch, promptCtx.tokens.size());
        std::vector<gpt_vocab::id> batch(promptCtx.tokens.begin() + i, promptCtx.tokens.begin() + batch_end);

        assert(promptCtx.n_past + batch.size() <= promptCtx.n_ctx);

        if (!gptj_eval(*d_ptr->model, d_ptr->n_threads, promptCtx.n_past, batch, promptCtx.logits,
            d_ptr->mem_per_token)) {
            std::cerr << "GPTJ ERROR: Failed to process prompt\n";
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
