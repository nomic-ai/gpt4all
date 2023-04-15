#ifndef LLM_H
#define LLM_H

#include <QObject>
#include <QThread>
#include "gptj.h"
#include "llamamodel.h"

class LLMObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<QString> modelList READ modelList NOTIFY modelListChanged)
    Q_PROPERTY(bool isModelLoaded READ isModelLoaded NOTIFY isModelLoadedChanged)
    Q_PROPERTY(QString response READ response NOTIFY responseChanged)
    Q_PROPERTY(QString modelName READ modelName WRITE setModelName NOTIFY modelNameChanged)
    Q_PROPERTY(QString modelName READ modelName NOTIFY modelNameChanged)
    Q_PROPERTY(int32_t threadCount READ threadCount WRITE setThreadCount NOTIFY threadCountChanged)

public:

    LLMObject();

    bool isModelLoaded() const;
    void regenerateResponse();
    void resetResponse();
    void resetContext();
    void stopGenerating() { m_stopGenerating = true; }
    void setThreadCount(int32_t n_threads);
    int32_t threadCount();

    QString response() const;
    QString modelName() const;

    QList<QString> modelList() const;
    void setModelName(const QString &modelName);

public Q_SLOTS:
    bool prompt(const QString &prompt, const QString &prompt_template, int32_t n_predict, int32_t top_k, float top_p,
                float temp, int32_t n_batch);
    bool loadModel();
    void modelNameChangeRequested(const QString &modelName);

Q_SIGNALS:
    void isModelLoadedChanged();
    void responseChanged();
    void responseStarted();
    void responseStopped();
    void modelNameChanged();
    void modelListChanged();
    void threadCountChanged();

private:
    bool loadModelPrivate(const QString &modelName);
    bool handleResponse(const std::string &response);

private:
    LLModel *m_llmodel;
    std::string m_response;
    quint32 m_responseTokens;
    quint32 m_responseLogits;
    QString m_modelName;
    QThread m_llmThread;
    std::atomic<bool> m_stopGenerating;
};

class LLM : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<QString> modelList READ modelList NOTIFY modelListChanged)
    Q_PROPERTY(bool isModelLoaded READ isModelLoaded NOTIFY isModelLoadedChanged)
    Q_PROPERTY(QString response READ response NOTIFY responseChanged)
    Q_PROPERTY(QString modelName READ modelName WRITE setModelName NOTIFY modelNameChanged)
    Q_PROPERTY(bool responseInProgress READ responseInProgress NOTIFY responseInProgressChanged)
    Q_PROPERTY(int32_t threadCount READ threadCount WRITE setThreadCount NOTIFY threadCountChanged)
public:

    static LLM *globalInstance();

    Q_INVOKABLE bool isModelLoaded() const;
    Q_INVOKABLE void prompt(const QString &prompt, const QString &prompt_template, int32_t n_predict, int32_t top_k, float top_p,
                            float temp, int32_t n_batch);
    Q_INVOKABLE void regenerateResponse();
    Q_INVOKABLE void resetResponse();
    Q_INVOKABLE void resetContext();
    Q_INVOKABLE void stopGenerating();
    Q_INVOKABLE void setThreadCount(int32_t n_threads);
    Q_INVOKABLE int32_t threadCount();

    QString response() const;
    bool responseInProgress() const { return m_responseInProgress; }

    QList<QString> modelList() const;

    QString modelName() const;
    void setModelName(const QString &modelName);

    Q_INVOKABLE bool checkForUpdates() const;

Q_SIGNALS:
    void isModelLoadedChanged();
    void responseChanged();
    void responseInProgressChanged();
    void promptRequested(const QString &prompt, const QString &prompt_template, int32_t n_predict, int32_t top_k, float top_p,
                         float temp, int32_t n_batch);
    void regenerateResponseRequested();
    void resetResponseRequested();
    void resetContextRequested();
    void modelNameChangeRequested(const QString &modelName);
    void modelNameChanged();
    void modelListChanged();
    void threadCountChanged();
    void setThreadCountRequested(int32_t threadCount);

private Q_SLOTS:
    void responseStarted();
    void responseStopped();

private:
    LLMObject *m_llmodel;
    bool m_responseInProgress;

private:
    explicit LLM();
    ~LLM() {}
    friend class MyLLM;
};

#endif // LLM_H
