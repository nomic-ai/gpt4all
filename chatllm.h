#ifndef CHATLLM_H
#define CHATLLM_H

#include <QObject>
#include <QThread>

#include "llmodel/llmodel.h"

class ChatLLM : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isModelLoaded READ isModelLoaded NOTIFY isModelLoadedChanged)
    Q_PROPERTY(QString response READ response NOTIFY responseChanged)
    Q_PROPERTY(QString modelName READ modelName WRITE setModelName NOTIFY modelNameChanged)
    Q_PROPERTY(int32_t threadCount READ threadCount WRITE setThreadCount NOTIFY threadCountChanged)
    Q_PROPERTY(bool isRecalc READ isRecalc NOTIFY recalcChanged)

public:

    ChatLLM();

    bool isModelLoaded() const;
    void regenerateResponse();
    void resetResponse();
    void resetContext();

    void stopGenerating() { m_stopGenerating = true; }
    void setThreadCount(int32_t n_threads);
    int32_t threadCount();

    QString response() const;
    QString modelName() const;

    void setModelName(const QString &modelName);

    bool isRecalc() const { return m_isRecalc; }

public Q_SLOTS:
    bool prompt(const QString &prompt, const QString &prompt_template, int32_t n_predict, int32_t top_k, float top_p,
                float temp, int32_t n_batch, float repeat_penalty, int32_t repeat_penalty_tokens);
    bool loadModel();
    void modelNameChangeRequested(const QString &modelName);

Q_SIGNALS:
    void isModelLoadedChanged();
    void responseChanged();
    void responseStarted();
    void responseStopped();
    void modelNameChanged();
    void threadCountChanged();
    void recalcChanged();
    void sendStartup();
    void sendModelLoaded();
    void sendResetContext();

private:
    void resetContextPrivate();
    bool loadModelPrivate(const QString &modelName);
    bool handlePrompt(int32_t token);
    bool handleResponse(int32_t token, const std::string &response);
    bool handleRecalculate(bool isRecalc);

private:
    LLModel::PromptContext m_ctx;
    LLModel *m_llmodel;
    std::string m_response;
    quint32 m_promptResponseTokens;
    quint32 m_responseLogits;
    QString m_modelName;
    QThread m_llmThread;
    std::atomic<bool> m_stopGenerating;
    bool m_isRecalc;
};

#endif // CHATLLM_H
