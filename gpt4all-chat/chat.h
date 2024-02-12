#ifndef CHAT_H
#define CHAT_H

#include <QObject>
#include <QtQml>
#include <QDataStream>

#include "chatllm.h"
#include "chatmodel.h"
#include "database.h"
#include "localdocsmodel.h"

class Chat : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ id NOTIFY idChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(ChatModel *chatModel READ chatModel NOTIFY chatModelChanged)
    Q_PROPERTY(bool isModelLoaded READ isModelLoaded NOTIFY isModelLoadedChanged)
    Q_PROPERTY(QString response READ response NOTIFY responseChanged)
    Q_PROPERTY(ModelInfo modelInfo READ modelInfo WRITE setModelInfo NOTIFY modelInfoChanged)
    Q_PROPERTY(bool responseInProgress READ responseInProgress NOTIFY responseInProgressChanged)
    Q_PROPERTY(bool isRecalc READ isRecalc NOTIFY recalcChanged)
    Q_PROPERTY(bool isServer READ isServer NOTIFY isServerChanged)
    Q_PROPERTY(ResponseState responseState READ responseState NOTIFY responseStateChanged)
    Q_PROPERTY(QList<QString> collectionList READ collectionList NOTIFY collectionListChanged)
    Q_PROPERTY(QString modelLoadingError READ modelLoadingError NOTIFY modelLoadingErrorChanged)
    Q_PROPERTY(QString tokenSpeed READ tokenSpeed NOTIFY tokenSpeedChanged);
    Q_PROPERTY(QString device READ device NOTIFY deviceChanged);
    Q_PROPERTY(QString fallbackReason READ fallbackReason NOTIFY fallbackReasonChanged);
    Q_PROPERTY(LocalDocsCollectionsModel *collectionModel READ collectionModel NOTIFY collectionModelChanged)
    QML_ELEMENT
    QML_UNCREATABLE("Only creatable from c++!")

public:
    enum ResponseState {
        ResponseStopped,
        LocalDocsRetrieval,
        LocalDocsProcessing,
        PromptProcessing,
        ResponseGeneration
    };
    Q_ENUM(ResponseState)

    explicit Chat(QObject *parent = nullptr);
    explicit Chat(bool isServer, QObject *parent = nullptr);
    virtual ~Chat();
    void connectLLM();

    QString id() const { return m_id; }
    QString name() const { return m_userName.isEmpty() ? m_name : m_userName; }
    void setName(const QString &name)
    {
        m_userName = name;
        emit nameChanged();
    }
    ChatModel *chatModel() { return m_chatModel; }

    bool isNewChat() const { return m_name == tr("New Chat") && !m_chatModel->count(); }

    Q_INVOKABLE void reset();
    Q_INVOKABLE void processSystemPrompt();
    Q_INVOKABLE bool isModelLoaded() const;
    Q_INVOKABLE void prompt(const QString &prompt);
    Q_INVOKABLE void regenerateResponse();
    Q_INVOKABLE void stopGenerating();
    Q_INVOKABLE void newPromptResponsePair(const QString &prompt);

    QList<ResultInfo> databaseResults() const { return m_databaseResults; }

    QString response() const;
    bool responseInProgress() const { return m_responseInProgress; }
    ResponseState responseState() const;
    ModelInfo modelInfo() const;
    void setModelInfo(const ModelInfo &modelInfo);
    bool isRecalc() const;

    void unloadModel();
    void reloadModel();
    void unloadAndDeleteLater();

    qint64 creationDate() const { return m_creationDate; }
    bool serialize(QDataStream &stream, int version) const;
    bool deserialize(QDataStream &stream, int version);
    bool isServer() const { return m_isServer; }

    QList<QString> collectionList() const;
    LocalDocsCollectionsModel *collectionModel() const { return m_collectionModel; }

    Q_INVOKABLE bool hasCollection(const QString &collection) const;
    Q_INVOKABLE void addCollection(const QString &collection);
    Q_INVOKABLE void removeCollection(const QString &collection);
    void resetResponseState();

    QString modelLoadingError() const { return m_modelLoadingError; }

    QString tokenSpeed() const { return m_tokenSpeed; }
    QString device() const { return m_device; }
    QString fallbackReason() const { return m_fallbackReason; }

public Q_SLOTS:
    void serverNewPromptResponsePair(const QString &prompt);

Q_SIGNALS:
    void idChanged(const QString &id);
    void nameChanged();
    void chatModelChanged();
    void isModelLoadedChanged();
    void responseChanged();
    void responseInProgressChanged();
    void responseStateChanged();
    void promptRequested(const QList<QString> &collectionList, const QString &prompt);
    void regenerateResponseRequested();
    void resetResponseRequested();
    void resetContextRequested();
    void processSystemPromptRequested();
    void modelChangeRequested(const ModelInfo &modelInfo);
    void modelInfoChanged();
    void recalcChanged();
    void loadDefaultModelRequested();
    void loadModelRequested(const ModelInfo &modelInfo);
    void generateNameRequested();
    void modelLoadingErrorChanged();
    void isServerChanged();
    void collectionListChanged(const QList<QString> &collectionList);
    void tokenSpeedChanged();
    void deviceChanged();
    void fallbackReasonChanged();
    void collectionModelChanged();

private Q_SLOTS:
    void handleResponseChanged(const QString &response);
    void handleModelLoadedChanged(bool);
    void promptProcessing();
    void responseStopped();
    void generatedNameChanged(const QString &name);
    void handleRecalculating();
    void handleModelLoadingError(const QString &error);
    void handleTokenSpeedChanged(const QString &tokenSpeed);
    void handleDeviceChanged(const QString &device);
    void handleFallbackReasonChanged(const QString &device);
    void handleDatabaseResultsChanged(const QList<ResultInfo> &results);
    void handleModelInfoChanged(const ModelInfo &modelInfo);
    void handleModelInstalled();

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
    QString m_response;
    QList<QString> m_collections;
    ChatModel *m_chatModel;
    bool m_responseInProgress = false;
    ResponseState m_responseState;
    qint64 m_creationDate;
    ChatLLM *m_llmodel;
    QList<ResultInfo> m_databaseResults;
    bool m_isServer = false;
    bool m_shouldDeleteLater = false;
    bool m_isModelLoaded = false;
    bool m_shouldLoadModelWhenInstalled = false;
    LocalDocsCollectionsModel *m_collectionModel;
};

#endif // CHAT_H
