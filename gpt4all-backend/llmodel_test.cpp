#include "llmodel_c.h"

#include <string>
#include <string_view>
#include <cstring>
#include <cstdint>
#include <gtest/gtest.h>

namespace {
llmodel_model model;
uint8_t *savedState;

TEST(CAPITest, ModelLoad) {
    llmodel_error ec;
    llmodel_set_implementation_search_path("..");
    model = llmodel_model_create2(LLMODEL_TEST_MODEL, "auto", &ec);
    EXPECT_NE(model, reinterpret_cast<void*>(NULL)) << "Implementation creation failed: " << ec.message;
    EXPECT_FALSE(llmodel_isModelLoaded(model));
    EXPECT_TRUE(llmodel_loadModel(model, LLMODEL_TEST_MODEL)) << "Model load failed";
    EXPECT_TRUE(llmodel_isModelLoaded(model));
}

TEST(CAPITest, Prompt) {
    static std::string response;

    auto prompt_cb = [] (int32_t) {
        return true;
    };
    auto response_cb = [] (int32_t, const char *token) {
        response += token;
        return true;
    };
    auto recalc_cb = [] (bool is_recalculating) {
        EXPECT_FALSE(is_recalculating);
        return true;
    };

    llmodel_prompt_context ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.top_k = 0;
    ctx.top_p = 1.0f;
    ctx.temp = 0.6;
    ctx.n_ctx = 2024;
    ctx.n_batch = 16;
    ctx.n_predict = 16;

    llmodel_prompt(model, "Did you know?", prompt_cb, response_cb, recalc_cb, &ctx);

    EXPECT_FALSE(response.empty()) << "Model didn't generate a response";
    EXPECT_EQ(response, " The average American eats 200 pounds of cheese per year.") << "Model didn't generate the expected response";
}

TEST(CAPITest, StateSave) {
    savedState = new uint8_t[llmodel_get_state_size(model)];
    EXPECT_GT(llmodel_save_state_data(model, savedState), 0);
}

TEST(CAPITest, StateRestore) {
    EXPECT_GT(llmodel_restore_state_data(model, savedState), 0);
    delete []savedState;
}

TEST(CAPITest, ModelUnload) {
    llmodel_model_destroy(model);
}
}
