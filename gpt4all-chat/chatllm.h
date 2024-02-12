#ifndef CHATLLM_H
#define CHATLLM_H

#include <QObject>
#include <QThread>
#include <QFileInfo>

#include "database.h"
#include "modellist.h"
#include "../gpt4all-backend/llmodel.h"

enum LLModelType {
    GPTJ_,
    LLAMA_,
    CHATGPT_,
    BERT_,
};

struct LLModelInfo {
    LLModel *model = nullptr;
    QFileInfo fileInfo;
    // NOTE: This does not store the model type or name on purpose as this is left for ChatLLM which
    // must be able to serialize the information even if it is in the unloaded state
};

class TokenTimer : public QObject {
    Q_OBJECT
public:
    explicit TokenTimer(QObject *parent)
        : QObject(parent)
        , m_elapsed(0) {}

    static int rollingAverage(int oldAvg, int newNumber, int n)
    {
        // i.e. to calculate the new average after then nth number,
        // you multiply the old average by n−1, add the new number, and divide the total by n.
        return qRound(((float(oldAvg) * (n - 1)) + newNumber) / float(n));
    }

    void start() { m_tokens = 0; m_elapsed = 0; m_time.invalidate(); }
    void stop() { handleTimeout(); }
    void inc() {
        if (!m_time.isValid())
            m_time.start();
        ++m_tokens;
        if (m_time.elapsed() > 999)
            handleTimeout();
    }

Q_SIGNALS:
    void report(const QString &speed);

private Q_SLOTS:
    void handleTimeout()
    {
        m_elapsed += m_time.restart();
        emit report(QString("%1 tokens/sec").arg(m_tokens / float(m_elapsed / 1000.0f), 0, 'g', 2));
    }

private:
    QElapsedTimer m_time;
    qint64 m_elapsed;
    quint32 m_tokens;
};

class Chat;
class ChatLLM : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isRecalc READ isRecalc NOTIFY recalcChanged)
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

    ModelInfo modelInfo() const;
    void setModelInfo(const ModelInfo &info);

    bool isRecalc() const { return m_isRecalc; }

    QString generatedName() const { return QString::fromStdString(m_nameResponse); }

    bool serialize(QDataStream &stream, int version, bool serializeKV);
    bool deserialize(QDataStream &stream, int version, bool deserializeKV, bool discardKV);
    void setStateFromText(const QVector<QPair<QString, QString>> &stateFromText) { m_stateFromText = stateFromText; }

public Q_SLOTS:
    bool prompt(const QList<QString> &collectionList, const QString &prompt);
    bool loadDefaultModel();
    bool loadModel(const ModelInfo &modelInfo);
    void modelChangeRequested(const ModelInfo &modelInfo);
    void forceUnloadModel();
    void unloadModel();
    void reloadModel();
    void generateName();
    void handleChatIdChanged(const QString &id);
    void handleShouldBeLoadedChanged();
    void handleThreadStarted();
    void handleForceMetalChanged(bool forceMetal);
    void handleDeviceChanged();
    void processSystemPrompt();
    void processRestoreStateFromText();

Q_SIGNALS:
    void recalcChanged();
    void isModelLoadedChanged(bool);
    void modelLoadingError(const QString &error);
    void responseChanged(const QString &response);
    void promptProcessing();
    void responseStopped();
    void sendStartup();
    void sendModelLoaded();
    void generatedNameChanged(const QString &name);
    void stateChanged();
    void threadStarted();
    void shouldBeLoadedChanged();
    void requestRetrieveFromDB(const QList<QString> &collections, const QString &text, int retrievalSize, QList<ResultInfo> *results);
    void reportSpeed(const QString &speed);
    void reportDevice(const QString &device);
    void reportFallbackReason(const QString &fallbackReason);
    void databaseResultsChanged(const QList<ResultInfo>&);
    void modelInfoChanged(const ModelInfo &modelInfo);

protected:
    bool promptInternal(const QList<QString> &collectionList, const QString &prompt, const QString &promptTemplate,
        int32_t n_predict, int32_t top_k, float top_p, float temp, int32_t n_batch, float repeat_penalty,
        int32_t repeat_penalty_tokens);
    bool handlePrompt(int32_t token);
    bool handleResponse(int32_t token, const std::string &response);
    bool handleRecalculate(bool isRecalc);
    bool handleNamePrompt(int32_t token);
    bool handleNameResponse(int32_t token, const std::string &response);
    bool handleNameRecalculate(bool isRecalc);
    bool handleSystemPrompt(int32_t token);
    bool handleSystemResponse(int32_t token, const std::string &response);
    bool handleSystemRecalculate(bool isRecalc);
    bool handleRestoreStateFromTextPrompt(int32_t token);
    bool handleRestoreStateFromTextResponse(int32_t token, const std::string &response);
    bool handleRestoreStateFromTextRecalculate(bool isRecalc);
    void saveState();
    void restoreState();

protected:
    LLModel::PromptContext m_ctx;
    quint32 m_promptTokens;
    quint32 m_promptResponseTokens;

private:
    std::string m_response;
    std::string m_nameResponse;
    LLModelInfo m_llModelInfo;
    LLModelType m_llModelType;
    ModelInfo m_modelInfo;
    TokenTimer *m_timer;
    QByteArray m_state;
    QThread m_llmThread;
    std::atomic<bool> m_stopGenerating;
    std::atomic<bool> m_shouldBeLoaded;
    std::atomic<bool> m_isRecalc;
    bool m_isServer;
    bool m_forceMetal;
    bool m_reloadingToChangeVariant;
    bool m_processedSystemPrompt;
    bool m_restoreStateFromText;
    QVector<QPair<QString, QString>> m_stateFromText;
};

#endif // CHATLLM_H
