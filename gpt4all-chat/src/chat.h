#ifndef CHAT_H
#define CHAT_H

#include "chatllm.h"
#include "chatmodel.h"
#include "database.h" // IWYU pragma: keep
#include "localdocsmodel.h" // IWYU pragma: keep
#include "modellist.h"

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QStringList> // IWYU pragma: keep
#include <QStringView>
#include <QtGlobal>

class QDataStream;

class Chat : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ id NOTIFY idChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(ChatModel *chatModel READ chatModel NOTIFY chatModelChanged)
    Q_PROPERTY(bool isModelLoaded READ isModelLoaded NOTIFY isModelLoadedChanged)
    Q_PROPERTY(bool isCurrentlyLoading READ isCurrentlyLoading NOTIFY isCurrentlyLoadingChanged)
    Q_PROPERTY(float modelLoadingPercentage READ modelLoadingPercentage NOTIFY modelLoadingPercentageChanged)
    Q_PROPERTY(ModelInfo modelInfo READ modelInfo WRITE setModelInfo NOTIFY modelInfoChanged)
    Q_PROPERTY(bool responseInProgress READ responseInProgress NOTIFY responseInProgressChanged)
    Q_PROPERTY(bool isServer READ isServer NOTIFY isServerChanged)
    Q_PROPERTY(ResponseState responseState READ responseState NOTIFY responseStateChanged)
    Q_PROPERTY(QList<QString> collectionList READ collectionList NOTIFY collectionListChanged)
    Q_PROPERTY(QString modelLoadingError READ modelLoadingError NOTIFY modelLoadingErrorChanged)
    Q_PROPERTY(QString tokenSpeed READ tokenSpeed NOTIFY tokenSpeedChanged)
    Q_PROPERTY(QString deviceBackend READ deviceBackend NOTIFY loadedModelInfoChanged)
    Q_PROPERTY(QString device READ device NOTIFY loadedModelInfoChanged)
    Q_PROPERTY(QString fallbackReason READ fallbackReason NOTIFY loadedModelInfoChanged)
    Q_PROPERTY(LocalDocsCollectionsModel *collectionModel READ collectionModel NOTIFY collectionModelChanged)
    // 0=no, 1=waiting, 2=working
    Q_PROPERTY(int trySwitchContextInProgress READ trySwitchContextInProgress NOTIFY trySwitchContextInProgressChanged)
    Q_PROPERTY(QList<QString> generatedQuestions READ generatedQuestions NOTIFY generatedQuestionsChanged)
    QML_ELEMENT
    QML_UNCREATABLE("Only creatable from c++!")

public:
    // tag for constructing a server chat
    struct server_tag_t { explicit server_tag_t() = default; };
    static inline constexpr server_tag_t server_tag = server_tag_t();

    enum ResponseState {
        ResponseStopped,
        LocalDocsRetrieval,
        LocalDocsProcessing,
        PromptProcessing,
        GeneratingQuestions,
        ResponseGeneration
    };
    Q_ENUM(ResponseState)

    explicit Chat(QObject *parent = nullptr);
    explicit Chat(server_tag_t, QObject *parent = nullptr);
    virtual ~Chat();
    void destroy() { m_llmodel->destroy(); }
    void connectLLM();

    QString id() const { return m_id; }
    QString name() const { return m_userName.isEmpty() ? m_name : m_userName; }
    void setName(const QString &name)
    {
        m_userName = name;
        emit nameChanged();
        m_needsSave = true;
    }
    ChatModel *chatModel() { return m_chatModel; }

    bool isNewChat() const { return m_name == tr("New Chat") && !m_chatModel->count(); }

    Q_INVOKABLE void reset();
    bool  isModelLoaded()          const { return m_modelLoadingPercentage == 1.0f; }
    bool  isCurrentlyLoading()     const { return m_modelLoadingPercentage > 0.0f && m_modelLoadingPercentage < 1.0f; }
    float modelLoadingPercentage() const { return m_modelLoadingPercentage; }
    Q_INVOKABLE void newPromptResponsePair(const QString &prompt, const QList<QUrl> &attachedUrls = {});
    Q_INVOKABLE void regenerateResponse(int index);
    Q_INVOKABLE QVariant popPrompt(int index);
    Q_INVOKABLE void stopGenerating();

    QList<ResultInfo> databaseResults() const { return m_databaseResults; }

    bool responseInProgress() const { return m_responseInProgress; }
    ResponseState responseState() const;
    ModelInfo modelInfo() const;
    void setModelInfo(const ModelInfo &modelInfo);

    Q_INVOKABLE void unloadModel();
    Q_INVOKABLE void reloadModel();
    Q_INVOKABLE void forceUnloadModel();
    Q_INVOKABLE void forceReloadModel();
    Q_INVOKABLE void trySwitchContextOfLoadedModel();
    void unloadAndDeleteLater();
    void markForDeletion();

    QDateTime creationDate() const { return QDateTime::fromSecsSinceEpoch(m_creationDate); }
    bool serialize(QDataStream &stream, int version) const;
    bool deserialize(QDataStream &stream, int version);
    bool isServer() const { return m_isServer; }

    QList<QString> collectionList() const;
    LocalDocsCollectionsModel *collectionModel() const { return m_collectionModel; }

    Q_INVOKABLE bool hasCollection(const QString &collection) const;
    Q_INVOKABLE void addCollection(const QString &collection);
    Q_INVOKABLE void removeCollection(const QString &collection);

    QString modelLoadingError() const { return m_modelLoadingError; }

    QString tokenSpeed() const { return m_tokenSpeed; }
    QString deviceBackend() const;
    QString device() const;
    // not loaded -> QString(), no fallback -> QString("")
    QString fallbackReason() const;

    int trySwitchContextInProgress() const { return m_trySwitchContextInProgress; }

    QList<QString> generatedQuestions() const { return m_generatedQuestions; }

    bool needsSave() const { return m_needsSave; }
    void setNeedsSave(bool n) { m_needsSave = n; }

