#define BERT_H_I_KNOW_WHAT_I_AM_DOING_WHEN_INCLUDING_THIS_FILE
#include "bert_impl.h"
#include "llmodel_shared.h"
#include "ggml.h"

#include <cassert>
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <regex>
#include <thread>
#include <algorithm>
#include <numeric>

//#define DEBUG_BERT

namespace {
const char *modelType_ = "Bert";
}

typedef int32_t bert_vocab_id;

// default hparams (all-MiniLM-L6-v2)
struct bert_hparams
{
    int32_t n_vocab = 30522;
    int32_t n_max_tokens = 512;
    int32_t n_embd = 256;
    int32_t n_intermediate = 1536;
    int32_t n_head = 12;
    int32_t n_layer = 6;
};

struct bert_layer
{
    // normalization
    struct ggml_tensor *ln_att_w;
    struct ggml_tensor *ln_att_b;

    struct ggml_tensor *ln_out_w;
    struct ggml_tensor *ln_out_b;

    // attention
    struct ggml_tensor *q_w;
    struct ggml_tensor *q_b;
    struct ggml_tensor *k_w;
    struct ggml_tensor *k_b;
    struct ggml_tensor *v_w;
    struct ggml_tensor *v_b;

    struct ggml_tensor *o_w;
    struct ggml_tensor *o_b;

    // ff
    struct ggml_tensor *ff_i_w;
    struct ggml_tensor *ff_i_b;

    struct ggml_tensor *ff_o_w;
    struct ggml_tensor *ff_o_b;
};

struct bert_vocab
{
    std::map<std::string, bert_vocab_id> token_to_id;
    std::map<std::string, bert_vocab_id> subword_token_to_id;

    std::map<bert_vocab_id, std::string> _id_to_token;
    std::map<bert_vocab_id, std::string> _id_to_subword_token;
};

struct bert_model
{
    bert_hparams hparams;

    // embeddings weights
    struct ggml_tensor *word_embeddings;
    struct ggml_tensor *token_type_embeddings;
    struct ggml_tensor *position_embeddings;
    struct ggml_tensor *ln_e_w;
    struct ggml_tensor *ln_e_b;

    std::vector<bert_layer> layers;

    struct ggml_context *ctx;
};

// Replacement for std::vector<uint8_t> that doesn't require zero-initialization.
struct bert_ctx
{
    bert_model model;
    bert_vocab vocab;

    size_t mem_per_token;
    int64_t mem_per_input;
    int32_t max_batch_n;
    llm_buffer buf_compute;
    llm_buffer work_buf;
};

int32_t bert_n_embd(bert_ctx * ctx)
{
    return ctx->model.hparams.n_embd;
}

int32_t bert_n_max_tokens(bert_ctx * ctx)
{
    return ctx->model.hparams.n_max_tokens;
}

const char* bert_vocab_id_to_token(bert_ctx * ctx, bert_vocab_id id) {
    bert_vocab & vocab = ctx->vocab;
    auto it = vocab._id_to_token.find(id);
    if (it != vocab._id_to_token.end())
    {
        return it->second.c_str();
    }
    it = vocab._id_to_subword_token.find(id);
    if (it != vocab._id_to_subword_token.end())
    {
        return it->second.c_str();
    }
    return "[UNK TOKEN from bert_vocab]";
}

//
// Tokenizing
//

static size_t utf8_len(char src)
{
    const size_t lookup[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 4};
    uint8_t highbits = static_cast<uint8_t>(src) >> 4;
    return lookup[highbits];
}

