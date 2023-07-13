#define BERT_H_I_KNOW_WHAT_I_AM_DOING_WHEN_INCLUDING_THIS_FILE
#include "bert_impl.h"
#include "ggml.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
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
    int32_t f16 = 1;
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
    std::map<std::string, struct ggml_tensor *> tensors;
};

// Replacement for std::vector<uint8_t> that doesn't require zero-initialization.
struct bert_buffer {
    uint8_t * data = NULL;
    size_t size = 0;

    void resize(size_t size) {
        delete[] data;
        data = new uint8_t[size];
        this->size = size;
    }

    ~bert_buffer() {
        delete[] data;
    }
};


struct bert_ctx
{
    bert_model model;
    bert_vocab vocab;

    size_t mem_per_token;
    int64_t mem_per_input;
    int32_t max_batch_n;
    bert_buffer buf_compute;
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
        .mem_buffer = buf_compute.data,
        .no_alloc = false,
    };

    struct ggml_context *ctx0 = ggml_init(params);
    struct ggml_cgraph gf = {};
    gf.n_threads = n_threads;

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
        inpL = ggml_norm(ctx0, inpL);

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
            KQ = ggml_soft_max(ctx0,
                               ggml_scale(ctx0,
                                          KQ,
                                          ggml_new_f32(ctx0, 1.0f / sqrt((float)d_head))));

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
            cur = ggml_norm(ctx0, cur);

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
            cur = ggml_norm(ctx0, cur);

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
    ggml_build_forward_expand(&gf, output);
    ggml_graph_compute(ctx0, &gf);


    // float *dat = ggml_get_data_f32(output);
    // pretty_print_tensor(dat, output->ne, output->nb, output->n_dims - 1, "");

    #ifdef GGML_PERF
        // print timing information per ggml operation (for debugging purposes)
        // requires GGML_PERF to be defined
        ggml_graph_print(&gf);
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
    ggml_free(ctx->model.ctx);
    delete ctx;
}

struct bert_ctx * bert_load_from_file(const char *fname)
{
#if defined(DEBUG_BERT)
    printf("%s: loading model from '%s' - please wait ...\n", __func__, fname);
#endif

    auto fin = std::ifstream(fname, std::ios::binary);
    if (!fin)
    {
        fprintf(stderr, "%s: failed to open '%s'\n", __func__, fname);
        return nullptr;
    }

    // verify magic
    {
        uint32_t magic;
        fin.read((char *)&magic, sizeof(magic));
        if (magic != 0x62657274)
        {
            fprintf(stderr, "%s: invalid model file '%s' (bad magic)\n", __func__, fname);
            return nullptr;
        }
    }

    bert_ctx * new_bert = new bert_ctx;
    bert_model & model = new_bert->model;
    bert_vocab & vocab = new_bert->vocab;

    // load hparams
    {
        auto &hparams = model.hparams;

        fin.read((char *)&hparams.n_vocab, sizeof(hparams.n_vocab));
        fin.read((char *)&hparams.n_max_tokens, sizeof(hparams.n_max_tokens));
        fin.read((char *)&hparams.n_embd, sizeof(hparams.n_embd));
        fin.read((char *)&hparams.n_intermediate, sizeof(hparams.n_intermediate));
        fin.read((char *)&hparams.n_head, sizeof(hparams.n_head));
        fin.read((char *)&hparams.n_layer, sizeof(hparams.n_layer));
        fin.read((char *)&hparams.f16, sizeof(hparams.f16));

#if defined(DEBUG_BERT)
        printf("%s: n_vocab = %d\n", __func__, hparams.n_vocab);
        printf("%s: n_max_tokens   = %d\n", __func__, hparams.n_max_tokens);
        printf("%s: n_embd  = %d\n", __func__, hparams.n_embd);
        printf("%s: n_intermediate  = %d\n", __func__, hparams.n_intermediate);
        printf("%s: n_head  = %d\n", __func__, hparams.n_head);
        printf("%s: n_layer = %d\n", __func__, hparams.n_layer);
        printf("%s: f16     = %d\n", __func__, hparams.f16);
#endif
    }

