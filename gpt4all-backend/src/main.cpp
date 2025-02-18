module;

#include <string>

#include <QLatin1StringView>

#include <ollama.hpp>

module gpt4all.backend.main;


LLMProvider::LLMProvider(QLatin1StringView serverUrl)
    : m_serverUrl(serverUrl.data(), serverUrl.size())
    , m_ollama(std::make_unique<Ollama>(m_serverUrl))
    {}

LLMProvider::~LLMProvider() = default;

void LLMProvider::setServerUrl(QLatin1StringView serverUrl)
{
    m_serverUrl.assign(serverUrl.data(), serverUrl.size());
    m_ollama->setServerURL(m_serverUrl);
}

QByteArray LLMProvider::getVersion()
{
    return QByteArray(m_ollama->get_version());
}
