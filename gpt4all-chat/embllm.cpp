#include "embllm.h"
#include "modellist.h"

EmbeddingLLMWorker::EmbeddingLLMWorker()
    : QObject(nullptr)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_model(nullptr)
{
    moveToThread(&m_workerThread);
    connect(this, &EmbeddingLLMWorker::finished, &m_workerThread, &QThread::quit, Qt::DirectConnection);
    m_workerThread.setObjectName("embedding");
    m_workerThread.start();
}

EmbeddingLLMWorker::~EmbeddingLLMWorker()
{
    if (m_model) {
        delete m_model;
        m_model = nullptr;
    }
}

void EmbeddingLLMWorker::wait()
{
    m_workerThread.wait();
}

bool EmbeddingLLMWorker::loadModel()
{
    const EmbeddingModels *embeddingModels = ModelList::globalInstance()->installedEmbeddingModels();
    if (!embeddingModels->count())
        return false;

    const ModelInfo defaultModel = embeddingModels->defaultModelInfo();

    QString filePath = defaultModel.dirpath + defaultModel.filename();
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        qWarning() << "WARNING: Could not load sbert because file does not exist";
        m_model = nullptr;
        return false;
    }

    auto filename = fileInfo.fileName();
    bool isNomic = filename.startsWith("nomic-") && filename.endsWith(".txt");
    if (isNomic) {
        QFile file(filePath);
        file.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text);
        QTextStream stream(&file);
        m_nomicAPIKey = stream.readAll();
        file.close();
        return true;
    }

    m_model = LLModel::Implementation::construct(filePath.toStdString());
    // NOTE: explicitly loads model on CPU to avoid GPU OOM
    // TODO(cebtenzzre): support GPU-accelerated embeddings
    bool success = m_model->loadModel(filePath.toStdString(), 2048, 0);
    if (!success) {
        qWarning() << "WARNING: Could not load embedding model";
        delete m_model;
        m_model = nullptr;
        return false;
    }

    if (!m_model->supportsEmbedding()) {
        qWarning() << "WARNING: Model type does not support embeddings";
        delete m_model;
        m_model = nullptr;
        return false;
    }
    return true;
}

bool EmbeddingLLMWorker::hasModel() const
{
    return m_model || !m_nomicAPIKey.isEmpty();
}

bool EmbeddingLLMWorker::isNomic() const
{
    return !m_nomicAPIKey.isEmpty();
}

// this function is always called for retrieval tasks
std::vector<float> EmbeddingLLMWorker::generateSyncEmbedding(const QString &text)
{
    if (!hasModel() && !loadModel()) {
        qWarning() << "WARNING: Could not load model for embeddings";
        return {};
    }

    if (isNomic()) {
        qWarning() << "WARNING: Request to generate sync embeddings for non-local model invalid";
        return {};
    }

    std::vector<float> embedding(m_model->embeddingSize());
    try {
        m_model->embed({text.toStdString()}, embedding.data(), true);
    } catch (const std::exception &e) {
        qWarning() << "WARNING: LLModel::embed failed: " << e.what();
        return {};
    }
    return embedding;
}

