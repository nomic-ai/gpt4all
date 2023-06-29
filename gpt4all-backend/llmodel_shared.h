#pragma once
#include <cstdint>
#include <cstddef>
#include <ggml.h>

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
