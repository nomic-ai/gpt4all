#include "../../gpt4all-backend/llmodel_c.h"
#include "../../gpt4all-backend/llmodel.h"
#include "../../gpt4all-backend/llama.cpp/llama.h"
#include "../../gpt4all-backend/llmodel_c.cpp"
#include "../../gpt4all-backend/mpt.h"
#include "../../gpt4all-backend/mpt.cpp"

#include "../../gpt4all-backend/llamamodel.h"
#include "../../gpt4all-backend/gptj.h"
#include "binding.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <unistd.h>

void* load_mpt_model(const char *fname, int n_threads) {
    // load the model
    auto gptj = llmodel_mpt_create();

    llmodel_setThreadCount(gptj,  n_threads);
    if (!llmodel_loadModel(gptj, fname)) {
        return nullptr;
    }

    return gptj;
}

void* load_llama_model(const char *fname, int n_threads) {
    // load the model
    auto gptj = llmodel_llama_create();

    llmodel_setThreadCount(gptj,  n_threads);
    if (!llmodel_loadModel(gptj, fname)) {
        return nullptr;
    }

    return gptj;
}

void* load_gptj_model(const char *fname, int n_threads) {
    // load the model
    auto gptj = llmodel_gptj_create();

    llmodel_setThreadCount(gptj,  n_threads);
    if (!llmodel_loadModel(gptj, fname)) {
        return nullptr;
    }

    return gptj;
}

std::string res = "";
void * mm;

void gptj_model_prompt( const char *prompt, void *m, char* result, int repeat_last_n, float repeat_penalty, int n_ctx, int tokens, int top_k,
                            float top_p, float temp, int n_batch,float ctx_erase)
{
    llmodel_model* model = (llmodel_model*) m;

   // std::string res = "";
 
    auto lambda_prompt = [](int token_id, const char *promptchars)  {
	        return true;
    };

    mm=model;
    res="";

    auto lambda_response = [](int token_id, const char *responsechars) {
        res.append((char*)responsechars);
        return !!getTokenCallback(mm, (char*)responsechars);
	};
	
	auto lambda_recalculate = [](bool is_recalculating) {
	        // You can handle recalculation requests here if needed
	    return is_recalculating;
	};

    llmodel_prompt_context* prompt_context = new llmodel_prompt_context{
        .logits = NULL,
        .logits_size = 0,
        .tokens = NULL,
        .tokens_size = 0,
        .n_past = 0,
        .n_ctx = 1024,
        .n_predict = 50,
        .top_k = 10,
        .top_p = 0.9,
        .temp = 1.0,
        .n_batch = 1,
        .repeat_penalty = 1.2,
        .repeat_last_n = 10,
        .context_erase = 0.5
    };

    prompt_context->n_predict = tokens;
    prompt_context->repeat_last_n = repeat_last_n;
    prompt_context->repeat_penalty = repeat_penalty;
    prompt_context->n_ctx = n_ctx;
    prompt_context->top_k = top_k;
    prompt_context->context_erase = ctx_erase;
    prompt_context->top_p = top_p;
    prompt_context->temp = temp;
    prompt_context->n_batch = n_batch;    

    llmodel_prompt(model, prompt,
                        lambda_prompt,
                        lambda_response,
                    lambda_recalculate,
                    prompt_context );

    strcpy(result, res.c_str()); 

    free(prompt_context);
}

void gptj_free_model(void *state_ptr) {
    llmodel_model* ctx = (llmodel_model*) state_ptr;
    llmodel_llama_destroy(ctx);
}

