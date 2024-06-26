#pragma once

#include <ggml.h>

#include <cstddef>
#include <cstdint>
#include <vector>

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

inline void ggml_graph_compute_g4a(llm_buffer& buf, ggml_cgraph * graph, int n_threads)
{
    struct ggml_cplan plan = ggml_graph_plan(graph, n_threads);
    if (plan.work_size > 0) {
        buf.resize(plan.work_size);
        plan.work_data = buf.addr;
    }
    ggml_graph_compute(graph, &plan);
}
