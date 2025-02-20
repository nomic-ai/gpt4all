#include <QLatin1StringView>

import fmt;
import gpt4all.backend.main;
import gpt4all.test.config;

using gpt4all::backend::LLMProvider;


int main()
{
    LLMProvider provider(OLLAMA_URL);
    fmt::print("Server version: {}", provider.getVersion());
}