    // load vocab
    {
        int32_t n_vocab = model.hparams.n_vocab;

        std::string word;
        for (int i = 0; i < n_vocab; i++)
        {
            uint32_t len;
            fin.read((char *)&len, sizeof(len));

            word.resize(len);
            fin.read((char *)word.data(), len);

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

    // for the big tensors, we have the option to store the data in 16-bit floats or quantized
    // in order to save memory and also to speed up the computation
    ggml_type wtype = GGML_TYPE_COUNT;
    switch (model.hparams.f16)
    {
    case 0:
        wtype = GGML_TYPE_F32;
        break;
    case 1:
        wtype = GGML_TYPE_F16;
        break;
    case 2:
        wtype = GGML_TYPE_Q4_0;
        break;
    case 3:
        wtype = GGML_TYPE_Q4_1;
        break;
    default:
    {
        fprintf(stderr, "%s: invalid model file '%s' (bad f16 value %d)\n",
                __func__, fname, model.hparams.f16);
        bert_free(new_bert);
        return nullptr;
    }
    }

    auto &ctx = model.ctx;

    size_t model_mem_req = 0;

    {
        const auto &hparams = model.hparams;

        const int n_embd = hparams.n_embd;
        const int n_layer = hparams.n_layer;
        const int n_max_tokens = hparams.n_max_tokens;
        const int n_intermediate = hparams.n_intermediate;
        const int n_vocab = hparams.n_vocab;

        // Calculate size requirements

        model_mem_req += n_embd * n_vocab * ggml_type_sizef(wtype); // word_embeddings
        model_mem_req += n_embd * 2 * ggml_type_sizef(wtype); // token_type_embeddings
        model_mem_req += n_embd * n_max_tokens * ggml_type_sizef(wtype); // position_embeddings

        model_mem_req += 2 * n_embd * ggml_type_sizef(GGML_TYPE_F32); // ln_e_*

        model_mem_req += 4 * n_layer * (n_embd * ggml_type_sizef(GGML_TYPE_F32)); // ln_*

        model_mem_req += 4 * n_layer * (n_embd * n_embd * ggml_type_sizef(wtype)); // kqvo weights
        model_mem_req += 4 * n_layer * (n_embd * ggml_type_sizef(GGML_TYPE_F32)); // kqvo bias

        model_mem_req += 2 * n_layer * (n_embd * n_intermediate * ggml_type_sizef(wtype)); // ff_*_w
        model_mem_req += n_layer * (n_intermediate * ggml_type_sizef(GGML_TYPE_F32)); // ff_i_b
        model_mem_req += n_layer * (n_embd * ggml_type_sizef(GGML_TYPE_F32)); // ff_o_b

        model_mem_req += (5 + 16 * n_layer) * 256; // object overhead

#if defined(DEBUG_BERT)
        printf("%s: ggml ctx size = %6.2f MB\n", __func__, model_mem_req / (1024.0 * 1024.0));
#endif
    }

    // create the ggml context
    {
        struct ggml_init_params params = {
            .mem_size = model_mem_req,
            .mem_buffer = NULL,
            .no_alloc = false,
        };

        model.ctx = ggml_init(params);
        if (!model.ctx)
        {
            fprintf(stderr, "%s: ggml_init() failed\n", __func__);
            bert_free(new_bert);
            return nullptr;
        }
    }

    // prepare memory for the weights
    {
        const auto &hparams = model.hparams;

        const int n_embd = hparams.n_embd;
        const int n_layer = hparams.n_layer;
        const int n_intermediate = hparams.n_intermediate;
        const int n_max_tokens = hparams.n_max_tokens;
        const int n_vocab = hparams.n_vocab;

        model.layers.resize(n_layer);

        model.word_embeddings = ggml_new_tensor_2d(ctx, wtype, n_embd, n_vocab);
        model.token_type_embeddings = ggml_new_tensor_2d(ctx, wtype, n_embd, 2);
        model.position_embeddings = ggml_new_tensor_2d(ctx, wtype, n_embd, n_max_tokens);

        model.ln_e_w = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embd);
        model.ln_e_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embd);

        // map by name
        model.tensors["embeddings.word_embeddings.weight"] = model.word_embeddings;
        model.tensors["embeddings.token_type_embeddings.weight"] = model.token_type_embeddings;
        model.tensors["embeddings.position_embeddings.weight"] = model.position_embeddings;

        model.tensors["embeddings.LayerNorm.weight"] = model.ln_e_w;
        model.tensors["embeddings.LayerNorm.bias"] = model.ln_e_b;

        for (int i = 0; i < n_layer; ++i)
        {
            auto &layer = model.layers[i];

            layer.ln_att_w = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embd);
            layer.ln_att_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embd);
            layer.ln_out_w = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embd);
            layer.ln_out_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embd);

            layer.q_w = ggml_new_tensor_2d(ctx, wtype, n_embd, n_embd);
            layer.q_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embd);
            layer.k_w = ggml_new_tensor_2d(ctx, wtype, n_embd, n_embd);
            layer.k_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embd);
            layer.v_w = ggml_new_tensor_2d(ctx, wtype, n_embd, n_embd);
            layer.v_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embd);
            layer.o_w = ggml_new_tensor_2d(ctx, wtype, n_embd, n_embd);
            layer.o_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embd);

            layer.ff_i_w = ggml_new_tensor_2d(ctx, wtype, n_embd, n_intermediate);
            layer.ff_i_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_intermediate);

            layer.ff_o_w = ggml_new_tensor_2d(ctx, wtype, n_intermediate, n_embd);
            layer.ff_o_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_embd);

            // map by name

            model.tensors["encoder.layer." + std::to_string(i) + ".attention.self.query.weight"] = layer.q_w;
            model.tensors["encoder.layer." + std::to_string(i) + ".attention.self.query.bias"] = layer.q_b;
            model.tensors["encoder.layer." + std::to_string(i) + ".attention.self.key.weight"] = layer.k_w;
            model.tensors["encoder.layer." + std::to_string(i) + ".attention.self.key.bias"] = layer.k_b;
            model.tensors["encoder.layer." + std::to_string(i) + ".attention.self.value.weight"] = layer.v_w;
            model.tensors["encoder.layer." + std::to_string(i) + ".attention.self.value.bias"] = layer.v_b;
            model.tensors["encoder.layer." + std::to_string(i) + ".attention.output.LayerNorm.weight"] = layer.ln_att_w;
            model.tensors["encoder.layer." + std::to_string(i) + ".attention.output.LayerNorm.bias"] = layer.ln_att_b;
            model.tensors["encoder.layer." + std::to_string(i) + ".attention.output.dense.weight"] = layer.o_w;
            model.tensors["encoder.layer." + std::to_string(i) + ".attention.output.dense.bias"] = layer.o_b;

            model.tensors["encoder.layer." + std::to_string(i) + ".intermediate.dense.weight"] = layer.ff_i_w;
            model.tensors["encoder.layer." + std::to_string(i) + ".intermediate.dense.bias"] = layer.ff_i_b;

            model.tensors["encoder.layer." + std::to_string(i) + ".output.LayerNorm.weight"] = layer.ln_out_w;
            model.tensors["encoder.layer." + std::to_string(i) + ".output.LayerNorm.bias"] = layer.ln_out_b;
            model.tensors["encoder.layer." + std::to_string(i) + ".output.dense.weight"] = layer.ff_o_w;
            model.tensors["encoder.layer." + std::to_string(i) + ".output.dense.bias"] = layer.ff_o_b;
        }
    }

    // load weights
    {
        int n_tensors = 0;
#if defined(DEBUG_BERT)
        size_t total_size = 0;
#endif

#if defined(DEBUG_BERT)
        printf("%s: ", __func__);
#endif

        while (true)
        {
            int32_t n_dims;
            int32_t length;
            int32_t ftype;

            fin.read(reinterpret_cast<char *>(&n_dims), sizeof(n_dims));
            fin.read(reinterpret_cast<char *>(&length), sizeof(length));
            fin.read(reinterpret_cast<char *>(&ftype), sizeof(ftype));

            if (fin.eof())
            {
                break;
            }

            int64_t nelements = 1;
            int64_t ne[2] = {1, 1};
            for (int i = 0; i < n_dims; ++i)
            {
                int32_t ne_cur;
                fin.read(reinterpret_cast<char *>(&ne_cur), sizeof(ne_cur));
                ne[i] = ne_cur;
                nelements *= ne[i];
            }

            std::string name(length, 0);
            fin.read(&name[0], length);

            if (model.tensors.find(name.data()) == model.tensors.end())
            {
                fprintf(stderr, "%s: unknown tensor '%s' in model file\n", __func__, name.data());
                bert_free(new_bert);
                return nullptr;
            }

            auto tensor = model.tensors[name.data()];
            if (ggml_nelements(tensor) != nelements)
            {
                fprintf(stderr, "%s: tensor '%s' has wrong size in model file\n", __func__, name.data());
                bert_free(new_bert);
                return nullptr;
            }

            if (tensor->ne[0] != ne[0] || tensor->ne[1] != ne[1])
            {
                fprintf(stderr, "%s: tensor '%s' has wrong shape in model file: got [%ld, %ld], expected [%ld, %ld]\n",
                        __func__, name.data(), tensor->ne[0], tensor->ne[1], ne[0], ne[1]);
                bert_free(new_bert);
                return nullptr;
            }

#if defined(DEBUG_BERT)
            static const char *ftype_str[] = {
                "f32",
                "f16",
                "q4_0",
                "q4_1",
            };
            printf("%24s - [%5ld, %5ld], type = %6s, %6.2f MB, %9zu bytes\n", name.data(), ne[0], ne[1], ftype_str[ftype], ggml_nbytes(tensor) / 1024.0 / 1024.0, ggml_nbytes(tensor));
#endif

            size_t bpe = 0;

            switch (ftype)
            {
            case 0:
                bpe = ggml_type_size(GGML_TYPE_F32);
                break;
            case 1:
                bpe = ggml_type_size(GGML_TYPE_F16);
                break;
            case 2:
                bpe = ggml_type_size(GGML_TYPE_Q4_0);
                assert(ne[0] % 64 == 0);
                break;
            case 3:
                bpe = ggml_type_size(GGML_TYPE_Q4_1);
                assert(ne[0] % 64 == 0);
                break;
            default:
            {
                fprintf(stderr, "%s: unknown ftype %d in model file\n", __func__, ftype);
                bert_free(new_bert);
                return nullptr;
            }
            };

            if ((nelements * bpe) / ggml_blck_size(tensor->type) != ggml_nbytes(tensor))
            {
                fprintf(stderr, "%s: tensor '%s' has wrong size in model file: got %zu, expected %lu\n",
                        __func__, name.data(), ggml_nbytes(tensor), nelements * bpe);
                bert_free(new_bert);
                return nullptr;
            }

            fin.read(reinterpret_cast<char *>(tensor->data), ggml_nbytes(tensor));

#if defined(DEBUG_BERT)
            // printf("%42s - [%5d, %5d], type = %6s, %6.2f MB\n", name.data(), ne[0], ne[1], ftype == 0 ? "float" : "f16", ggml_nbytes(tensor)/1024.0/1024.0);
            total_size += ggml_nbytes(tensor);
#endif

            if (++n_tensors % 8 == 0)
            {
#if defined(DEBUG_BERT)
                printf(".");
                fflush(stdout);
#endif
            }
        }

#if defined(DEBUG_BERT)
        printf(" done\n");
        printf("%s: model size = %8.2f MB / num tensors = %d\n", __func__, total_size / 1024.0 / 1024.0, n_tensors);
#endif
    }

    fin.close();

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

bool Bert::loadModel(const std::string &modelPath)
{
    d_ptr->ctx = bert_load_from_file(modelPath.c_str());
    d_ptr->n_threads = std::min(4, (int32_t) std::thread::hardware_concurrency());
    d_ptr->modelLoaded = d_ptr->ctx != nullptr;
    fflush(stdout);
    return true;
}

bool Bert::isModelLoaded() const
{
    return d_ptr->modelLoaded;
}

size_t Bert::requiredMem(const std::string &/*modelPath*/)
{
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
    if (magic != 0x62657274) {
         return false;
    }
    return true;
}

DLL_EXPORT LLModel *construct() {
    return new Bert;
}
}