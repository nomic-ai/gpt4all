#ifndef CHATLLM_H
#define CHATLLM_H

#include "chatmodel.h"
#include "database.h" // IWYU pragma: keep
#include "modellist.h"

#include <gpt4all-backend/llmodel.h>

#include <QByteArray>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QList>      // IWYU pragma: keep
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList> // IWYU pragma: keep
#include <QStringView>
#include <QThread>
#include <QVariantMap> // IWYU pragma: keep
#include <QtGlobal>

#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <variant>

using namespace Qt::Literals::StringLiterals;

class QDataStream;

// NOTE: values serialized to disk, do not change or reuse
enum class LLModelTypeV0 { // chat versions 2-5
    MPT       = 0,
    GPTJ      = 1,
    LLAMA     = 2,
    CHATGPT   = 3,
    REPLIT    = 4,
    FALCON    = 5,
    BERT      = 6, // not used
    STARCODER = 7,
};
enum class LLModelTypeV1 { // since chat version 6 (v2.5.0)
    GPTJ      = 0, // not for new chats
    LLAMA     = 1,
    API       = 2,
    BERT      = 3, // not used
    // none of the below are used in new chats
    REPLIT    = 4,
    FALCON    = 5,
    MPT       = 6,
    STARCODER = 7,
    NONE      = -1, // no state
};

inline LLModelTypeV1 parseLLModelTypeV1(int type)
{
    switch (LLModelTypeV1(type)) {
    case LLModelTypeV1::GPTJ:
    case LLModelTypeV1::LLAMA:
    case LLModelTypeV1::API:
    // case LLModelTypeV1::BERT: -- not used
    case LLModelTypeV1::REPLIT:
    case LLModelTypeV1::FALCON:
    case LLModelTypeV1::MPT:
    case LLModelTypeV1::STARCODER:
        return LLModelTypeV1(type);
    default:
        return LLModelTypeV1::NONE;
    }
}

inline LLModelTypeV1 parseLLModelTypeV0(int v0)
{
    switch (LLModelTypeV0(v0)) {
    case LLModelTypeV0::MPT:       return LLModelTypeV1::MPT;
    case LLModelTypeV0::GPTJ:      return LLModelTypeV1::GPTJ;
    case LLModelTypeV0::LLAMA:     return LLModelTypeV1::LLAMA;
    case LLModelTypeV0::CHATGPT:   return LLModelTypeV1::API;
    case LLModelTypeV0::REPLIT:    return LLModelTypeV1::REPLIT;
    case LLModelTypeV0::FALCON:    return LLModelTypeV1::FALCON;
    // case LLModelTypeV0::BERT: -- not used
    case LLModelTypeV0::STARCODER: return LLModelTypeV1::STARCODER;
    default:                       return LLModelTypeV1::NONE;
    }
}

class ChatLLM;
class ChatModel;

struct LLModelInfo {
    std::unique_ptr<LLModel> model;
    QFileInfo fileInfo;
    std::optional<QString> fallbackReason;

    // NOTE: This does not store the model type or name on purpose as this is left for ChatLLM which
    // must be able to serialize the information even if it is in the unloaded state

    void resetModel(ChatLLM *cllm, LLModel *model = nullptr);
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
        emit report(u"%1 tokens/sec"_s.arg(m_tokens / float(m_elapsed / 1000.0f), 0, 'g', 2));
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
    Q_PROPERTY(QString deviceBackend READ deviceBackend NOTIFY loadedModelInfoChanged)
    Q_PROPERTY(QString device READ device NOTIFY loadedModelInfoChanged)
    Q_PROPERTY(QString fallbackReason READ fallbackReason NOTIFY loadedModelInfoChanged)
public:
    ChatLLM(Chat *parent, bool isServer = false);
    virtual ~ChatLLM();

    static void destroyStore();
    static std::optional<std::string> checkJinjaTemplateError(const std::string &source);

    void destroy();
    bool isModelLoaded() const;
    void regenerateResponse(int index);
    // used to implement edit functionality
    std::optional<QString> popPrompt(int index);

    void stopGenerating() { m_stopGenerating = true; }

    bool shouldBeLoaded() const { return m_shouldBeLoaded; }
    void setShouldBeLoaded(bool b);
    void requestTrySwitchContext();
    void setForceUnloadModel(bool b) { m_forceUnloadModel = b; }
    void setMarkedForDeletion(bool b) { m_markedForDeletion = b; }

