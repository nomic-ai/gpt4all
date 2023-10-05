#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <ggml.h>

#if defined(GGML_USE_KOMPUTE)
#include "ggml-vulkan.h"
struct llm_buffer {
    uint8_t * addr = NULL;
    size_t size = 0;
    ggml_vk_memory memory;

    llm_buffer() = default;

    void resize(size_t size) {
        free();

        if (!ggml_vk_has_device()) {
            this->addr = new uint8_t[size];
            this->size = size;
        } else {
            this->memory = ggml_vk_allocate(size);
            this->addr = (uint8_t*)memory.data;
            this->size = size;
        }
    }

    void free() {
        if (!memory.primaryMemory) {
            delete[] addr;
        } else if (memory.data) {
            ggml_vk_free_memory(memory);
        }
        this->addr = NULL;
        this->size = 0;
    }

    ~llm_buffer() {
        free();
    }

    // disable copy and move
    llm_buffer(const llm_buffer&) = delete;
    llm_buffer(llm_buffer&&) = delete;
    llm_buffer& operator=(const llm_buffer&) = delete;
    llm_buffer& operator=(llm_buffer&&) = delete;
};
#else
struct llm_buffer {
    uint8_t * addr = NULL;
    size_t size = 0;

    void resize(size_t size) {
        delete[] addr;
        addr = new uint8_t[size];
        this->size = size;
    }

    ~llm_buffer() {
        delete[] addr;
    }
};
#endif

struct llm_kv_cache {
    struct ggml_tensor * k;
    struct ggml_tensor * v;

    struct ggml_context * ctx = NULL;

    llm_buffer buf;

    int n; // number of tokens currently in the cache

    ~llm_kv_cache() {
        if (ctx) {
            ggml_free(ctx);
        }
    }
};

inline void ggml_graph_compute_g4a(llm_buffer& buf, ggml_cgraph * graph, int n_threads) {
    struct ggml_cplan plan = ggml_graph_plan(graph, n_threads);
    if (plan.work_size > 0) {
        buf.resize(plan.work_size);
        plan.work_data = buf.addr;
    }
    ggml_graph_compute(graph, &plan);
}
