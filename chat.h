#ifndef CHAT_H
#define CHAT_H

#include <QObject>
#include <QtQml>

#include "chatllm.h"
#include "chatmodel.h"

class Chat : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ id NOTIFY idChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(ChatModel *chatModel READ chatModel NOTIFY chatModelChanged)
    Q_PROPERTY(bool isModelLoaded READ isModelLoaded NOTIFY isModelLoadedChanged)
    Q_PROPERTY(QString response READ response NOTIFY responseChanged)
    Q_PROPERTY(QString modelName READ modelName WRITE setModelName NOTIFY modelNameChanged)
    Q_PROPERTY(bool responseInProgress READ responseInProgress NOTIFY responseInProgressChanged)
    Q_PROPERTY(int32_t threadCount READ threadCount WRITE setThreadCount NOTIFY threadCountChanged)
    Q_PROPERTY(bool isRecalc READ isRecalc NOTIFY recalcChanged)
    QML_ELEMENT
    QML_UNCREATABLE("Only creatable from c++!")

public:
    explicit Chat(QObject *parent = nullptr);

    QString id() const { return m_id; }
    QString name() const { return m_name; }
    ChatModel *chatModel() { return m_chatModel; }

    Q_INVOKABLE void reset();
    Q_INVOKABLE bool isModelLoaded() const;
    Q_INVOKABLE void prompt(const QString &prompt, const QString &prompt_template, int32_t n_predict, int32_t top_k, float top_p,
                            float temp, int32_t n_batch, float repeat_penalty, int32_t repeat_penalty_tokens);
    Q_INVOKABLE void regenerateResponse();
    Q_INVOKABLE void resetResponse();
    Q_INVOKABLE void resetContext();
    Q_INVOKABLE void stopGenerating();
    Q_INVOKABLE void syncThreadCount();
    Q_INVOKABLE void setThreadCount(int32_t n_threads);
    Q_INVOKABLE int32_t threadCount();

    QString response() const;
    bool responseInProgress() const { return m_responseInProgress; }
    QString modelName() const;
    void setModelName(const QString &modelName);
    bool isRecalc() const;

Q_SIGNALS:
    void idChanged();
    void nameChanged();
    void chatModelChanged();
    void isModelLoadedChanged();
    void responseChanged();
    void responseInProgressChanged();
    void promptRequested(const QString &prompt, const QString &prompt_template, int32_t n_predict, int32_t top_k, float top_p,
                         float temp, int32_t n_batch, float repeat_penalty, int32_t repeat_penalty_tokens);
    void regenerateResponseRequested();
    void resetResponseRequested();
    void resetContextRequested();
    void modelNameChangeRequested(const QString &modelName);
    void modelNameChanged();
    void threadCountChanged();
    void setThreadCountRequested(int32_t threadCount);
    void recalcChanged();

private Q_SLOTS:
    void responseStarted();
    void responseStopped();

private:
    ChatLLM *m_llmodel;
    QString m_id;
    QString m_name;
    ChatModel *m_chatModel;
    bool m_responseInProgress;
    int32_t m_desiredThreadCount;
};

#endif // CHAT_H
