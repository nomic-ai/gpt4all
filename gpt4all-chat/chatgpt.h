#ifndef CHATGPT_H
#define CHATGPT_H

#include <stdexcept>

#include <QObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QThread>
#include "../gpt4all-backend/llmodel.h"

class ChatGPT;
class ChatGPTWorker : public QObject {
    Q_OBJECT
public:
    ChatGPTWorker(ChatGPT *chatGPT)
        : QObject(nullptr)
        , m_ctx(nullptr)
        , m_networkManager(nullptr)
        , m_chat(chatGPT) {}
    virtual ~ChatGPTWorker() {}

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
    ChatGPT *m_chat;
    LLModel::PromptContext *m_ctx;
    QNetworkAccessManager *m_networkManager;
    QString m_currentResponse;
};

class ChatGPT : public QObject, public LLModel {
    Q_OBJECT
public:
    ChatGPT();
    virtual ~ChatGPT();

    bool supportsEmbedding() const override { return false; }
    bool supportsCompletion() const override { return true; }
    bool loadModel(const std::string &modelPath, int n_ctx, int ngl) override;
    bool isModelLoaded() const override;
    size_t requiredMem(const std::string &modelPath, int n_ctx, int ngl) override;
    size_t stateSize() const override;
    size_t saveState(uint8_t *dest) const override;
    size_t restoreState(const uint8_t *src) override;
    void prompt(const std::string &prompt,
        const std::string &promptTemplate,
        std::function<bool(int32_t)> promptCallback,
        std::function<bool(int32_t, const std::string&)> responseCallback,
        std::function<bool(bool)> recalculateCallback,
        PromptContext &ctx,
        bool special,
        std::string *fakeReply) override;

    void setThreadCount(int32_t n_threads) override;
    int32_t threadCount() const override;

    void setModelName(const QString &modelName) { m_modelName = modelName; }
    void setAPIKey(const QString &apiKey) { m_apiKey = apiKey; }

    QList<QString> context() const { return m_context; }
    void setContext(const QList<QString> &context) { m_context = context; }

    bool callResponse(int32_t token, const std::string &string);

Q_SIGNALS:
    void request(const QString &apiKey,
        LLModel::PromptContext *ctx,
        const QByteArray &array);

protected:
    // We have to implement these as they are pure virtual in base class, but we don't actually use
    // them as they are only called from the default implementation of 'prompt' which we override and
    // completely replace

    std::vector<Token> tokenize(PromptContext &ctx, const std::string &str, bool special) const override {
        (void)ctx;
        (void)str;
        (void)special;
        throw std::logic_error("not implemented");
    }

    std::string tokenToString(Token id) const override {
        (void)id;
        throw std::logic_error("not implemented");
    }

    Token sampleToken(PromptContext &ctx) const override {
        (void)ctx;
        throw std::logic_error("not implemented");
    }

    bool evalTokens(PromptContext &ctx, const std::vector<int32_t> &tokens) const override {
        (void)ctx;
        (void)tokens;
        throw std::logic_error("not implemented");
    }

    int32_t contextLength() const override {
        throw std::logic_error("not implemented");
    }

    const std::vector<Token> &endTokens() const override {
        throw std::logic_error("not implemented");
    }

    bool shouldAddBOS() const override {
        throw std::logic_error("not implemented");
    }

private:
    std::function<bool(int32_t, const std::string&)> m_responseCallback;
    QString m_modelName;
    QString m_apiKey;
    QList<QString> m_context;
};

#endif // CHATGPT_H
