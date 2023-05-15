#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

void* load_mpt_model(const char *fname, int n_threads);

void* load_llama_model(const char *fname, int n_threads);

void* load_gptj_model(const char *fname, int n_threads);

void gptj_model_prompt( const char *prompt, void *m, char* result, int repeat_last_n, float repeat_penalty, int n_ctx, int tokens, int top_k,
                            float top_p, float temp, int n_batch,float ctx_erase);

void gptj_free_model(void *state_ptr);

extern unsigned char getTokenCallback(void *, char *);

#ifdef __cplusplus
}
#endif