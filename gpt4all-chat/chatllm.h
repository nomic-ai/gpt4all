#ifndef CHATLLM_H
#define CHATLLM_H

#include <QObject>
#include <QThread>
#include <QFileInfo>

#include "../gpt4all-backend/llmodel.h"

enum LLModelType {
    MPT_,
    GPTJ_,
    LLAMA_,
    CHATGPT_,
};

struct LLModelInfo {
    LLModel *model = nullptr;
    QFileInfo fileInfo;
    // NOTE: This does not store the model type or name on purpose as this is left for ChatLLM which
    // must be able to serialize the information even if it is in the unloaded state
};

class Chat;
class ChatLLM : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isModelLoaded READ isModelLoaded NOTIFY isModelLoadedChanged)
    Q_PROPERTY(QString response READ response NOTIFY responseChanged)
    Q_PROPERTY(QString modelName READ modelName WRITE setModelName NOTIFY modelNameChanged)
    Q_PROPERTY(bool isRecalc READ isRecalc NOTIFY recalcChanged)
    Q_PROPERTY(QString generatedName READ generatedName NOTIFY generatedNameChanged)

public:
    ChatLLM(Chat *parent, bool isServer = false);
    virtual ~ChatLLM();

    bool isModelLoaded() const;
    void regenerateResponse();
    void resetResponse();
    void resetContext();

    void stopGenerating() { m_stopGenerating = true; }

    bool shouldBeLoaded() const { return m_shouldBeLoaded; }
    void setShouldBeLoaded(bool b);

    QString response() const;
    QString modelName() const;

    void setModelName(const QString &modelName);

    bool isRecalc() const { return m_isRecalc; }

    QString generatedName() const { return QString::fromStdString(m_nameResponse); }

    bool serialize(QDataStream &stream, int version);
    bool deserialize(QDataStream &stream, int version);

public Q_SLOTS:
    bool prompt(const QString &prompt, const QString &prompt_template, int32_t n_predict,
        int32_t top_k, float top_p, float temp, int32_t n_batch, float repeat_penalty, int32_t repeat_penalty_tokens,
        int32_t n_threads);
    bool loadDefaultModel();
    bool loadModel(const QString &modelName);
    void modelNameChangeRequested(const QString &modelName);
    void forceUnloadModel();
    void unloadModel();
    void reloadModel();
    void generateName();
    void handleChatIdChanged();
    void handleShouldBeLoadedChanged();

Q_SIGNALS:
    void isModelLoadedChanged();
    void modelLoadingError(const QString &error);
    void responseChanged();
    void responseStarted();
    void responseStopped();
    void modelNameChanged();
    void recalcChanged();
    void sendStartup();
    void sendModelLoaded();
    void sendResetContext();
    void generatedNameChanged();
    void stateChanged();
    void threadStarted();
    void shouldBeLoadedChanged();

protected:
    void resetContextProtected();
    bool handlePrompt(int32_t token);
    bool handleResponse(int32_t token, const std::string &response);
    bool handleRecalculate(bool isRecalc);
    bool handleNamePrompt(int32_t token);
    bool handleNameResponse(int32_t token, const std::string &response);
    bool handleNameRecalculate(bool isRecalc);
    void saveState();
    void restoreState();

protected:
    LLModel::PromptContext m_ctx;
    quint32 m_promptTokens;
    quint32 m_promptResponseTokens;
    LLModelInfo m_modelInfo;
    LLModelType m_modelType;
    std::string m_response;
    std::string m_nameResponse;
    quint32 m_responseLogits;
    QString m_modelName;
    Chat *m_chat;
    QByteArray m_state;
    QThread m_llmThread;
    std::atomic<bool> m_stopGenerating;
    std::atomic<bool> m_shouldBeLoaded;
    bool m_isRecalc;
    bool m_isServer;
    bool m_isChatGPT;
};

#endif // CHATLLM_H
