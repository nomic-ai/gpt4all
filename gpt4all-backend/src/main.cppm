module;

#include <expected>

#include <QNetworkReply>
#include <QUrl>

#include <QCoro/QCoroTask>

class QString;
template <typename T> class QFuture;

export module gpt4all.backend.main;


export namespace gpt4all::backend {

template <typename T>
using DataOrNetErr = std::expected<T, QNetworkReply::NetworkError>;


class LLMProvider {
public:
    LLMProvider(QUrl baseUrl)
        : m_baseUrl(baseUrl)
        {}

    const QUrl &baseUrl() const { return m_baseUrl; }
    void getBaseUrl(QUrl value) { m_baseUrl = std::move(value); }

    /// Retrieve the Ollama version, e.g. "0.5.1"
    auto getVersion() const -> QCoro::Task<DataOrNetErr<QString>>;

private:
    QUrl m_baseUrl;
};

} // namespace gpt4all::backend
