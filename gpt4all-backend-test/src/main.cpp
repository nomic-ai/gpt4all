import fmt;
import gpt4all.backend.main;
import gpt4all.test.config;

#include <QLatin1StringView>


int main()
{
    LLMProvider provider { QLatin1StringView(OLLAMA_URL) };
    fmt::print("Server version: {}", provider.getVersion());
}
