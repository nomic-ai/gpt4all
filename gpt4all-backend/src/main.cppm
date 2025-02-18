module;

#include <memory>
#include <string>

#include <QLatin1StringView>

export module gpt4all.backend.main;

class Ollama;


export class LLMProvider {
public:
    LLMProvider(QLatin1StringView serverUrl);
    ~LLMProvider();

    QLatin1StringView serverUrl() const { return QLatin1StringView(m_serverUrl); }
    void setServerUrl(QLatin1StringView serverUrl);

    /// Retrieve the Ollama version, e.g. "0.5.1"
    QByteArray getVersion();

private:
    std::string             m_serverUrl;
    std::unique_ptr<Ollama> m_ollama;
};