std::string stripAccents(const std::string &inputString)
{
    std::string resultString;
    std::map<std::string, char> accentMap = {{"À", 'A'},{"Á", 'A'},
        {"Â", 'A'},{"Ã", 'A'},{"Ä", 'A'},{"Å", 'A'},{"à", 'a'},{"á", 'a'},
        {"â", 'a'},{"ã", 'a'},{"ä", 'a'},{"å", 'a'},{"È", 'E'},{"É", 'E'},
        {"Ê", 'E'},{"Ë", 'E'},{"è", 'e'},{"é", 'e'},{"ê", 'e'},{"ë", 'e'},
        {"Ì", 'I'},{"Í", 'I'},{"Î", 'I'},{"Ï", 'I'},{"ì", 'i'},{"í", 'i'},
        {"î", 'i'},{"ï", 'i'},{"Ò", 'O'},{"Ó", 'O'},{"Ô", 'O'},{"Õ", 'O'},
        {"Ö", 'O'},{"ò", 'o'},{"ó", 'o'},{"ô", 'o'},{"õ", 'o'},{"ö", 'o'},
        {"Ù", 'U'},{"Ú", 'U'},{"Û", 'U'},{"Ü", 'U'},{"ù", 'u'},{"ú", 'u'},
        {"û", 'u'},{"ü", 'u'},{"Ý", 'Y'},{"ý", 'y'},{"Ç", 'C'},{"ç", 'c'},
        {"Ñ", 'N'},{"ñ", 'n'},
    };

    for (size_t i = 0; i < inputString.length();)
    {
        int len = utf8_len(inputString[i]);
        std::string curChar = inputString.substr(i, len);
        auto iter = accentMap.find(curChar);
        if (iter != accentMap.end())
        {
            resultString += iter->second;
        }
        else
        {
            resultString += curChar;
        }
        i += len;
    }

    return resultString;
}

std::string bert_normalize_prompt(const std::string &text)
{
    // TODO: handle chinese characters? https://github.com/huggingface/tokenizers/blob/ef5f50605ddf9f8caef1598c0e4853862b9707a7/tokenizers/src/normalizers/bert.rs#L98
    std::string text2 = stripAccents(text);
    for (size_t i = 0; i < text2.size(); i += utf8_len(text2[i]))
    {
        char c = text2[i];
        if (c >= 'A' && c <= 'Z')
            text2[i] = c - 'A' + 'a';
    }
    return text2;
}

std::vector<bert_vocab_id> bert_tokenize(
    struct bert_ctx * ctx,
    const char * text)
{
    const bert_vocab &vocab = ctx->vocab;

    std::string str = text;

    std::vector<std::string> words;
    // first split the text into words
    {
        str = bert_normalize_prompt(str);

        std::string pat = R"([[:punct:]]|[[:alpha:]]+|[[:digit:]]+)";

        std::regex re(pat);
        std::smatch m;

        while (std::regex_search(str, m, re))
        {
            for (std::string x : m)
            {
                words.push_back(x);
            }
            str = m.suffix();
        }
    }

    // find the longest tokens that form the words:
    std::vector<bert_vocab_id> tokens;
    int cls_tok_id = 101;
    tokens.push_back(cls_tok_id);
    for (const auto &word : words)
    {
        if (word.size() == 0)
            continue;

        int i = 0;
        int n = word.size();
        auto *token_map = &vocab.token_to_id;
        while (i < n)
        {
            int j = n;
            while (j > i)
            {
                auto it = token_map->find(word.substr(i, j - i));
                if (it != token_map->end())
                {
                    tokens.push_back(it->second);
                    i = j;
                    token_map = &vocab.subword_token_to_id;
                }
                --j;
            }
            if (j == i)
            {
                fprintf(stderr, "%s: unknown token '%s'\n", __func__, word.substr(i, 1).data());
                token_map = &vocab.subword_token_to_id;
                ++i;
            }
        }
    }

    return tokens;
}

void bert_resize_ctx(bert_ctx * ctx, int32_t new_size) {
    int64_t buf_size_new = ctx->mem_per_input * new_size;

    // TODO: Max memory should be a param? Now just 1 GB
    int64_t GB = 1 << 30;
#if defined(DEBUG_BERT)
    printf("%s: requested_buf_size %lldMB\n", __func__, buf_size_new / (1 << 20));
#endif
    if (buf_size_new > GB) {
        int32_t adjusted_new_size = GB / ctx->mem_per_input;
        if (adjusted_new_size < 1) adjusted_new_size = 1;
#if defined(DEBUG_BERT)
        printf("%s: requested batch size %d, actual new batch size %d\n", __func__, new_size, adjusted_new_size);
#endif
        new_size = adjusted_new_size;
        buf_size_new = ctx->mem_per_input * new_size;
    }
    if (new_size > ctx->max_batch_n) {
        ctx->buf_compute.resize(buf_size_new);
        ctx->max_batch_n = new_size;
    }
}

