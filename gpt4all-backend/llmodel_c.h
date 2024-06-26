#ifndef LLMODEL_C_H
#define LLMODEL_C_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __GNUC__
#define DEPRECATED __attribute__ ((deprecated))
#elif defined(_MSC_VER)
#define DEPRECATED __declspec(deprecated)
#else
#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#define DEPRECATED
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opaque pointer to the underlying model.
 */
typedef void *llmodel_model;

/**
 * llmodel_prompt_context structure for holding the prompt context.
 * NOTE: The implementation takes care of all the memory handling of the raw logits pointer and the
 * raw tokens pointer. Attempting to resize them or modify them in any way can lead to undefined
 * behavior.
 */
struct llmodel_prompt_context {
    float *logits;          // logits of current context
    size_t logits_size;     // the size of the raw logits vector
    int32_t *tokens;        // current tokens in the context window
    size_t tokens_size;     // the size of the raw tokens vector
    int32_t n_past;         // number of tokens in past conversation
    int32_t n_ctx;          // number of tokens possible in context window
    int32_t n_predict;      // number of tokens to predict
    int32_t top_k;          // top k logits to sample from
    float top_p;            // nucleus sampling probability threshold
    float min_p;            // Min P sampling
    float temp;             // temperature to adjust model's output distribution
    int32_t n_batch;        // number of predictions to generate in parallel
    float repeat_penalty;   // penalty factor for repeated tokens
    int32_t repeat_last_n;  // last n tokens to penalize
    float context_erase;    // percent of context to erase if we exceed the context window
};

struct llmodel_gpu_device {
    const char * backend;
    int index;
    int type; // same as VkPhysicalDeviceType
    size_t heapSize;
    const char * name;
    const char * vendor;
};

#ifndef __cplusplus
typedef struct llmodel_prompt_context llmodel_prompt_context;
typedef struct llmodel_gpu_device llmodel_gpu_device;
#endif

/**
 * Callback type for prompt processing.
 * @param token_id The token id of the prompt.
 * @return a bool indicating whether the model should keep processing.
 */
typedef bool (*llmodel_prompt_callback)(int32_t token_id);

/**
 * Callback type for response.
 * @param token_id The token id of the response.
 * @param response The response string. NOTE: a token_id of -1 indicates the string is an error string.
 * @return a bool indicating whether the model should keep generating.
 */
typedef bool (*llmodel_response_callback)(int32_t token_id, const char *response);

/**
 * Callback type for recalculation of context.
 * @param whether the model is recalculating the context.
 * @return a bool indicating whether the model should keep generating.
 */
typedef bool (*llmodel_recalculate_callback)(bool is_recalculating);

/**
 * Embedding cancellation callback for use with llmodel_embed.
 * @param batch_sizes The number of tokens in each batch that will be embedded.
 * @param n_batch The number of batches that will be embedded.
 * @param backend The backend that will be used for embedding. One of "cpu", "kompute", "cuda", or "metal".
 * @return True to cancel llmodel_embed, false to continue.
 */
typedef bool (*llmodel_emb_cancel_callback)(unsigned *batch_sizes, unsigned n_batch, const char *backend);

/**
 * Create a llmodel instance.
 * Recognises correct model type from file at model_path
 * @param model_path A string representing the path to the model file.
 * @return A pointer to the llmodel_model instance; NULL on error.
 */
DEPRECATED llmodel_model llmodel_model_create(const char *model_path);

/**
 * Create a llmodel instance.
 * Recognises correct model type from file at model_path
 * @param model_path A string representing the path to the model file; will only be used to detect model type.
 * @param backend A string representing the implementation to use. One of 'auto', 'cpu', 'metal', 'kompute', or 'cuda'.
 * @param error A pointer to a string; will only be set on error.
 * @return A pointer to the llmodel_model instance; NULL on error.
 */
llmodel_model llmodel_model_create2(const char *model_path, const char *backend, const char **error);

/**
 * Destroy a llmodel instance.
 * Recognises correct model type using type info
 * @param model a pointer to a llmodel_model instance.
 */