    ModelInfo modelInfo() const;
    void setModelInfo(const ModelInfo &info);

    void acquireModel();
    void resetModel();

    QString deviceBackend() const
    {
        if (!isModelLoaded()) return QString();
        std::string name = LLModel::GPUDevice::backendIdToName(m_llModelInfo.model->backendName());
        return QString::fromStdString(name);
    }

    QString device() const
    {
        if (!isModelLoaded()) return QString();
        const char *name = m_llModelInfo.model->gpuDeviceName();
        return name ? QString(name) : u"CPU"_s;
    }

    // not loaded -> QString(), no fallback -> QString("")
    QString fallbackReason() const
    {
        if (!isModelLoaded()) return QString();
        return m_llModelInfo.fallbackReason.value_or(u""_s);
    }

    bool serialize(QDataStream &stream, int version);
    bool deserialize(QDataStream &stream, int version);

public Q_SLOTS:
    void prompt(const QStringList &enabledCollections);
    bool loadDefaultModel();
    void trySwitchContextOfLoadedModel(const ModelInfo &modelInfo);
    bool loadModel(const ModelInfo &modelInfo);
    void modelChangeRequested(const ModelInfo &modelInfo);
    void unloadModel();
    void reloadModel();
    void generateName();
    void handleChatIdChanged(const QString &id);
    void handleShouldBeLoadedChanged();
    void handleThreadStarted();
    void handleForceMetalChanged(bool forceMetal);
    void handleDeviceChanged();

Q_SIGNALS:
    void loadedModelInfoChanged();
    void modelLoadingPercentageChanged(float);
    void modelLoadingError(const QString &error);
    void modelLoadingWarning(const QString &warning);
    void responseChanged();
    void responseFailed();
    void promptProcessing();
    void generatingQuestions();
    void responseStopped(qint64 promptResponseMs);
    void generatedNameChanged(const QString &name);
    void generatedQuestionFinished(const QString &generatedQuestion);
    void stateChanged();
    void threadStarted();
    void shouldBeLoadedChanged();
    void trySwitchContextRequested(const ModelInfo &modelInfo);
    void trySwitchContextOfLoadedModelCompleted(int value);
    void requestRetrieveFromDB(const QList<QString> &collections, const QString &text, int retrievalSize, QList<ResultInfo> *results);
    void reportSpeed(const QString &speed);
    void reportDevice(const QString &device);
    void reportFallbackReason(const QString &fallbackReason);
    void databaseResultsChanged(const QList<ResultInfo>&);
    void modelInfoChanged(const ModelInfo &modelInfo);

protected:
    struct PromptResult {
        QByteArray response;       // raw UTF-8
        int        promptTokens;   // note: counts *entire* history, even if cached
        int        responseTokens;
    };

    struct ChatPromptResult : PromptResult {
        QList<ResultInfo> databaseResults;
    };

    ChatPromptResult promptInternalChat(const QStringList &enabledCollections, const LLModel::PromptContext &ctx,
                                        qsizetype startOffset = 0);
    // passing a string_view directly skips templating and uses the raw string
    PromptResult promptInternal(const std::variant<std::span<const MessageItem>, std::string_view> &prompt,
                                const LLModel::PromptContext &ctx,
                                bool usedLocalDocs);

private:
    bool loadNewModel(const ModelInfo &modelInfo, QVariantMap &modelLoadProps);

    std::vector<MessageItem> forkConversation(const QString &prompt) const;

    // Applies the Jinja template. Query mode returns only the last message without special tokens.
    // Returns a (# of messages, rendered prompt) pair.
    std::string applyJinjaTemplate(std::span<const MessageItem> items) const;

    void generateQuestions(qint64 elapsed);

protected:
    QPointer<ChatModel> m_chatModel;

private:
    const Chat *m_chat;
    LLModelInfo m_llModelInfo;
    LLModelTypeV1 m_llModelType = LLModelTypeV1::NONE;
    ModelInfo m_modelInfo;
    TokenTimer *m_timer;
    QThread m_llmThread;
    std::atomic<bool> m_stopGenerating;
    std::atomic<bool> m_shouldBeLoaded;
    std::atomic<bool> m_forceUnloadModel;
    std::atomic<bool> m_markedForDeletion;
    bool m_isServer;
    bool m_forceMetal;
    bool m_reloadingToChangeVariant;
};

#endif // CHATLLM_H
