#ifndef CHAT_H
#define CHAT_H

#include <QObject>
#include <QtQml>
#include <QDataStream>

#include "chatllm.h"
#include "chatmodel.h"
#include "database.h"

class Chat : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ id NOTIFY idChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(ChatModel *chatModel READ chatModel NOTIFY chatModelChanged)
    Q_PROPERTY(bool isModelLoaded READ isModelLoaded NOTIFY isModelLoadedChanged)
    Q_PROPERTY(QString response READ response NOTIFY responseChanged)
    Q_PROPERTY(QString modelName READ modelName WRITE setModelName NOTIFY modelNameChanged)
    Q_PROPERTY(bool responseInProgress READ responseInProgress NOTIFY responseInProgressChanged)
    Q_PROPERTY(bool isRecalc READ isRecalc NOTIFY recalcChanged)
    Q_PROPERTY(QList<QString> modelList READ modelList NOTIFY modelListChanged)
    Q_PROPERTY(bool isServer READ isServer NOTIFY isServerChanged)
    Q_PROPERTY(QString responseState READ responseState NOTIFY responseStateChanged)
    Q_PROPERTY(QList<QString> collectionList READ collectionList NOTIFY collectionListChanged)
    Q_PROPERTY(QString modelLoadingError READ modelLoadingError NOTIFY modelLoadingErrorChanged)
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

    Q_INVOKABLE void reset();
    Q_INVOKABLE bool isModelLoaded() const;
    Q_INVOKABLE void prompt(const QString &prompt, const QString &prompt_template, int32_t n_predict,
        int32_t top_k, float top_p, float temp, int32_t n_batch, float repeat_penalty, int32_t repeat_penalty_tokens);
    Q_INVOKABLE void regenerateResponse();
    Q_INVOKABLE void stopGenerating();
    Q_INVOKABLE void newPromptResponsePair(const QString &prompt);

    QList<ResultInfo> databaseResults() const;

    QString response() const;
    bool responseInProgress() const { return m_responseInProgress; }
    QString responseState() const;
    QString modelName() const;
    void setModelName(const QString &modelName);
    bool isRecalc() const;

    void loadDefaultModel();
    void loadModel(const QString &modelName);
    void unloadModel();
    void reloadModel();
    void unloadAndDeleteLater();

    qint64 creationDate() const { return m_creationDate; }
    bool serialize(QDataStream &stream, int version) const;
    bool deserialize(QDataStream &stream, int version);

    QList<QString> modelList() const;
    bool isServer() const { return m_isServer; }

    QList<QString> collectionList() const;

    Q_INVOKABLE bool hasCollection(const QString &collection) const;
    Q_INVOKABLE void addCollection(const QString &collection);
    Q_INVOKABLE void removeCollection(const QString &collection);
    void resetResponseState();

    QString modelLoadingError() const { return m_modelLoadingError; }

public Q_SLOTS:
    void serverNewPromptResponsePair(const QString &prompt);

Q_SIGNALS:
    void idChanged();
    void nameChanged();
    void chatModelChanged();
    void isModelLoadedChanged();
    void responseChanged();
    void responseInProgressChanged();
    void responseStateChanged();
    void promptRequested(const QString &prompt, const QString &prompt_template, int32_t n_predict,
        int32_t top_k, float top_p, float temp, int32_t n_batch, float repeat_penalty, int32_t repeat_penalty_tokens,
        int32_t n_threads);
    void regenerateResponseRequested();
    void resetResponseRequested();
    void resetContextRequested();
    void modelNameChangeRequested(const QString &modelName);
    void modelNameChanged();
    void recalcChanged();
    void loadDefaultModelRequested();
    void loadModelRequested(const QString &modelName);
    void generateNameRequested();
    void modelListChanged();
    void modelLoadingErrorChanged();
    void isServerChanged();
    void collectionListChanged();

private Q_SLOTS:
    void handleResponseChanged();
    void handleModelLoadedChanged();
    void promptProcessing();
    void responseStopped();
    void generatedNameChanged();
    void handleRecalculating();
    void handleModelNameChanged();
    void handleModelLoadingError(const QString &error);

private:
    QString m_id;
    QString m_name;
    QString m_userName;
    QString m_savedModelName;
    QString m_modelLoadingError;
    QList<QString> m_collections;
    ChatModel *m_chatModel;
    bool m_responseInProgress;
    ResponseState m_responseState;
    qint64 m_creationDate;
    ChatLLM *m_llmodel;
    bool m_isServer;
    bool m_shouldDeleteLater;
};

#endif // CHAT_H