void bert_eval(
    struct bert_ctx *ctx,
    int32_t n_threads,
    const bert_vocab_id *raw_tokens,
    int32_t n_tokens,
    float *embeddings)
{
    const bert_model& model = ctx->model;
    bool mem_req_mode = !embeddings;

    // batch_embeddings is nullptr for the initial memory requirements run
    if (!mem_req_mode && 1 > ctx->max_batch_n)
        bert_resize_ctx(ctx, 1);

    const int N = n_tokens;
    const auto &tokens = raw_tokens;

    const auto &hparams = model.hparams;

    const int n_embd = hparams.n_embd;
    const int n_layer = hparams.n_layer;
    const int n_max_tokens = hparams.n_max_tokens;
    const int n_head = hparams.n_head;

    const int d_head = n_embd / n_head;

    std::vector<float> result;
    if (N > n_max_tokens)
    {
        fprintf(stderr, "Too many tokens, maximum is %d\n", n_max_tokens);
        return;
    }

    auto & mem_per_token = ctx->mem_per_token;
    auto & buf_compute   = ctx->buf_compute;

    struct ggml_init_params params = {
        .mem_size = buf_compute.size,
        .mem_buffer = buf_compute.addr,
        .no_alloc = false,
    };

    struct ggml_context *ctx0 = ggml_init(params);
    struct ggml_cgraph *gf = ggml_new_graph(ctx0);

    // Embeddings. word_embeddings + token_type_embeddings + position_embeddings
    struct ggml_tensor *token_layer = ggml_new_tensor_1d(ctx0, GGML_TYPE_I32, N);
    memcpy(token_layer->data, tokens, N * ggml_element_size(token_layer));

    struct ggml_tensor *token_types = ggml_new_tensor_1d(ctx0, GGML_TYPE_I32, N);
    ggml_set_zero(token_types);

    struct ggml_tensor *positions = ggml_new_tensor_1d(ctx0, GGML_TYPE_I32, N);
    for (int i = 0; i < N; i++)
    {
        ggml_set_i32_1d(positions, i, i);
    }

    struct ggml_tensor *inpL = ggml_get_rows(ctx0, model.word_embeddings, token_layer);

    inpL = ggml_add(ctx0,
                    ggml_get_rows(ctx0, model.token_type_embeddings, token_types),
                    inpL);
    inpL = ggml_add(ctx0,
                    ggml_get_rows(ctx0, model.position_embeddings, positions),
                    inpL);

    // embd norm
    {
        inpL = ggml_norm(ctx0, inpL, 1e-5f);

        inpL = ggml_add(ctx0,
                        ggml_mul(ctx0,
                                 ggml_repeat(ctx0, model.ln_e_w, inpL),
                                 inpL),
                        ggml_repeat(ctx0, model.ln_e_b, inpL));
    }
    // layers
    for (int il = 0; il < n_layer; il++)
    {
        struct ggml_tensor *cur = inpL;

        // self-attention
        {
            struct ggml_tensor *Qcur = cur;
            Qcur = ggml_reshape_3d(ctx0,
                                   ggml_add(ctx0, ggml_repeat(ctx0, model.layers[il].q_b, Qcur),
                                            ggml_mul_mat(ctx0, model.layers[il].q_w, Qcur)),
                                   d_head, n_head, N);
            struct ggml_tensor *Q = ggml_permute(ctx0, Qcur, 0, 2, 1, 3);

            struct ggml_tensor *Kcur = cur;
            Kcur = ggml_reshape_3d(ctx0,
                                   ggml_add(ctx0, ggml_repeat(ctx0, model.layers[il].k_b, Kcur),
                                            ggml_mul_mat(ctx0, model.layers[il].k_w, Kcur)),
                                   d_head, n_head, N);
            struct ggml_tensor *K = ggml_permute(ctx0, Kcur, 0, 2, 1, 3);

            struct ggml_tensor *Vcur = cur;
            Vcur = ggml_reshape_3d(ctx0,
                                   ggml_add(ctx0, ggml_repeat(ctx0, model.layers[il].v_b, Vcur),
                                            ggml_mul_mat(ctx0, model.layers[il].v_w, Vcur)),
                                   d_head, n_head, N);
            struct ggml_tensor *V = ggml_permute(ctx0, Vcur, 0, 2, 1, 3);

            struct ggml_tensor *KQ = ggml_mul_mat(ctx0, K, Q);
            // KQ = soft_max(KQ / sqrt(head width))
            KQ = ggml_soft_max(
                ctx0, ggml_scale(ctx0, KQ, 1.0f / sqrt((float)d_head))
            );

            V = ggml_cont(ctx0, ggml_transpose(ctx0, V));
            struct ggml_tensor *KQV = ggml_mul_mat(ctx0, V, KQ);
            KQV = ggml_permute(ctx0, KQV, 0, 2, 1, 3);

            cur = ggml_cpy(ctx0,
                           KQV,
                           ggml_new_tensor_2d(ctx0, GGML_TYPE_F32, n_embd, N));
        }
        // attention output
        cur = ggml_add(ctx0,
                       ggml_repeat(ctx0, model.layers[il].o_b, cur),
                       ggml_mul_mat(ctx0, model.layers[il].o_w, cur));

        // re-add the layer input
        cur = ggml_add(ctx0, cur, inpL);

        // attention norm
        {
            cur = ggml_norm(ctx0, cur, 1e-5f);

            cur = ggml_add(ctx0,
                           ggml_mul(ctx0,
                                    ggml_repeat(ctx0, model.layers[il].ln_att_w, cur),
                                    cur),
                           ggml_repeat(ctx0, model.layers[il].ln_att_b, cur));
        }
        struct ggml_tensor *att_output = cur;
        // intermediate_output = self.intermediate(attention_output)
        cur = ggml_mul_mat(ctx0, model.layers[il].ff_i_w, cur);
        cur = ggml_add(ctx0,
                       ggml_repeat(ctx0, model.layers[il].ff_i_b, cur),
                       cur);
        cur = ggml_gelu(ctx0, cur);

        // layer_output = self.output(intermediate_output, attention_output)
        cur = ggml_mul_mat(ctx0, model.layers[il].ff_o_w, cur);
        cur = ggml_add(ctx0,
                       ggml_repeat(ctx0, model.layers[il].ff_o_b, cur),
                       cur);
        // attentions bypass the intermediate layer
        cur = ggml_add(ctx0, att_output, cur);

        // output norm
        {
            cur = ggml_norm(ctx0, cur, 1e-5f);

            cur = ggml_add(ctx0,
                           ggml_mul(ctx0,
                                    ggml_repeat(ctx0, model.layers[il].ln_out_w, cur),
                                    cur),
                           ggml_repeat(ctx0, model.layers[il].ln_out_b, cur));
        }
        inpL = cur;
    }
    inpL = ggml_cont(ctx0, ggml_transpose(ctx0, inpL));
    // pooler
    struct ggml_tensor *sum = ggml_new_tensor_2d(ctx0, GGML_TYPE_F32, N, 1);
    ggml_set_f32(sum, 1.0f / N);
    inpL = ggml_mul_mat(ctx0, inpL, sum);

    ggml_tensor *output = inpL;
    // run the computation
    ggml_build_forward_expand(gf, output);
    //ggml_graph_compute_g4a()
    ggml_graph_compute_g4a(ctx->work_buf, gf, n_threads);
    //ggml_graph_compute(ctx0, gf);


    // float *dat = ggml_get_data_f32(output);
    // pretty_print_tensor(dat, output->ne, output->nb, output->n_dims - 1, "");

    #ifdef GGML_PERF
        // print timing information per ggml operation (for debugging purposes)
        // requires GGML_PERF to be defined
        ggml_graph_print(gf);
    #endif

    if (!mem_req_mode) {
        memcpy(embeddings, (float *)ggml_get_data(output), sizeof(float) * n_embd);
    } else {
        mem_per_token = ggml_used_mem(ctx0) / N;
    }

    // printf("used_mem = %zu KB \n", ggml_used_mem(ctx0) / 1024);
    // printf("mem_per_token = %zu KB \n", mem_per_token / 1024);

    ggml_free(ctx0);
}

