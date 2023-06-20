#ifndef CHATGPT_H
#define CHATGPT_H

#include <QObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include "../gpt4all-backend/llmodel.h"

class ChatGPTPrivate;
class ChatGPT : public QObject, public LLModel {
    Q_OBJECT
public:
    ChatGPT();
    virtual ~ChatGPT();

    bool loadModel(const std::string &modelPath) override;
    bool isModelLoaded() const override;
    size_t stateSize() const override;
    size_t saveState(uint8_t *dest) const override;
    size_t restoreState(const uint8_t *src) override;
    void prompt(const std::string &prompt,
        std::function<bool(int32_t)> promptCallback,
        std::function<bool(int32_t, const std::string&)> responseCallback,
        std::function<bool(bool)> recalculateCallback,
        PromptContext &ctx) override;

    void setThreadCount(int32_t n_threads) override;
    int32_t threadCount() const override;

    void setModelName(const QString &modelName) { m_modelName = modelName; }
    void setAPIKey(const QString &apiKey) { m_apiKey = apiKey; }

    QList<QString> context() const { return m_context; }
    void setContext(const QList<QString> &context) { m_context = context; }

protected:
    // We have to implement these as they are pure virtual in base class, but we don't actually use
    // them as they are only called from the default implementation of 'prompt' which we override and
    // completely replace
    std::vector<Token> tokenize(PromptContext &, const std::string&) const override { return std::vector<Token>(); }
    std::string tokenToString(Token) const override { return std::string(); }
    Token sampleToken(PromptContext &ctx) const override { return -1; }
    bool evalTokens(PromptContext &/*ctx*/, const std::vector<int32_t>& /*tokens*/) const override { return false; }
    int32_t contextLength() const override { return -1; }
    const std::vector<Token>& endTokens() const override { static const std::vector<Token> fres; return fres; }

private Q_SLOTS:
    void handleFinished();
    void handleReadyRead();
    void handleErrorOccurred(QNetworkReply::NetworkError code);

private:
    PromptContext *m_ctx;
    std::function<bool(int32_t, const std::string&)> m_responseCallback;
    QString m_modelName;
    QString m_apiKey;
    QList<QString> m_context;
    QString m_currentResponse;
    QNetworkAccessManager m_networkManager;
};

#endif // CHATGPT_H
