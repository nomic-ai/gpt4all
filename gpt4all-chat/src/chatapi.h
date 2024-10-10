#ifndef CHATAPI_H
#define CHATAPI_H

#include <gpt4all-backend/llmodel.h>

#include <QByteArray> // IWYU pragma: keep
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

class QNetworkAccessManager;

class ChatAPI;
class ChatAPIWorker : public QObject {
    Q_OBJECT
public:
    ChatAPIWorker(ChatAPI *chatAPI)
        : QObject(nullptr)
        , m_ctx(nullptr)
        , m_networkManager(nullptr)
        , m_chat(chatAPI) {}
    virtual ~ChatAPIWorker() {}

    QString currentResponse() const { return m_currentResponse; }

    void request(const QString &apiKey,
                 LLModel::PromptContext *promptCtx,
                 const QByteArray &array);

Q_SIGNALS:
    void finished();

private Q_SLOTS:
    void handleFinished();
    void handleReadyRead();
    void handleErrorOccurred(QNetworkReply::NetworkError code);

private:
    ChatAPI *m_chat;
    LLModel::PromptContext *m_ctx;
    QNetworkAccessManager *m_networkManager;
    QString m_currentResponse;
};

class ChatAPI : public QObject, public LLModel {
    Q_OBJECT
public:
    ChatAPI();
    virtual ~ChatAPI();

    bool supportsEmbedding() const override { return false; }
    bool supportsCompletion() const override { return true; }
    bool loadModel(const std::string &modelPath, int n_ctx, int ngl) override;
    bool isModelLoaded() const override;
    size_t requiredMem(const std::string &modelPath, int n_ctx, int ngl) override;

    // All three of the state virtual functions are handled custom inside of chatllm save/restore
    size_t stateSize() const override
    { throwNotImplemented(); }
    size_t saveState(std::span<uint8_t> stateOut, std::vector<Token> &inputTokensOut) const override
    { Q_UNUSED(stateOut); Q_UNUSED(inputTokensOut); throwNotImplemented(); }
    size_t restoreState(std::span<const uint8_t> state, std::span<const Token> inputTokens) override
    { Q_UNUSED(state); Q_UNUSED(inputTokens); throwNotImplemented(); }

    void prompt(const std::string &prompt,
                const std::string &promptTemplate,
                std::function<bool(int32_t)> promptCallback,
                std::function<bool(int32_t, const std::string&)> responseCallback,
                bool allowContextShift,
                PromptContext &ctx,
                bool special,
                std::optional<std::string_view> fakeReply) override;

    void setThreadCount(int32_t n_threads) override;
    int32_t threadCount() const override;

    void setModelName(const QString &modelName) { m_modelName = modelName; }
    void setAPIKey(const QString &apiKey) { m_apiKey = apiKey; }
    void setRequestURL(const QString &requestURL) { m_requestURL = requestURL; }
    QString url() const { return m_requestURL; }

    QList<QString> context() const { return m_context; }
    void setContext(const QList<QString> &context) { m_context = context; }

    bool callResponse(int32_t token, const std::string &string);

    [[noreturn]]
    int32_t contextLength() const override
    { throwNotImplemented(); }

Q_SIGNALS:
    void request(const QString &apiKey,
                 LLModel::PromptContext *ctx,
                 const QByteArray &array);

protected:
    // We have to implement these as they are pure virtual in base class, but we don't actually use
    // them as they are only called from the default implementation of 'prompt' which we override and
    // completely replace

    [[noreturn]]
    static void throwNotImplemented() { throw std::logic_error("not implemented"); }

    [[noreturn]]
    std::vector<Token> tokenize(std::string_view str, bool special) override
    { Q_UNUSED(str); Q_UNUSED(special); throwNotImplemented(); }

    [[noreturn]]
    bool isSpecialToken(Token id) const override
    { Q_UNUSED(id); throwNotImplemented(); }

    [[noreturn]]
    std::string tokenToString(Token id) const override
    { Q_UNUSED(id); throwNotImplemented(); }

    [[noreturn]]
    void initSampler(PromptContext &ctx) override
    { Q_UNUSED(ctx); throwNotImplemented(); }

    [[noreturn]]
    Token sampleToken() const override
    { throwNotImplemented(); }

    [[noreturn]]
    bool evalTokens(PromptContext &ctx, std::span<const Token> tokens) const override
    { Q_UNUSED(ctx); Q_UNUSED(tokens); throwNotImplemented(); }

    [[noreturn]]
    void shiftContext(PromptContext &promptCtx) override
    { Q_UNUSED(promptCtx); throwNotImplemented(); }

    [[noreturn]]
    int32_t inputLength() const override
    { throwNotImplemented(); }

    [[noreturn]]
    void setTokenizeInputPosition(int32_t pos) override
    { Q_UNUSED(pos); throwNotImplemented(); }

    [[noreturn]]
    void setModelInputPosition(int32_t pos) override
    { Q_UNUSED(pos); throwNotImplemented(); }

    [[noreturn]]
    void appendInputToken(PromptContext &ctx, Token tok) override
    { Q_UNUSED(ctx); Q_UNUSED(tok); throwNotImplemented(); }

    [[noreturn]]
    const std::vector<Token> &endTokens() const override
    { throwNotImplemented(); }

    [[noreturn]]
    bool shouldAddBOS() const override
    { throwNotImplemented(); }

    [[noreturn]]
    std::span<const Token> inputTokens() const override
    { throwNotImplemented(); }

private:
    std::function<bool(int32_t, const std::string&)> m_responseCallback;
    QString m_modelName;
    QString m_apiKey;
    QString m_requestURL;
    QList<QString> m_context;
    QStringList m_queuedPrompts;
};

#endif // CHATAPI_H
