#define FALCON_H_I_KNOW_WHAT_I_AM_DOING_WHEN_INCLUDING_THIS_FILE
#include "falcon_impl.h"
#include "llama.h"
#include "llama-util.h"
#include "utils.h"
#include "llmodel_shared.h"

#include <cassert>
#include <cinttypes>
#include <iostream>
#include <sstream>

namespace {
const char *modelType_ = "Falcon";
}

// commented out 40B support as it presently would require forking ggml/llama.cpp
// can re-add once mainline ggml supports it

#define FALCON_MAGIC 0x67676a74

// default hparams (Falcon 7B)
struct falcon_hparams {
    int32_t n_vocab = 65024;
    int32_t n_embd  = 4544;
    int32_t n_head  = 71;
    int32_t n_head_kv = 1;
    int32_t n_layer = 32;
    int32_t falcon_version = 7; // 7 for Falcon-7B, 40 for Falcon-40B
    int32_t ftype   = 1;
    int32_t n_ctx   = 2048;
};

struct falcon_layer {
    // normalization
    struct ggml_tensor* input_layernorm;
    struct ggml_tensor* input_layernorm_b;
    //struct ggml_tensor* attention_norm;    // Falcon-40B only
    //struct ggml_tensor* attention_norm_b;  // Falcon-40B only

    // attention
    struct ggml_tensor* query_key_value;
    struct ggml_tensor* wo;

    // ff
    struct ggml_tensor* ffn_up;
    struct ggml_tensor* ffn_down;
};

struct falcon_model {
    falcon_hparams hparams;

    struct ggml_tensor* tok_embeddings;
    struct ggml_tensor* output_norm;
    struct ggml_tensor* output_norm_b;
    struct ggml_tensor* lm_head;

    std::vector<falcon_layer> layers;

    // key + value memory
    llm_kv_cache kv_self;

    struct ggml_context* ctx;
    std::map<std::string, struct ggml_tensor*> tensors;

    llm_buffer eval_buf;
    llm_buffer scr0_buf;
    llm_buffer scr1_buf;
};