//
// Loading and setup
//

void bert_free(bert_ctx * ctx) {
    delete ctx;
}

struct bert_ctx * bert_load_from_file(const char *fname)
{
#if defined(DEBUG_BERT)
    printf("%s: loading model from '%s' - please wait ...\n", __func__, fname);
#endif

    bert_ctx * new_bert = new bert_ctx;

    bert_model & model = new_bert->model;
    bert_vocab & vocab = new_bert->vocab;

    struct gguf_init_params params = {
        /*.no_alloc = */ false,
        /*.ctx      = */ &model.ctx,
    };
    gguf_context *ggufctx = gguf_init_from_file(fname, params);
    if (!ggufctx) {
        fprintf(stderr, "%s: gguf_init_from_file() failed\n", __func__);
        return nullptr;
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
            return nullptr;
        }
        if (strcmp(gguf_get_val_str(ggufctx, keyidx), "bert") != 0) {
            fprintf(stderr, "%s: model architecture not supported!\n", __func__);
            return nullptr;
        }
    }

    // load hparams
    {
        auto &hparams = model.hparams;

        bool ok = false;
        int keyidx;

        do {
            keyidx = gguf_find_key(ggufctx, "bert.context_length");
            if (keyidx == -1) { break; }
            hparams.n_max_tokens = gguf_get_val_u32(ggufctx, keyidx);

            keyidx = gguf_find_key(ggufctx, "bert.embedding_length");
            if (keyidx == -1) { break; }
            hparams.n_embd = gguf_get_val_u32(ggufctx, keyidx);

            keyidx = gguf_find_key(ggufctx, "bert.feed_forward_length");
            if (keyidx == -1) { break; }
            hparams.n_intermediate = gguf_get_val_u32(ggufctx, keyidx);

            keyidx = gguf_find_key(ggufctx, "bert.attention.head_count");
            if (keyidx == -1) { break; }
            hparams.n_head = gguf_get_val_u32(ggufctx, keyidx);

            keyidx = gguf_find_key(ggufctx, "bert.block_count");
            if (keyidx == -1) { break; }
            hparams.n_layer = gguf_get_val_u32(ggufctx, keyidx);

            ok = true;
        } while (false);

        if (!ok) {
            fprintf(stderr, "%s: required hparam missing!\n", __func__);
            return nullptr;
        }

#if defined(DEBUG_BERT)
        printf("%s: n_max_tokens   = %d\n", __func__, hparams.n_max_tokens);
        printf("%s: n_embd         = %d\n", __func__, hparams.n_embd);
        printf("%s: n_intermediate = %d\n", __func__, hparams.n_intermediate);
        printf("%s: n_head         = %d\n", __func__, hparams.n_head);
        printf("%s: n_layer        = %d\n", __func__, hparams.n_layer);
#endif
    }

    // load vocab
    {
        auto & hparams = model.hparams;

        int keyidx = gguf_find_key(ggufctx, "tokenizer.ggml.model");
        if (keyidx == -1) {
            fprintf(stderr, "%s: tokenizer model not found!\n", __func__);
            return nullptr;
        }
        if (strcmp(gguf_get_val_str(ggufctx, keyidx), "bert") != 0) {
            fprintf(stderr, "%s: tokenizer model not supported!\n", __func__);
            return nullptr;
        }

        int tokens_keyidx = gguf_find_key(ggufctx, "tokenizer.ggml.tokens");
        if (tokens_keyidx == -1) {
            fprintf(stderr, "%s: bert tokenizer vocab not found!\n", __func__);
            return nullptr;
        }

        hparams.n_vocab = gguf_get_arr_n(ggufctx, tokens_keyidx);
        printf("%s: bert tokenizer vocab = %d\n", __func__, int(hparams.n_vocab));

        for (int i = 0; i < hparams.n_vocab; i++) {
            std::string word = gguf_get_arr_str(ggufctx, tokens_keyidx, i);

            if (word[0] == '#' && word[1] == '#')
            {
                vocab.subword_token_to_id[word.substr(2)] = i;
                vocab._id_to_subword_token[i] = word;
            }

            if (vocab.token_to_id.count(word) == 0)
            {
                vocab.token_to_id[word] = i;
                vocab._id_to_token[i] = word;
            }
        }
    }

    auto &ctx = model.ctx;