void EmbeddingLLMWorker::sendAtlasRequest(const QStringList &texts, const QString &taskType, QVariant userData) {
    QJsonObject root;
    root.insert("model", "nomic-embed-text-v1");
    root.insert("texts", QJsonArray::fromStringList(texts));
    root.insert("task_type", taskType);

    QJsonDocument doc(root);

    QUrl nomicUrl("https://api-atlas.nomic.ai/v1/embedding/text");
    const QString authorization = QString("Bearer %1").arg(m_nomicAPIKey).trimmed();
    QNetworkRequest request(nomicUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", authorization.toUtf8());
    request.setAttribute(QNetworkRequest::User, userData);
    QNetworkReply *reply = m_networkManager->post(request, doc.toJson(QJsonDocument::Compact));
    connect(qApp, &QCoreApplication::aboutToQuit, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::finished, this, &EmbeddingLLMWorker::handleFinished);
}

// this function is always called for retrieval tasks
void EmbeddingLLMWorker::requestSyncEmbedding(const QString &text)
{
    if (!hasModel() && !loadModel()) {
        qWarning() << "WARNING: Could not load model for embeddings";
        return;
    }

    if (!isNomic()) {
        qWarning() << "WARNING: Request to generate sync embeddings for local model invalid";
        return;
    }

    Q_ASSERT(hasModel());

    sendAtlasRequest({text}, "search_query");
}

// this function is always called for storage into the database
void EmbeddingLLMWorker::requestAsyncEmbedding(const QVector<EmbeddingChunk> &chunks)
{
    if (!hasModel() && !loadModel()) {
        qWarning() << "WARNING: Could not load model for embeddings";
        return;
    }

    if (m_nomicAPIKey.isEmpty()) {
        QVector<EmbeddingResult> results;
        results.reserve(chunks.size());
        for (auto c : chunks) {
            EmbeddingResult result;
            result.folder_id = c.folder_id;
            result.chunk_id = c.chunk_id;
            // TODO(cebtenzzre): take advantage of batched embeddings
            result.embedding.resize(m_model->embeddingSize());
            try {
                m_model->embed({c.chunk.toStdString()}, result.embedding.data(), false);
            } catch (const std::exception &e) {
                qWarning() << "WARNING: LLModel::embed failed:" << e.what();
                return;
            }
            results << result;
        }
        emit embeddingsGenerated(results);
        return;
    };

    QStringList texts;
    for (auto &c: chunks)
        texts.append(c.chunk);
    sendAtlasRequest(texts, "search_document", QVariant::fromValue(chunks));
}

std::vector<float> jsonArrayToVector(const QJsonArray &jsonArray) {
    std::vector<float> result;

    for (const QJsonValue &innerValue : jsonArray) {
        if (innerValue.isArray()) {
            QJsonArray innerArray = innerValue.toArray();
            result.reserve(result.size() + innerArray.size());
            for (const QJsonValue &value : innerArray) {
                result.push_back(static_cast<float>(value.toDouble()));
            }
        }
    }

    return result;
}

QVector<EmbeddingResult> jsonArrayToEmbeddingResults(const QVector<EmbeddingChunk>& chunks, const QJsonArray& embeddings) {
    QVector<EmbeddingResult> results;

    if (chunks.size() != embeddings.size()) {
        qWarning() << "WARNING: Size of json array result does not match input!";
        return results;
    }

    for (int i = 0; i < chunks.size(); ++i) {
        const EmbeddingChunk& chunk = chunks.at(i);
        const QJsonArray embeddingArray = embeddings.at(i).toArray();

        std::vector<float> embeddingVector;
        for (const QJsonValue& value : embeddingArray)
            embeddingVector.push_back(static_cast<float>(value.toDouble()));

        EmbeddingResult result;
        result.folder_id = chunk.folder_id;
        result.chunk_id = chunk.chunk_id;
        result.embedding = std::move(embeddingVector);
        results.push_back(std::move(result));
    }

    return results;
}

void EmbeddingLLMWorker::handleFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;

    QVariant retrievedData = reply->request().attribute(QNetworkRequest::User);
    QVector<EmbeddingChunk> chunks;
    if (retrievedData.isValid() && retrievedData.canConvert<QVector<EmbeddingChunk>>())
        chunks = retrievedData.value<QVector<EmbeddingChunk>>();

    int folder_id = 0;
    if (!chunks.isEmpty())
        folder_id = chunks.first().folder_id;

    QVariant response = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    Q_ASSERT(response.isValid());
    bool ok;
    int code = response.toInt(&ok);
    if (!ok || code != 200) {
        QString errorDetails;
        QString replyErrorString = reply->errorString().trimmed();
        QByteArray replyContent = reply->readAll().trimmed();
        errorDetails = QString("ERROR: Nomic Atlas responded with error code \"%1\"").arg(code);
        if (!replyErrorString.isEmpty())
            errorDetails += QString(". Error Details: \"%1\"").arg(replyErrorString);
        if (!replyContent.isEmpty())
            errorDetails += QString(". Response Content: \"%1\"").arg(QString::fromUtf8(replyContent));
        qWarning() << errorDetails;
        emit errorGenerated(folder_id, errorDetails);
        return;
    }

    QByteArray jsonData = reply->readAll();

    QJsonParseError err;
    QJsonDocument document = QJsonDocument::fromJson(jsonData, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "ERROR: Couldn't parse Nomic Atlas response: " << jsonData << err.errorString();
        return;
    }

    const QJsonObject root = document.object();
    const QJsonArray embeddings = root.value("embeddings").toArray();

    if (!chunks.isEmpty()) {
        emit embeddingsGenerated(jsonArrayToEmbeddingResults(chunks, embeddings));
    } else {
        m_lastResponse = jsonArrayToVector(embeddings);
        emit finished();
    }

    reply->deleteLater();
}

EmbeddingLLM::EmbeddingLLM()
    : QObject(nullptr)
    , m_embeddingWorker(new EmbeddingLLMWorker)
{
    connect(this, &EmbeddingLLM::requestAsyncEmbedding, m_embeddingWorker,
        &EmbeddingLLMWorker::requestAsyncEmbedding, Qt::QueuedConnection);
    connect(m_embeddingWorker, &EmbeddingLLMWorker::embeddingsGenerated, this,
        &EmbeddingLLM::embeddingsGenerated, Qt::QueuedConnection);
    connect(m_embeddingWorker, &EmbeddingLLMWorker::errorGenerated, this,
        &EmbeddingLLM::errorGenerated, Qt::QueuedConnection);
}

EmbeddingLLM::~EmbeddingLLM()
{
    delete m_embeddingWorker;
    m_embeddingWorker = nullptr;
}

std::vector<float> EmbeddingLLM::generateEmbeddings(const QString &text)
{
    if (!m_embeddingWorker->isNomic()) {
        return m_embeddingWorker->generateSyncEmbedding(text);
    } else {
        EmbeddingLLMWorker worker;
        connect(this, &EmbeddingLLM::requestSyncEmbedding, &worker,
            &EmbeddingLLMWorker::requestSyncEmbedding, Qt::QueuedConnection);
        emit requestSyncEmbedding(text);
        worker.wait();
        return worker.lastResponse();
    }
}

void EmbeddingLLM::generateAsyncEmbeddings(const QVector<EmbeddingChunk> &chunks)
{
    emit requestAsyncEmbedding(chunks);
}
