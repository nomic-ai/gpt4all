#include "llmodel.h"

#include <cassert>
#include <iostream>

void LLModel::recalculateContext(PromptContext &promptCtx, std::function<bool(bool)> recalculate) {
    size_t i = 0;
    promptCtx.n_past = 0;
    while (i < promptCtx.tokens.size()) {
        size_t batch_end = std::min(i + promptCtx.n_batch, promptCtx.tokens.size());
        std::vector<int32_t> batch(promptCtx.tokens.begin() + i, promptCtx.tokens.begin() + batch_end);
        assert(promptCtx.n_past + int32_t(batch.size()) <= promptCtx.n_ctx);
        if (!evalTokens(promptCtx, batch)) {
            std::cerr << "LLModel ERROR: Failed to process prompt\n";
            goto stop_generating;
        }
        promptCtx.n_past += batch.size();
        if (!recalculate(true))
            goto stop_generating;
        i = batch_end;
    }
    assert(promptCtx.n_past == int32_t(promptCtx.tokens.size()));

stop_generating:
    recalculate(false);
}