#if defined(DEBUG_BERT)
    printf("%s: ggml ctx size = %6.2f MB\n", __func__, ggml_get_mem_size(ctx) / (1024.0 * 1024.0));
#endif

    // prepare memory for the weights
    {
        const int n_layer = model.hparams.n_layer;
        model.layers.resize(n_layer);

        model.word_embeddings       = ggml_get_tensor(ctx, "token_embd.weight");
        model.token_type_embeddings = ggml_get_tensor(ctx, "token_types.weight");
        model.position_embeddings   = ggml_get_tensor(ctx, "position_embd.weight");
        model.ln_e_w                = ggml_get_tensor(ctx, "output_norm.weight");
        model.ln_e_b                = ggml_get_tensor(ctx, "output_norm.bias");

        auto name = [](int i, std::string n) {
            static std::string key;
            key = "blk." + std::to_string(i) + "." + n;
            return key.c_str();
        };

        for (int i = 0; i < n_layer; ++i)
        {
            auto &layer = model.layers[i];

            layer.ln_att_w = ggml_get_tensor(ctx, name(i, "attn_norm.weight"));
            layer.ln_att_b = ggml_get_tensor(ctx, name(i, "attn_norm.bias"));
            layer.ln_out_w = ggml_get_tensor(ctx, name(i, "ffn_norm.weight"));
            layer.ln_out_b = ggml_get_tensor(ctx, name(i, "ffn_norm.bias"));
            layer.q_w      = ggml_get_tensor(ctx, name(i, "attn_q.weight"));
            layer.q_b      = ggml_get_tensor(ctx, name(i, "attn_q.bias"));
            layer.k_w      = ggml_get_tensor(ctx, name(i, "attn_k.weight"));
            layer.k_b      = ggml_get_tensor(ctx, name(i, "attn_k.bias"));
            layer.v_w      = ggml_get_tensor(ctx, name(i, "attn_v.weight"));
            layer.v_b      = ggml_get_tensor(ctx, name(i, "attn_v.bias"));
            layer.o_w      = ggml_get_tensor(ctx, name(i, "attn_output.weight"));
            layer.o_b      = ggml_get_tensor(ctx, name(i, "attn_output.bias"));
            layer.ff_i_w   = ggml_get_tensor(ctx, name(i, "ffn_up.weight"));
            layer.ff_i_b   = ggml_get_tensor(ctx, name(i, "ffn_up.bias"));
            layer.ff_o_w   = ggml_get_tensor(ctx, name(i, "ffn_down.weight"));
            layer.ff_o_b   = ggml_get_tensor(ctx, name(i, "ffn_down.bias"));
        }
    }

    // Calculate space requirements for setting up context buffers later
    {
        bert_vocab_id tokens[] = {0, 1, 2, 3};
        // TODO: We set the initial buffer size to 16MB and hope it's enough. Maybe there is a better way to do this?
        new_bert->buf_compute.resize(16 * 1024 * 1024);
        bert_eval(new_bert, 1, tokens, 4, nullptr);
        new_bert->max_batch_n = 0;

        // TODO: Max tokens should be a param?
        int32_t N = new_bert->model.hparams.n_max_tokens;
        new_bert->mem_per_input = 2.2 * (new_bert->mem_per_token * N); // add 10% to account for ggml object overhead

    }