void llmodel_model_destroy(llmodel_model model);

/**
 * Estimate RAM requirement for a model file
 * @param model A pointer to the llmodel_model instance.
 * @param model_path A string representing the path to the model file.
 * @param n_ctx Maximum size of context window
 * @param ngl Number of GPU layers to use (Vulkan)
 * @return size greater than 0 if the model was parsed successfully, 0 if file could not be parsed.
 */
size_t llmodel_required_mem(llmodel_model model, const char *model_path, int n_ctx, int ngl);

/**
 * Load a model from a file.
 * @param model A pointer to the llmodel_model instance.
 * @param model_path A string representing the path to the model file.
 * @param n_ctx Maximum size of context window
 * @param ngl Number of GPU layers to use (Vulkan)
 * @return true if the model was loaded successfully, false otherwise.
 */
bool llmodel_loadModel(llmodel_model model, const char *model_path, int n_ctx, int ngl);

/**
 * Check if a model is loaded.
 * @param model A pointer to the llmodel_model instance.
 * @return true if the model is loaded, false otherwise.
 */
bool llmodel_isModelLoaded(llmodel_model model);

/**
 * Get the size of the internal state of the model.
 * NOTE: This state data is specific to the type of model you have created.
 * @param model A pointer to the llmodel_model instance.
 * @return the size in bytes of the internal state of the model
 */
uint64_t llmodel_get_state_size(llmodel_model model);

/**
 * Saves the internal state of the model to the specified destination address.
 * NOTE: This state data is specific to the type of model you have created.
 * @param model A pointer to the llmodel_model instance.
 * @param dest A pointer to the destination.
 * @return the number of bytes copied
 */
uint64_t llmodel_save_state_data(llmodel_model model, uint8_t *dest);

/**
 * Restores the internal state of the model using data from the specified address.
 * NOTE: This state data is specific to the type of model you have created.
 * @param model A pointer to the llmodel_model instance.
 * @param src A pointer to the src.
 * @return the number of bytes read
 */
uint64_t llmodel_restore_state_data(llmodel_model model, const uint8_t *src);

/**
 * Generate a response using the model.
 * @param model A pointer to the llmodel_model instance.
 * @param prompt A string representing the input prompt.
 * @param prompt_template A string representing the input prompt template.
 * @param prompt_callback A callback function for handling the processing of prompt.
 * @param response_callback A callback function for handling the generated response.
 * @param recalculate_callback A callback function for handling recalculation requests.
 * @param special True if special tokens in the prompt should be processed, false otherwise.
 * @param fake_reply A string to insert into context as the model's reply, or NULL to generate one.
 * @param ctx A pointer to the llmodel_prompt_context structure.
 */
void llmodel_prompt(llmodel_model model, const char *prompt,
                    const char *prompt_template,
                    llmodel_prompt_callback prompt_callback,
                    llmodel_response_callback response_callback,
                    llmodel_recalculate_callback recalculate_callback,
                    llmodel_prompt_context *ctx,
                    bool special,
                    const char *fake_reply);

/**
 * Generate an embedding using the model.
 * NOTE: If given NULL pointers for the model or text, or an empty text, a NULL pointer will be
 * returned. Bindings should signal an error when NULL is the return value.
 * @param model A pointer to the llmodel_model instance.
 * @param texts A pointer to a NULL-terminated array of strings representing the texts to generate an
 * embedding for.
 * @param embedding_size A pointer to a size_t type that will be set by the call indicating the length
 * of the returned floating point array.
 * @param prefix The model-specific prefix representing the embedding task, without the trailing colon. NULL for no
 * prefix.
 * @param dimensionality The embedding dimension, for use with Matryoshka-capable models. Set to -1 to for full-size.
 * @param token_count Return location for the number of prompt tokens processed, or NULL.
 * @param do_mean True to average multiple embeddings if the text is longer than the model can accept, False to
 * truncate.
 * @param atlas Try to be fully compatible with the Atlas API. Currently, this means texts longer than 8192 tokens with
 * long_text_mode="mean" will raise an error. Disabled by default.
 * @param cancel_cb Cancellation callback, or NULL. See the documentation of llmodel_emb_cancel_callback.
 * @param error Return location for a malloc()ed string that will be set on error, or NULL.
 * @return A pointer to an array of floating point values passed to the calling method which then will
 * be responsible for lifetime of this memory. NULL if an error occurred.
 */
