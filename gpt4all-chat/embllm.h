#ifndef EMBLLM_H
#define EMBLLM_H

#include <QByteArray>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QVariant>
#include <QVector>

#include <atomic>
#include <vector>

class LLModel;
class QNetworkAccessManager;

struct EmbeddingChunk {
    QString model; // TODO(jared): use to select model
    int folder_id;
    int chunk_id;
    QString chunk;
};

Q_DECLARE_METATYPE(EmbeddingChunk)

struct EmbeddingResult {
    QString model;
    int folder_id;
    int chunk_id;
    std::vector<float> embedding;
};

class EmbeddingLLMWorker : public QObject {
    Q_OBJECT
public:
    EmbeddingLLMWorker();
    ~EmbeddingLLMWorker() override;

    void wait();

    std::vector<float> lastResponse() const { return m_lastResponse; }

    bool loadModel();
    bool isNomic() const { return !m_nomicAPIKey.isEmpty(); }
    bool hasModel() const { return isNomic() || m_model; }

    std::vector<float> generateQueryEmbedding(const QString &text);

public Q_SLOTS:
    void atlasQueryEmbeddingRequested(const QString &text);
    void docEmbeddingsRequested(const QVector<EmbeddingChunk> &chunks);

Q_SIGNALS:
    void requestAtlasQueryEmbedding(const QString &text);
    void embeddingsGenerated(const QVector<EmbeddingResult> &embeddings);
    void errorGenerated(const QVector<EmbeddingChunk> &chunks, const QString &error);
    void finished();

private Q_SLOTS:
    void handleFinished();

private:
    void sendAtlasRequest(const QStringList &texts, const QString &taskType, const QVariant &userData = {});

    QString m_nomicAPIKey;
    QNetworkAccessManager *m_networkManager;
    std::vector<float> m_lastResponse;
    LLModel *m_model = nullptr;
    std::atomic<bool> m_stopGenerating;
    QThread m_workerThread;
    QMutex m_mutex; // guards m_model and m_nomicAPIKey
};

class EmbeddingLLM : public QObject
{
    Q_OBJECT
public:
    EmbeddingLLM();
    ~EmbeddingLLM() override;

    static QString model();
    bool loadModel();
    bool hasModel() const;

public Q_SLOTS:
    std::vector<float> generateQueryEmbedding(const QString &text); // synchronous
    void generateDocEmbeddingsAsync(const QVector<EmbeddingChunk> &chunks);

Q_SIGNALS:
    void requestDocEmbeddings(const QVector<EmbeddingChunk> &chunks);
    void embeddingsGenerated(const QVector<EmbeddingResult> &embeddings);
    void errorGenerated(const QVector<EmbeddingChunk> &chunks, const QString &error);

private:
    EmbeddingLLMWorker *m_embeddingWorker;
};

#endif // EMBLLM_H