#if defined(DEBUG_BERT)
    printf("%s: mem_per_token %ld KB, mem_per_input %ld MB\n", __func__, new_bert->mem_per_token / (1 << 10), new_bert->mem_per_input / (1 << 20));
#endif

    return new_bert;
}

struct BertPrivate {
    const std::string modelPath;
    bool modelLoaded;
    bert_ctx *ctx = nullptr;
    int64_t n_threads = 0;
};

Bert::Bert() : d_ptr(new BertPrivate) {
    d_ptr->modelLoaded = false;
}

Bert::~Bert() {
    bert_free(d_ptr->ctx);
}

bool Bert::loadModel(const std::string &modelPath, int n_ctx, int ngl)
{
    (void)n_ctx;
    (void)ngl;
    d_ptr->modelLoaded = false;

    auto * ctx = bert_load_from_file(modelPath.c_str());
    fflush(stdout);
    if (!ctx)
        return false;

    d_ptr->ctx = ctx;
    d_ptr->n_threads = std::min(4, (int32_t) std::thread::hardware_concurrency());
    d_ptr->modelLoaded = true;
    return true;
}

bool Bert::isModelLoaded() const
{
    return d_ptr->modelLoaded;
}

size_t Bert::requiredMem(const std::string &modelPath, int n_ctx, int ngl)
{
    (void)modelPath;
    (void)n_ctx;
    (void)ngl;
    return 0;
}

size_t Bert::stateSize() const
{
    return 0;
}

size_t Bert::saveState(uint8_t */*dest*/) const
{
    return 0;
}

size_t Bert::restoreState(const uint8_t */*src*/)
{
    return 0;
}

void Bert::setThreadCount(int32_t n_threads)
{
    d_ptr->n_threads = n_threads;
}

