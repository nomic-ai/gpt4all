#ifndef CHATAPI_H
#define CHATAPI_H

#include "../gpt4all-backend/model_backend.h"

#include <QByteArray>
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <functional>
#include <string>
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
                 ModelBackend::PromptContext *promptCtx,
                 const QByteArray &array);

Q_SIGNALS:
    void finished();

private Q_SLOTS:
    void handleFinished();
    void handleReadyRead();
    void handleErrorOccurred(QNetworkReply::NetworkError code);

private:
    ChatAPI *m_chat;
    ModelBackend::PromptContext *m_ctx;
    QNetworkAccessManager *m_networkManager;
    QString m_currentResponse;
};

class ChatAPI : public QObject, public ModelBackend {
    Q_OBJECT
public:
    ChatAPI();
    virtual ~ChatAPI();

    bool loadModel(const std::string &modelPath, int n_ctx, int ngl) override;
    bool isModelLoaded() const override;
    size_t stateSize() const override;
    size_t saveState(uint8_t *dest) const override;
    size_t restoreState(const uint8_t *src) override;
    void prompt(const std::string &prompt,
                const std::string &promptTemplate,
                std::function<bool(int32_t)> promptCallback,
                std::function<bool(int32_t, const std::string&)> responseCallback,
                bool allowContextShift,
                PromptContext &ctx,
                bool special,
                std::string *fakeReply) override;

    void setModelName(const QString &modelName) { m_modelName = modelName; }
    void setAPIKey(const QString &apiKey) { m_apiKey = apiKey; }
    void setRequestURL(const QString &requestURL) { m_requestURL = requestURL; }
    QString url() const { return m_requestURL; }

    QList<QString> context() const { return m_context; }
    void setContext(const QList<QString> &context) { m_context = context; }

    bool callResponse(int32_t token, const std::string &string);

Q_SIGNALS:
    void request(const QString &apiKey,
                 ModelBackend::PromptContext *ctx,
                 const QByteArray &array);

private:
    std::function<bool(int32_t, const std::string&)> m_responseCallback;
    QString m_modelName;
    QString m_apiKey;
    QString m_requestURL;
    QList<QString> m_context;
    QStringList m_queuedPrompts;
};

#endif // CHATAPI_H