static bool kv_cache_init(
        const struct falcon_hparams & hparams,
              struct llm_kv_cache & cache,
                         ggml_type   wtype,
                               int   n_ctx) {
    const int n_embd  = hparams.n_embd;
    const int dim_head  = n_embd / hparams.n_head;
    const int dim_kv = dim_head * hparams.n_head_kv;
    const int n_layer = hparams.n_layer;

    const int64_t n_mem      = (int64_t)n_layer*n_ctx;
    const int64_t n_elements = dim_kv * n_mem;
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

// load the model's weights from a file
bool falcon_model_load(const std::string & fname, falcon_model & model, gpt_vocab & vocab, size_t *mem_req) {
    printf("%s: loading model from '%s' - please wait ...\n", __func__, fname.c_str());
    if (mem_req) {
        *mem_req = 0;
    }

    auto fin = std::ifstream(fname, std::ios::binary);
    if (!fin) {
        fprintf(stderr, "%s: failed to open '%s'\n", __func__, fname.c_str());
        return false;
    }

    // verify magic
    {
        uint32_t magic;
        fin.read((char *) &magic, sizeof(magic));
        if (magic != FALCON_MAGIC) {
            fprintf(stderr, "%s: invalid model file '%s' (bad magic)\n", __func__, fname.c_str());
            return false;
        }
    }

    uint32_t format_version;
    fin.read((char *) &format_version, sizeof(format_version));

    // load hparams
    {
        auto & hparams = model.hparams;

        fin.read((char *) &hparams.n_vocab, sizeof(hparams.n_vocab));
        fin.read((char *) &hparams.n_embd,  sizeof(hparams.n_embd));
        fin.read((char *) &hparams.n_head,  sizeof(hparams.n_head));
        fin.read((char *) &hparams.n_head_kv, sizeof(hparams.n_head_kv));
        fin.read((char *) &hparams.n_layer, sizeof(hparams.n_layer));
        fin.read((char *) &hparams.falcon_version, sizeof(hparams.falcon_version));
        fin.read((char *) &hparams.ftype,   sizeof(hparams.ftype));

        if (hparams.falcon_version != 7) { // && hparams.falcon_version != 40) {
            fprintf(stderr, "%s: invalid model file '%s' (bad Falcon version: %d)\n", __func__, fname.c_str(), hparams.falcon_version);
            return false;
        }

        const int32_t qntvr = hparams.ftype / GGML_QNT_VERSION_FACTOR;

        printf("%s: n_vocab   = %d\n", __func__, hparams.n_vocab);
        printf("%s: n_embd    = %d\n", __func__, hparams.n_embd);
        printf("%s: n_head    = %d\n", __func__, hparams.n_head);
        printf("%s: n_head_kv = %d\n", __func__, hparams.n_head_kv);
        printf("%s: n_layer   = %d\n", __func__, hparams.n_layer);
        printf("%s: ftype     = %d\n", __func__, hparams.ftype);
        printf("%s: qntvr     = %d\n", __func__, qntvr);

        hparams.ftype %= GGML_QNT_VERSION_FACTOR;
    }

    // load vocab
    {
        const int32_t n_vocab = model.hparams.n_vocab;

        std::string word;
        std::vector<char> buf(128);

        for (int i = 0; i < n_vocab; i++) {
            uint32_t len;
            fin.read((char *) &len, sizeof(len));

            buf.resize(len);
            fin.read((char *) buf.data(), len);
            word.assign(buf.data(), len);

            uint32_t dummy;
            fin.read((char *) &dummy, sizeof(dummy));

            vocab.token_to_id[word] = i;
            vocab.id_to_token[i] = word;
        }
    }

    // for the big tensors, we have the option to store the data in 16-bit floats or quantized
    // in order to save memory and also to speed up the computation
    ggml_type wtype = ggml_ftype_to_ggml_type((ggml_ftype) (model.hparams.ftype));
    if (wtype == GGML_TYPE_COUNT) {
        fprintf(stderr, "%s: invalid model file '%s' (bad ftype value %d)\n",
                __func__, fname.c_str(), model.hparams.ftype);
        return false;
    }

    auto & ctx = model.ctx;

    size_t ctx_size = 0;

    {
        const auto& hparams = model.hparams;

        const int n_embd = hparams.n_embd;
        const int n_head = hparams.n_head;
        const int n_head_kv = hparams.n_head_kv;
        const int n_layer = hparams.n_layer;
        const int n_ctx = hparams.n_ctx;
        const int n_ff = 4 * model.hparams.n_embd;
        const int n_vocab = hparams.n_vocab;
        const int head_dim = hparams.n_embd / hparams.n_head;

        ctx_size += ggml_tensor_overhead() + ggml_type_sizef(wtype) * n_embd * n_vocab;  // tok_embeddings
        ctx_size += ggml_tensor_overhead() + ggml_type_sizef(GGML_TYPE_F32) * n_embd;  // output_norm
        ctx_size += ggml_tensor_overhead() + ggml_type_sizef(GGML_TYPE_F32) * n_embd;  // output_norm_b
        ctx_size += ggml_tensor_overhead() + ggml_type_sizef(wtype) * n_embd * n_vocab;  // lm_head

        // if (hparams.version == 40) { // Falcon-40B
        //     ctx_size += n_layer * ggml_sizeof_tensor_1d(GGML_TYPE_F32, n_embd);  // attention_norm
        //     ctx_size += n_layer * ggml_sizeof_tensor_1d(GGML_TYPE_F32, n_embd);  // attention_norm_b
        // }
        ctx_size += n_layer * (ggml_tensor_overhead() + ggml_type_sizef(GGML_TYPE_F32) * n_embd);  // input_layernorm
        ctx_size += n_layer * (ggml_tensor_overhead() + ggml_type_sizef(GGML_TYPE_F32) * n_embd);  // input_layernorm_b
        ctx_size += n_layer * (ggml_tensor_overhead() + ggml_type_sizef(wtype) * n_embd * (n_head_kv * 2 + n_head) * head_dim);  // query_key_value
        ctx_size += n_layer * (ggml_tensor_overhead() + ggml_type_sizef(wtype) * n_embd * n_embd);  // wo
        ctx_size += n_layer * (ggml_tensor_overhead() + ggml_type_sizef(wtype) * n_embd * n_ff);  // ffn_up
        ctx_size += n_layer * (ggml_tensor_overhead() + ggml_type_sizef(wtype) * n_ff * n_embd);  // ffn_down
 
        printf("%s: ggml ctx size = %6.2f MB\n", __func__, ctx_size/(1024.0*1024.0));
    }

    if (mem_req) {
        const int n_embd  = model.hparams.n_embd;
        const int dim_head  = n_embd / model.hparams.n_head;
        const int dim_kv = dim_head * model.hparams.n_head_kv;
        const int n_layer = model.hparams.n_layer;

        const int64_t n_mem      = (int64_t)n_layer*model.hparams.n_ctx;
        const int64_t n_elements = dim_kv * n_mem;
        size_t kv_cache_size = 2u*n_elements*ggml_type_size(wtype) + 2_MiB;
        *mem_req = ctx_size + kv_cache_size;
        return false;
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
        const auto& hparams = model.hparams;

        const int n_embd = hparams.n_embd;
        const int n_head = hparams.n_head;
        const int n_head_kv = hparams.n_head_kv;
        const int n_layer = hparams.n_layer;
        const int n_ff = 4 * model.hparams.n_embd;
        const int n_vocab = hparams.n_vocab;
        const int head_dim = hparams.n_embd / hparams.n_head;

        model.layers.resize(n_layer);

        model.tok_embeddings = ggml_new_tensor_2d(ctx, wtype, n_embd, n_vocab);

        model.output_norm = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embd);
        model.output_norm_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embd);
        model.lm_head = ggml_new_tensor_2d(ctx, wtype, n_embd, n_vocab);

        // map by name
        model.tensors["transformer.word_embeddings.weight"] =
            model.tok_embeddings;

        model.tensors["transformer.ln_f.weight"] = model.output_norm;
        model.tensors["transformer.ln_f.bias"] = model.output_norm_b;
        model.tensors["lm_head.weight"] = model.lm_head;

        for (int i = 0; i < n_layer; ++i) {
            auto& layer = model.layers[i];

            layer.input_layernorm =
                ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embd);
            layer.input_layernorm_b =
                ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embd);

            // if (hparams.version == 40) { // for Falcon-40B only
            //     layer.attention_norm =
            //         ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embd);
            //     layer.attention_norm_b =
            //         ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embd);
            // }

            // query_key_value shape for config.multi_query == True:
            layer.query_key_value = ggml_new_tensor_2d(
                ctx, wtype, n_embd, (n_head_kv * 2 + n_head) * head_dim);
            layer.wo = ggml_new_tensor_2d(ctx, wtype, n_embd, n_embd);

            layer.ffn_up = ggml_new_tensor_2d(ctx, wtype, n_embd, n_ff);
            layer.ffn_down = ggml_new_tensor_2d(ctx, wtype, n_ff, n_embd);

            // map by name
            // if (hparams.version == 40) {
            //     // Falcon-40B:
            //     model.tensors["transformer.h." + std::to_string(i) +
            //                   ".ln_mlp.weight"] = layer.input_layernorm;
            //     model.tensors["transformer.h." + std::to_string(i) +
            //                   ".ln_mlp.bias"] = layer.input_layernorm_b;
            //     model.tensors["transformer.h." + std::to_string(i) +
            //                   ".ln_attn.weight"] = layer.attention_norm;
            //     model.tensors["transformer.h." + std::to_string(i) +
            //                   ".ln_attn.bias"] = layer.attention_norm_b;
            // } else {
            // Falcon-7B:
            model.tensors["transformer.h." + std::to_string(i) +
                          ".input_layernorm.weight"] = layer.input_layernorm;
            model.tensors["transformer.h." + std::to_string(i) +
                          ".input_layernorm.bias"] = layer.input_layernorm_b;
            //}

            model.tensors["transformer.h." + std::to_string(i) +
                          ".self_attention.query_key_value.weight"] =
                layer.query_key_value;
            model.tensors["transformer.h." + std::to_string(i) +
                          ".self_attention.dense.weight"] = layer.wo;

            model.tensors["transformer.h." + std::to_string(i) +
                          ".mlp.dense_h_to_4h.weight"] = layer.ffn_up;
            model.tensors["transformer.h." + std::to_string(i) +
                          ".mlp.dense_4h_to_h.weight"] = layer.ffn_down;
        }
    }

    // key + value memory
    {
        const auto & hparams = model.hparams;

        const int n_layer = hparams.n_layer;
        const int n_ctx   = hparams.n_ctx;
        const int n_head_kv = hparams.n_head_kv;
        const int head_dim = hparams.n_embd / hparams.n_head;

        const int64_t n_mem      = n_layer*n_ctx;
        const int64_t n_elements = head_dim*n_mem;

        if (!kv_cache_init(hparams, model.kv_self, GGML_TYPE_F32, model.hparams.n_ctx)) {
            fprintf(stderr, "%s: kv_cache_init() failed for self-attention cache\n", __func__);
            ggml_free(ctx);
            return false;
        }
        const size_t memory_size = ggml_nbytes(model.kv_self.k) + ggml_nbytes(model.kv_self.v);

        printf("%s: memory_size = %8.2f MB, n_mem = %" PRId64 "\n", __func__, memory_size/1024.0/1024.0, n_mem);
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
            fin.seekg(-static_cast<ptrdiff_t>(fin.tellg()) & 31, std::ios_base::cur);

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
                fprintf(stderr, "%s: tensor '%s' has wrong shape in model file: got [%5d, %5d], expected [%5d, %5d]\n",
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

            total_size += ggml_nbytes(tensor);
            if (++n_tensors % 8 == 0) {
                printf(".");
                fflush(stdout);
            }
        }

        printf(" done\n");

        printf("%s: model size = %8.2f MB / num tensors = %d\n", __func__, total_size/1024.0/1024.0, n_tensors);
    }

    fin.close();

    model.eval_buf.resize(1280u * 1024 * 1024);
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
bool falcon_eval(
        const falcon_model & model,
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
    const int n_head_kv = hparams.n_head_kv;
    const int n_vocab = hparams.n_vocab;
    const int version = hparams.falcon_version;
    const size_t head_dim = n_embd / n_head;

   struct ggml_init_params eval_ctx_params = {
        .mem_size = model.eval_buf.size,
        .mem_buffer = model.eval_buf.addr,
        .no_alloc = false,
    };

    struct ggml_context * ctx0 = ggml_init(eval_ctx_params);
    struct ggml_cgraph gf = {};
    gf.n_threads = n_threads;

    struct ggml_tensor * embd = ggml_new_tensor_1d(ctx0, GGML_TYPE_I32, N);
    memcpy(embd->data, embd_inp.data(), N*ggml_element_size(embd));

    // wte
    struct ggml_tensor * inpL = ggml_get_rows(ctx0, model.tok_embeddings, embd);
    struct ggml_tensor* repeat_dummy = ggml_new_tensor_3d(ctx0, inpL->type, head_dim, N + n_past, n_head);

    ggml_type wtype = GGML_TYPE_F32;
    const int sizeof_wtype = ggml_type_sizef(wtype);

    for (int il = 0; il < n_layer; ++il) {
        struct ggml_tensor * cur;
        struct ggml_tensor * layernorm_output;

        ggml_set_scratch(ctx0, {0, model.scr0_buf.size, model.scr0_buf.addr, });

        // self-attention
        {
            layernorm_output = ggml_norm(ctx0, inpL);

            layernorm_output = ggml_add(ctx0,
                    ggml_mul(ctx0,
                        ggml_repeat(ctx0, model.layers[il].input_layernorm, layernorm_output),
                        layernorm_output),
                    ggml_repeat(ctx0, model.layers[il].input_layernorm_b, layernorm_output));

            // if (version == 40) { // Falcon-40B only
            //     cur = ggml_norm(ctx0, inpL);

            //     cur = ggml_add(ctx0,
            //             ggml_mul(ctx0,
            //                 ggml_repeat(ctx0, model.layers[il].attention_norm, cur),
            //                 cur),
            //             ggml_repeat(ctx0, model.layers[il].attention_norm_b, cur));
            // }
            // else {
            cur = layernorm_output;
            // }

            // compute QKV

            cur = ggml_mul_mat(ctx0, model.layers[il].query_key_value, cur);

            // Note that the strides for Kcur, Vcur are set up so that the
            // resulting views are misaligned with the tensor's storage
            // (by applying the K/V offset we shift the tensor's original
            // view to stick out behind the viewed QKV tensor's allocated
            // memory, so to say). This is ok because no actual accesses
            // happen to that out-of-range memory, but it can require some
            // trickery when trying to accurately dump these views for
            // debugging.

            struct ggml_tensor * Qcur = ggml_view_3d(
                ctx0, cur, head_dim, n_head, N,
                head_dim * sizeof_wtype,
                head_dim * (n_head + 2 * n_head_kv) * sizeof_wtype,
                0);

            struct ggml_tensor * Kcur = ggml_view_3d(
                ctx0, cur, head_dim, n_head_kv, N,
                head_dim * sizeof_wtype,
                head_dim * (n_head + 2 * n_head_kv) * sizeof_wtype,
                head_dim * n_head * sizeof_wtype);

            struct ggml_tensor * Vcur = ggml_view_3d(
                ctx0, cur, head_dim, n_head_kv, N,
                head_dim * sizeof_wtype,
                head_dim * (n_head + 2 * n_head_kv) * sizeof_wtype,
                head_dim * (n_head + n_head_kv) * sizeof_wtype);

            // using mode = 2 for neox mode
            Qcur = ggml_rope_inplace(ctx0, Qcur, n_past, head_dim, 2);
            Kcur = ggml_rope_inplace(ctx0, Kcur, n_past, head_dim, 2);

            // store key and value to memory
            {
                struct ggml_tensor* k = ggml_view_1d(
                    ctx0, model.kv_self.k, N * n_head_kv * head_dim,
                    (ggml_element_size(model.kv_self.k) * n_head_kv * head_dim) *
                        (il * n_ctx + n_past));
                struct ggml_tensor* v = ggml_view_1d(
                    ctx0, model.kv_self.v, N * n_head_kv * head_dim,
                    (ggml_element_size(model.kv_self.v) * n_head_kv * head_dim) *
                        (il * n_ctx + n_past));

                ggml_build_forward_expand(&gf, ggml_cpy(ctx0, Kcur, k));
                ggml_build_forward_expand(&gf, ggml_cpy(ctx0, Vcur, v));
            }

            struct ggml_tensor * K = ggml_permute(
                ctx0,
                ggml_view_3d(
                    ctx0,
                    model.kv_self.k,
                    head_dim, n_head_kv, n_past + N,
                    head_dim * sizeof_wtype,
                    head_dim * n_head_kv * sizeof_wtype,
                    il * n_ctx * ggml_element_size(model.kv_self.k) * n_head_kv * head_dim),
                0, 2, 1, 3);

            // K * Q

            // changed from repeat2 back to repeat, will not support 40B!
            K = ggml_cont(ctx0, ggml_repeat(ctx0, K, repeat_dummy));

            struct ggml_tensor * Q = ggml_permute(ctx0, Qcur, 0, 2, 1, 3);
            struct ggml_tensor * KQ = ggml_mul_mat(ctx0, K, Q);

            // KQ_scaled = KQ / sqrt(n_embd/n_head)
            struct ggml_tensor * KQ_scaled =
                ggml_scale_inplace(ctx0,
                        KQ,
                        ggml_new_f32(ctx0, 1.0f/sqrt(float(head_dim)))
                        );

            // KQ_masked = mask_past(KQ_scaled)
            struct ggml_tensor * KQ_masked = ggml_diag_mask_inf_inplace(ctx0, KQ_scaled, n_past);

            // KQ = soft_max(KQ_masked)
            struct ggml_tensor * KQ_soft_max = ggml_soft_max_inplace(ctx0, KQ_masked);

            // V_trans = Vmem.view(n_embd/n_head, n_head, n_past + N).permute(1, 2, 0, 3).contiguous()
            struct ggml_tensor* V = ggml_permute(
                ctx0,
                ggml_view_3d(
                    ctx0,
                    model.kv_self.v,
                    head_dim, n_head_kv, n_past + N,
                    head_dim * sizeof_wtype,
                    head_dim * n_head_kv * sizeof_wtype,
                    il * n_ctx * ggml_element_size(model.kv_self.v) * n_head_kv * head_dim),
                0, 2, 1, 3);

            // changed from repeat2 back to repeat, will not support 40B!
            V = ggml_cont(ctx0, ggml_transpose(ctx0, ggml_repeat(ctx0, V, repeat_dummy)));

            // KQV = transpose(V) * KQ_soft_max
            struct ggml_tensor * KQV = ggml_mul_mat(ctx0, V, KQ_soft_max);

            // KQV_merged = KQV.permute(0, 2, 1, 3)
            struct ggml_tensor * KQV_merged = ggml_permute(ctx0, KQV, 0, 2, 1, 3);

            // cur = KQV_merged.contiguous().view(n_embd, N)
            cur = ggml_cpy(ctx0,
                    KQV_merged,
                    ggml_new_tensor_2d(ctx0, GGML_TYPE_F32, n_embd, N));

            // projection
            {
                cur = ggml_mul_mat(ctx0,
                        model.layers[il].wo,
                        cur);
            }
        }

        ggml_set_scratch(ctx0, {0, model.scr1_buf.size, model.scr1_buf.addr, });

        struct ggml_tensor* inpFF = layernorm_output;
        struct ggml_tensor* attn_out = ggml_cpy(
            ctx0, cur, ggml_new_tensor_2d(ctx0, GGML_TYPE_F32, n_embd, N));

        {
            cur = ggml_mul_mat(ctx0, model.layers[il].ffn_up, inpFF);
            cur = ggml_gelu(ctx0, cur);
            cur = ggml_mul_mat(ctx0, model.layers[il].ffn_down, cur);
        }

        cur = ggml_add(ctx0, cur, attn_out);
        cur = ggml_add(ctx0, cur, inpL);
        // input for next layer
        inpL = cur;
    }

    ggml_set_scratch(ctx0, {0, model.scr0_buf.size, model.scr0_buf.addr, });

    // norm
    {
        inpL = ggml_norm(ctx0, inpL);

        // inpL = ln_f_g*inpL + ln_f_b
        inpL = ggml_add(ctx0,
                ggml_mul(ctx0,
                    ggml_repeat(ctx0, model.output_norm, inpL),
                    inpL),
                ggml_repeat(ctx0, model.output_norm_b, inpL));
    }

    ggml_set_scratch(ctx0, { 0, 0, nullptr, });

    // lm_head
    {
        inpL = ggml_mul_mat(ctx0, model.lm_head, inpL);

        //inpL = ggml_add(ctx0,
        //        ggml_repeat(ctx0, model.lmh_b, inpL),
        //        inpL);
    }

    // logits -> probs
    //inpL = ggml_soft_max_inplace(ctx0, inpL);

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


#define MAX_RNG_STATE 64*1024
size_t falcon_get_state_size(const falcon_model &model) {
    const size_t s_rng_size        = sizeof(size_t);
    const size_t s_rng             = MAX_RNG_STATE;
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
    return s_total;
}

size_t falcon_copy_state_data(const falcon_model &model, const std::mt19937 &rng, uint8_t *dest)
{
    uint8_t * out = dest;
    // copy rng
    {
        std::stringstream rng_ss;
        rng_ss << rng;

        const size_t rng_size = rng_ss.str().size();
        char rng_buf[MAX_RNG_STATE];

        memset(&rng_buf[0], 0, MAX_RNG_STATE);
        memcpy(&rng_buf[0], rng_ss.str().data(), rng_ss.str().size());

        memcpy(out, &rng_size,   sizeof(rng_size));   out += sizeof(rng_size);
        memcpy(out, &rng_buf[0], MAX_RNG_STATE); out += MAX_RNG_STATE;
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
    assert(written == falcon_get_state_size(model));
    fflush(stdout);
    return written;
}

size_t falcon_set_state_data(falcon_model *model, std::mt19937 *rng, const uint8_t *src)
{
    const uint8_t * in = src;

    // set rng
    {
        size_t rng_size;
        char   rng_buf[MAX_RNG_STATE];

        memcpy(&rng_size,   in, sizeof(rng_size));    in += sizeof(rng_size);
        memcpy(&rng_buf[0], in, MAX_RNG_STATE); in += MAX_RNG_STATE;

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
    assert(nread == falcon_get_state_size(*model));
    fflush(stdout);
    return nread;
}

struct FalconPrivate {
    const std::string modelPath;
    bool modelLoaded;
    gpt_vocab vocab;
    falcon_model *model = nullptr;
    int64_t n_threads = 0;
    size_t mem_per_token = 0;
    std::mt19937 rng;
};

Falcon::Falcon() : d_ptr(new FalconPrivate) {
    d_ptr->model = new falcon_model;
    d_ptr->model->ctx = nullptr;
    d_ptr->modelLoaded = false;
}

Falcon::~Falcon() {
    if(d_ptr->model->ctx) {
        ggml_free(d_ptr->model->ctx);
        d_ptr->model->ctx = nullptr;
    }
    delete d_ptr->model;
}

bool Falcon::loadModel(const std::string &modelPath)
{
    std::mt19937 rng(time(NULL));
    d_ptr->rng = rng;

    // load the model
    if (!falcon_model_load(modelPath, *d_ptr->model, d_ptr->vocab, nullptr)) {
        std::cerr << "FALCON ERROR: failed to load model from " <<  modelPath;
        return false;
    }

    d_ptr->n_threads = std::min(4, (int32_t) std::thread::hardware_concurrency());
    d_ptr->modelLoaded = true;
    fflush(stdout);
    return true;
}

bool Falcon::isModelLoaded() const
{
    return d_ptr -> modelLoaded;
}

size_t Falcon::requiredMem(const std::string &modelPath)
{
    falcon_model dummy_model;
    gpt_vocab dummy_vocab;
    size_t mem_req;
    auto fin = std::ifstream(modelPath, std::ios::binary);
    falcon_model_load(modelPath, dummy_model, dummy_vocab, &mem_req);
    return mem_req;
}

size_t Falcon::stateSize() const
{
    return falcon_get_state_size(*d_ptr->model);
}

size_t Falcon::saveState(uint8_t *dest) const
{
    return falcon_copy_state_data(*d_ptr->model, d_ptr->rng, dest);
}

size_t Falcon::restoreState(const uint8_t *src)
{
    return falcon_set_state_data(d_ptr->model, &d_ptr->rng, src);
}

void Falcon::setThreadCount(int32_t n_threads)
{
    d_ptr->n_threads = n_threads;
}

int32_t Falcon::threadCount() const
{
    return d_ptr->n_threads;
}

std::vector<LLModel::Token> Falcon::tokenize(PromptContext &, const std::string &str) const
{
    return ::gpt_tokenize(d_ptr->vocab, str);
}

LLModel::Token Falcon::sampleToken(PromptContext &promptCtx) const
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

std::string Falcon::tokenToString(Token id) const
{
    return d_ptr->vocab.id_to_token[id];
}

bool Falcon::evalTokens(PromptContext &ctx, const std::vector<int32_t> &tokens) const
{
    // determine the required inference memory per token:
    static bool initialized = false;
    if (!initialized) {
        falcon_eval(*d_ptr->model, d_ptr->n_threads, 0, { 0, 1, 2, 3 }, ctx.logits,
            d_ptr->mem_per_token);
        initialized = true;
    }

    return falcon_eval(*d_ptr->model, d_ptr->n_threads, ctx.n_past, tokens, ctx.logits, d_ptr->mem_per_token);
}

int32_t Falcon::contextLength() const
{
    return d_ptr->model->hparams.n_ctx;
}

const std::vector<LLModel::Token> &Falcon::endTokens() const
{
    static const std::vector<LLModel::Token> out = { 11 };
    return out;
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
    uint32_t version = 0;
    f.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (magic != FALCON_MAGIC) {
         return false; 
    }
    falcon_hparams hparams;
    f.read(reinterpret_cast<char*>(&hparams), sizeof(hparams));
    // we're matching the file format of existing pre-converted models
    // compatible with ctransformers llama.cpp based format, which also
    // unfortunately shares its magic number what llama uses, so we now
    // differentiate by n_vocab
    // give some wiggle room over the max to allow for finetunes that expand the
    // vocabulary
    if (!(hparams.n_vocab >= 65024 && hparams.n_vocab <= 65100)) {
        return false;
    }
    if (hparams.falcon_version != 7) {
        return false;
    }
    return true;
}

DLL_EXPORT LLModel *construct() {
    return new Falcon;
}
}
