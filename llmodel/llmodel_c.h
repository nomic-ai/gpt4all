#ifndef LLMODEL_C_H
#define LLMODEL_C_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opaque pointers to the underlying C++ classes.
 */
typedef void *LLMODEL_C;
typedef void *GPTJ_C;
typedef void *LLAMA_C;

/**
 * PromptContext_C structure for holding the prompt context.
 */
typedef struct {
    float *logits;          // logits of current context
    int32_t *tokens;        // current tokens in the context window
    int32_t n_past;         // number of tokens in past conversation
    int32_t n_ctx;          // number of tokens possible in context window
    int32_t n_predict;      // number of tokens to predict
    int32_t top_k;          // top k logits to sample from
    float top_p;            // nucleus sampling probability threshold
    float temp;             // temperature to adjust model's output distribution
    int32_t n_batch;        // number of predictions to generate in parallel
    float repeat_penalty;   // penalty factor for repeated tokens
    int32_t repeat_last_n;  // last n tokens to penalize
    float contextErase;     // percent of context to erase if we exceed the context window
} PromptContext_C;

/**
 * Callback types for response and recalculation.
 */
typedef bool (*ResponseCallback)(int32_t, const char *);
typedef bool (*RecalculateCallback)(bool);

/**
 * Create a GPTJ instance.
 * @return A pointer to the GPTJ instance.
 */
GPTJ_C GPTJ_create();

/**
 * Destroy a GPTJ instance.
 * @param gptj A pointer to the GPTJ instance.
 */
void GPTJ_destroy(GPTJ_C gptj);

/**
 * Create a LLAMA instance.
 * @return A pointer to the LLAMA instance.
 */
LLAMA_C LLAMA_create();

/**
 * Destroy a LLAMA instance.
 * @param llama A pointer to the LLAMA instance.
 */
void LLAMA_destroy(LLAMA_C llama);

/**
 * Load a model from a file.
 * @param model A pointer to the LLMODEL_C instance.
 * @param modelPath A string representing the path to the model file.
 * @return true if the model was loaded successfully, false otherwise.
 */
bool LLMODEL_loadModel(LLMODEL_C model, const char *modelPath);

/**
 * Load a model from an input stream.
 * @param model A pointer to the LLMODEL_C instance.
 * @param modelPath A string representing the path to the model file.
 * @param fin A pointer to the input stream.
 * @return true if the model was loaded successfully, false otherwise.
 */
bool LLMODEL_loadModelStream(LLMODEL_C model, const char *modelPath, void *fin);

/**
 * Check if a model is loaded.
 * @param model A pointer to the LLMODEL_C instance.
 * @return true if the model is loaded, false otherwise.
 */
bool LLMODEL_isModelLoaded(LLMODEL_C model);

/**
 * Generate a response using the model.
 * @param model A pointer to the LLMODEL_C instance.
 * @param prompt A string representing the input prompt.
 * @param response A callback function for handling the generated response.
 * @param recalculate A callback function for handling recalculation requests.
 * @param ctx A pointer to the PromptContext_C structure.
 */
void LLMODEL_prompt(LLMODEL_C model, const char *prompt,
                    ResponseCallback response,
                    RecalculateCallback recalculate,
                    PromptContext_C *ctx);

/**
 * Set the number of threads to be used by the model.
 * @param model A pointer to the LLMODEL_C instance.
 * @param n_threads The number of threads to be used.
 */
void LLMODEL_setThreadCount(LLMODEL_C model, int32_t n_threads);

/**
 * Get the number of threads currently being used by the model.
 * @param model A pointer to the LLMODEL_C instance.
 * @return The number of threads currently being used.
 */
int32_t LLMODEL_threadCount(LLMODEL_C model);

#ifdef __cplusplus
}
#endif

#endif // LLMODEL_C_H
