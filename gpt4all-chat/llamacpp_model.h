#pragma once

#include "database.h" // IWYU pragma: keep
#include "modellist.h"

#include "../gpt4all-backend/llamacpp_backend.h"
#include "../gpt4all-backend/model_backend.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QList>
#include <QObject>
#include <QPair>
#include <QString>
#include <QThread>
#include <QVariantMap>
#include <QVector>
#include <QtGlobal>

#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

using namespace Qt::Literals::StringLiterals;

class Chat;
class LlamaCppModel;
class QDataStream;

// NOTE: values serialized to disk, do not change or reuse
enum LLModelType {
    GPTJ_  = 0, // no longer used
    LLAMA_ = 1,
    API_   = 2,
    BERT_  = 3, // no longer used
};

struct LLModelInfo {
    std::unique_ptr<ModelBackend> model;
    QFileInfo fileInfo;
    std::optional<QString> fallbackReason;

    // NOTE: This does not store the model type or name on purpose as this is left for LlamaCppModel which
    // must be able to serialize the information even if it is in the unloaded state

    void resetModel(LlamaCppModel *cllm, ModelBackend *model = nullptr);
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

class LlamaCppModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool restoringFromText READ restoringFromText NOTIFY restoringFromTextChanged)
    Q_PROPERTY(QString deviceBackend READ deviceBackend NOTIFY loadedModelInfoChanged)
    Q_PROPERTY(QString device READ device NOTIFY loadedModelInfoChanged)
    Q_PROPERTY(QString fallbackReason READ fallbackReason NOTIFY loadedModelInfoChanged)

public:
    LlamaCppModel(Chat *parent, bool isServer = false);
    virtual ~LlamaCppModel();

    void destroy();
    static void destroyStore();
    void regenerateResponse();
    void resetResponse();
    void resetContext();

    void stopGenerating() { m_stopGenerating = true; }

    void setShouldBeLoaded(bool b);
    void requestTrySwitchContext();
    void setForceUnloadModel(bool b) { m_forceUnloadModel = b; }
    void setMarkedForDeletion(bool b) { m_markedForDeletion = b; }

    void setModelInfo(const ModelInfo &info);

    bool restoringFromText() const { return m_restoringFromText; }

    QString deviceBackend() const
    {
        auto *lcppmodel = dynamic_cast<LlamaCppBackend *>(m_llModelInfo.model.get());
        if (!isModelLoaded() && !lcppmodel) return QString();
        std::string name = LlamaCppBackend::GPUDevice::backendIdToName(lcppmodel->backendName());
        return QString::fromStdString(name);
    }

    QString device() const
    {
        auto *lcppmodel = dynamic_cast<LlamaCppBackend *>(m_llModelInfo.model.get());
        if (!isModelLoaded() || !lcppmodel) return QString();
        const char *name = lcppmodel->gpuDeviceName();
        return name ? QString(name) : u"CPU"_s;
    }

    // not loaded -> QString(), no fallback -> QString("")
    QString fallbackReason() const
    {
        if (!isModelLoaded()) return QString();
        return m_llModelInfo.fallbackReason.value_or(u""_s);
    }

    bool serialize(QDataStream &stream, int version, bool serializeKV);
    bool deserialize(QDataStream &stream, int version, bool deserializeKV, bool discardKV);
    void setStateFromText(const QVector<QPair<QString, QString>> &stateFromText) { m_stateFromText = stateFromText; }

public Q_SLOTS:
    bool prompt(const QList<QString> &collectionList, const QString &prompt);
    bool loadDefaultModel();
    bool loadModel(const ModelInfo &modelInfo);
    void modelChangeRequested(const ModelInfo &modelInfo);
    void generateName();
    void processSystemPrompt();

Q_SIGNALS:
    void restoringFromTextChanged();
    void loadedModelInfoChanged();
    void modelLoadingPercentageChanged(float);
    void modelLoadingError(const QString &error);
    void modelLoadingWarning(const QString &warning);
    void responseChanged(const QString &response);
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
    void databaseResultsChanged(const QList<ResultInfo> &results);
    void modelInfoChanged(const ModelInfo &modelInfo);

protected:
    bool isModelLoaded() const;
    void acquireModel();
    void resetModel();
    bool promptInternal(const QList<QString> &collectionList, const QString &prompt, const QString &promptTemplate,
        int32_t n_predict, int32_t top_k, float top_p, float min_p, float temp, int32_t n_batch, float repeat_penalty,
        int32_t repeat_penalty_tokens);
    bool handlePrompt(int32_t token);
    bool handleResponse(int32_t token, const std::string &response);
    bool handleNamePrompt(int32_t token);
    bool handleNameResponse(int32_t token, const std::string &response);
    bool handleSystemPrompt(int32_t token);
    bool handleSystemResponse(int32_t token, const std::string &response);
    bool handleRestoreStateFromTextPrompt(int32_t token);
    bool handleRestoreStateFromTextResponse(int32_t token, const std::string &response);
    bool handleQuestionPrompt(int32_t token);
    bool handleQuestionResponse(int32_t token, const std::string &response);
    void saveState();
    void restoreState();

    // used by Server class
    ModelInfo modelInfo() const { return m_modelInfo; }
    QString response() const;
    QString generatedName() const { return QString::fromStdString(m_nameResponse); }

protected Q_SLOTS:
    void trySwitchContextOfLoadedModel(const ModelInfo &modelInfo);
    void unloadModel();
    void reloadModel();
    void generateQuestions(qint64 elapsed);
    void handleChatIdChanged(const QString &id);
    void handleShouldBeLoadedChanged();
    void handleThreadStarted();
    void handleForceMetalChanged(bool forceMetal);
    void handleDeviceChanged();
    void processRestoreStateFromText();

private:
    bool loadNewModel(const ModelInfo &modelInfo, QVariantMap &modelLoadProps);

protected:
    ModelBackend::PromptContext m_ctx;
    quint32 m_promptTokens;
    quint32 m_promptResponseTokens;

private:
    std::string m_response;
    std::string m_nameResponse;
    QString m_questionResponse;
    LLModelInfo m_llModelInfo;
    LLModelType m_llModelType;
    ModelInfo m_modelInfo;
    TokenTimer *m_timer;
    QByteArray m_state;
    QThread m_llmThread;
    std::atomic<bool> m_stopGenerating;
    std::atomic<bool> m_shouldBeLoaded;
    std::atomic<bool> m_restoringFromText; // status indication
    std::atomic<bool> m_forceUnloadModel;
    std::atomic<bool> m_markedForDeletion;
    bool m_isServer;
    bool m_forceMetal;
    bool m_reloadingToChangeVariant;
    bool m_processedSystemPrompt;
    bool m_restoreStateFromText;
    // m_pristineLoadedState is set if saveSate is unnecessary, either because:
    // - an unload was queued during ModelBackend::restoreState()
    // - the chat will be restored from text and hasn't been interacted with yet
    bool m_pristineLoadedState = false;
    QVector<QPair<QString, QString>> m_stateFromText;
};