public Q_SLOTS:
    void resetResponseState();

Q_SIGNALS:
    void idChanged(const QString &id);
    void nameChanged();
    void chatModelChanged();
    void isModelLoadedChanged();
    void isCurrentlyLoadingChanged();
    void modelLoadingPercentageChanged();
    void modelLoadingWarning(const QString &warning);
    void responseInProgressChanged();
    void responseStateChanged();
    void promptRequested(const QStringList &enabledCollections);
    void regenerateResponseRequested(int index);
    void resetResponseRequested();
    void resetContextRequested();
    void modelChangeRequested(const ModelInfo &modelInfo);
    void modelInfoChanged();
    void loadDefaultModelRequested();
    void generateNameRequested();
    void modelLoadingErrorChanged();
    void isServerChanged();
    void collectionListChanged(const QList<QString> &collectionList);
    void tokenSpeedChanged();
    void deviceChanged();
    void fallbackReasonChanged();
    void collectionModelChanged();
    void trySwitchContextInProgressChanged();
    void loadedModelInfoChanged();
    void generatedQuestionsChanged();

private Q_SLOTS:
    void handleResponseChanged();
    void handleModelLoadingPercentageChanged(float);
    void promptProcessing();
    void generatingQuestions();
    void responseStopped(qint64 promptResponseMs);
    void generatedNameChanged(const QString &name);
    void generatedQuestionFinished(const QString &question);
    void handleModelLoadingError(const QString &error);
    void handleTokenSpeedChanged(const QString &tokenSpeed);
    void handleDatabaseResultsChanged(const QList<ResultInfo> &results);
    void handleModelInfoChanged(const ModelInfo &modelInfo);
    void handleModelChanged(const ModelInfo &modelInfo);
    void handleTrySwitchContextOfLoadedModelCompleted(int value);

private:
    QString m_id;
    QString m_name;
    QString m_generatedName;
    QString m_userName;
    ModelInfo m_modelInfo;
    QString m_modelLoadingError;
    QString m_tokenSpeed;
    QString m_device;
    QString m_fallbackReason;
    QList<QString> m_collections;
    QList<QString> m_generatedQuestions;
    ChatModel *m_chatModel;
    bool m_responseInProgress = false;
    ResponseState m_responseState;
    qint64 m_creationDate;
    ChatLLM *m_llmodel;
    QList<ResultInfo> m_databaseResults;
    bool m_isServer = false;
    bool m_shouldDeleteLater = false;
    float m_modelLoadingPercentage = 0.0f;
    LocalDocsCollectionsModel *m_collectionModel;
    bool m_firstResponse = true;
    int m_trySwitchContextInProgress = 0;
    bool m_isCurrentlyLoading = false;
    // True if we need to serialize the chat to disk, because of one of two reasons:
    // - The chat was freshly created during this launch.
    // - The chat was changed after loading it from disk.
    bool m_needsSave = true;
    int m_consecutiveToolCalls = 0;
};

#endif // CHAT_H
