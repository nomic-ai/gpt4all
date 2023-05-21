// Various helper functions and utilities

#pragma once

#include <string>
#include <map>
#include <vector>
#include <random>
#include <thread>
#include "tokenizer/bpe.h"

//
// CLI argument parsing
//

struct gpt_params {
    int32_t seed      = -1; // RNG seed
    int32_t n_threads = std::min(4, (int32_t) std::thread::hardware_concurrency());
    int32_t n_predict = 200; // new tokens to predict

    // sampling parameters
    int32_t top_k = 40;
    float   top_p = 0.9f;
    float   temp  = 0.9f;

    int32_t n_batch = 8; // batch size for prompt processing

    std::string model = "models/gpt-2-117M/ggml-model.bin"; // model path
    std::string prompt;
};

bool gpt_params_parse(int argc, char ** argv, gpt_params & params);

void gpt_print_usage(int argc, char ** argv, const gpt_params & params);

std::string gpt_random_prompt(std::mt19937 & rng);

//
// Vocab utils
//

struct gpt_vocab {
    using id    = int32_t;
    using token = std::string;

    std::map<token, id> token_to_id;
    std::map<id, token> id_to_token;
    std::vector<std::string> special_tokens;

    void add_special_token(const std::string &token) {
        special_tokens.push_back(token);
    }
};

// sample next token given probabilities for each embedding
//
//   - consider only the top K tokens
//   - from them, consider only the top tokens with cumulative probability > P
//
// TODO: not sure if this implementation is correct
//
gpt_vocab::id gpt_sample_top_k_top_p(
        const gpt_vocab & vocab,
        const size_t actualVocabSize,
        const int32_t * last_n_tokens_data,
        int   last_n_tokens_size,
        const std::vector<float> logits,
        int    top_k,
        double top_p,
        double temp,
        float repeat_penalty,
        std::mt19937 & rng);

enum TokenizerType {
    MPT, MPT_CHAT, GPTJ 
};

void get_bpecpp_tokenizer(const TokenizerType ttype, std::unique_ptr<bpecpp::BPE>& bpe, std::unique_ptr<bpecpp::AdditionalVocabAdapter>& av);