float *llmodel_embed(llmodel_model model, const char **texts, size_t *embedding_size, const char *prefix,
                     int dimensionality, size_t *token_count, bool do_mean, bool atlas,
                     llmodel_emb_cancel_callback cancel_cb, const char **error);

/**
 * Frees the memory allocated by the llmodel_embedding function.
 * @param ptr A pointer to the embedding as returned from llmodel_embedding.
 */
void llmodel_free_embedding(float *ptr);

/**
 * Set the number of threads to be used by the model.
 * @param model A pointer to the llmodel_model instance.
 * @param n_threads The number of threads to be used.
 */
void llmodel_setThreadCount(llmodel_model model, int32_t n_threads);

/**
 * Get the number of threads currently being used by the model.
 * @param model A pointer to the llmodel_model instance.
 * @return The number of threads currently being used.
 */
int32_t llmodel_threadCount(llmodel_model model);

/**
 * Set llmodel implementation search path.
 * Default is "."
 * @param path The path to the llmodel implementation shared objects. This can be a single path or
 * a list of paths separated by ';' delimiter.
 */
void llmodel_set_implementation_search_path(const char *path);

/**
 * Get llmodel implementation search path.
 * @return The current search path; lifetime ends on next set llmodel_set_implementation_search_path() call.
 */
const char *llmodel_get_implementation_search_path();

/**
 * Get a list of available GPU devices given the memory required.
 * @param memoryRequired The minimum amount of VRAM, in bytes
 * @return A pointer to an array of llmodel_gpu_device's whose number is given by num_devices.
 */
struct llmodel_gpu_device* llmodel_available_gpu_devices(size_t memoryRequired, int* num_devices);

/**
 * Initializes a GPU device based on a specified string criterion.
 *
 * This function initializes a GPU device based on a string identifier provided. The function
 * allows initialization based on general device type ("gpu"), vendor name ("amd", "nvidia", "intel"),
 * or any specific device name.
 *
 * @param memoryRequired The amount of memory (in bytes) required by the application or task
 *                       that will utilize the GPU device.
 * @param device A string specifying the desired criterion for GPU device selection. It can be:
 *               - "gpu": To initialize the best available GPU.
 *               - "amd", "nvidia", or "intel": To initialize the best available GPU from that vendor.
 *               - A specific GPU device name: To initialize a GPU with that exact name.
 *
 * @return True if the GPU device is successfully initialized based on the provided string
 *         criterion. Returns false if the desired GPU device could not be initialized.
 */
bool llmodel_gpu_init_gpu_device_by_string(llmodel_model model, size_t memoryRequired, const char *device);

/**
 * Initializes a GPU device by specifying a valid gpu device pointer.
 * @param device A gpu device pointer.
 * @return True if the GPU device is successfully initialized, false otherwise.
 */
bool llmodel_gpu_init_gpu_device_by_struct(llmodel_model model, const llmodel_gpu_device *device);

/**
 * Initializes a GPU device by its index.
 * @param device An integer representing the index of the GPU device to be initialized.
 * @return True if the GPU device is successfully initialized, false otherwise.
 */
bool llmodel_gpu_init_gpu_device_by_int(llmodel_model model, int device);

/**
 * @return The name of the llama.cpp backend currently in use. One of "cpu", "kompute", or "metal".
 */
const char *llmodel_model_backend_name(llmodel_model model);

/**
 * @return The name of the GPU device currently in use, or NULL for backends other than Kompute.
 */
const char *llmodel_model_gpu_device_name(llmodel_model model);

#ifdef __cplusplus
}
#endif

#endif // LLMODEL_C_H
