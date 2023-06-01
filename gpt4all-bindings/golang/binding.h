#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

void* load_model(const char *fname, int n_threads);

void model_prompt( const char *prompt, void *m, char* result, int repeat_last_n, float repeat_penalty, int n_ctx, int tokens, int top_k,
                            float top_p, float temp, int n_batch,float ctx_erase);

void free_model(void *state_ptr);

extern unsigned char getTokenCallback(void *, char *);

#ifdef __cplusplus
}
#endif