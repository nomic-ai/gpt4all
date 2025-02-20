import fmt;
import gpt4all.backend.main;
import gpt4all.test.config;

#include <QLatin1StringView>

using gpt4all::backend::LLMProvider;


int main()
{
    LLMProvider provider(OLLAMA_URL);
    fmt::print("Server version: {}", provider.getVersion());
}