int32_t Bert::threadCount() const
{
    return d_ptr->n_threads;
}

std::vector<float> Bert::embedding(const std::string &text)
{
    const int overlap = 32;
    const LLModel::Token clsToken = 101;
    const size_t contextLength = bert_n_max_tokens(d_ptr->ctx);
    typedef std::vector<LLModel::Token> TokenString;
    TokenString tokens = ::bert_tokenize(d_ptr->ctx, text.c_str());
#if defined(DEBUG_BERT)
    std::cerr << "embedding: " << tokens.size()
              << " contextLength " << contextLength
              << "\n";
#endif
    std::vector<double> embeddingsSum(bert_n_embd(d_ptr->ctx), 0);
    int embeddingsSumTotal = 0;
    size_t start_pos = 0;
    bool isFirstChunk = true;
    while (start_pos < tokens.size()) {
        TokenString chunk;
        if (!isFirstChunk)
            chunk.push_back(clsToken);
        const size_t l = isFirstChunk ? contextLength : contextLength - 1;
        if (tokens.size() - start_pos > l) {
            chunk.insert(chunk.end(), tokens.begin() + start_pos, tokens.begin() + start_pos + l);
            start_pos = start_pos + contextLength - overlap;
        } else {
            chunk.insert(chunk.end(), tokens.begin() + start_pos, tokens.end());
            start_pos = tokens.size();
        }
#if defined(DEBUG_BERT)
        std::cerr << "chunk length: " << chunk.size()
            << " embeddingsSumTotal " << embeddingsSumTotal
            << " contextLength " << contextLength
            << " start_pos " << start_pos
            << "\n";
#endif
        embeddingsSumTotal++;
        std::vector<float> embeddings(bert_n_embd(d_ptr->ctx));
        bert_eval(d_ptr->ctx, d_ptr->n_threads, chunk.data(), chunk.size(), embeddings.data());
        std::transform(embeddingsSum.begin(), embeddingsSum.end(), embeddings.begin(), embeddingsSum.begin(), std::plus<float>());
        isFirstChunk = false;
    }

    std::transform(embeddingsSum.begin(), embeddingsSum.end(), embeddingsSum.begin(), [embeddingsSumTotal](float num){ return num / embeddingsSumTotal; });
    double magnitude = std::sqrt(std::inner_product(embeddingsSum.begin(), embeddingsSum.end(), embeddingsSum.begin(), 0.0));
    for (auto &value : embeddingsSum)
        value /= magnitude;
    std::vector<float> finalEmbeddings(embeddingsSum.begin(), embeddingsSum.end());
    return finalEmbeddings;
}

std::vector<LLModel::Token> Bert::tokenize(PromptContext &, const std::string &str) const
{
    return ::bert_tokenize(d_ptr->ctx, str.c_str());
}

LLModel::Token Bert::sampleToken(PromptContext &/*promptCtx*/) const
{
    return 999 /*!*/;
}

std::string Bert::tokenToString(Token id) const
{
    return bert_vocab_id_to_token(d_ptr->ctx, id);
}

bool Bert::evalTokens(PromptContext &ctx, const std::vector<int32_t> &tokens) const
{
    std::vector<float> embeddings(bert_n_embd(d_ptr->ctx));
    int32_t cls = 101;
    const bool useCLS = tokens.front() != cls;
    if (useCLS) {
        std::vector<int32_t> myTokens;
        myTokens.push_back(cls);
        myTokens.insert(myTokens.end(), tokens.begin(), tokens.end());
        bert_eval(d_ptr->ctx, d_ptr->n_threads, myTokens.data(), myTokens.size(), embeddings.data());
    } else
        bert_eval(d_ptr->ctx, d_ptr->n_threads, tokens.data(), tokens.size(), embeddings.data());
    ctx.n_past = 0; // bert does not store any context
    return true;
}

int32_t Bert::contextLength() const
{
    return bert_n_max_tokens(d_ptr->ctx);
}

const std::vector<LLModel::Token> &Bert::endTokens() const
{
    static const std::vector<LLModel::Token> out = { 102 /*sep*/};
    return out;
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
    isValid = isValid && get_arch_name(ctx_gguf) == "bert";

    gguf_free(ctx_gguf);
    return isValid;
}

DLL_EXPORT LLModel *construct() {
    return new Bert;
}
}
