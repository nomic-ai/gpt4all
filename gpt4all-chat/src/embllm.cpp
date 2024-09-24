#include "embllm.h"

#include "modellist.h"
#include "mysettings.h"

#include <gpt4all-backend/llmodel.h>

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QMutexLocker>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <Qt>
#include <QtGlobal>
#include <QtLogging>

#include <exception>
#include <utility>
#include <vector>

using namespace Qt::Literals::StringLiterals;

static const QString EMBEDDING_MODEL_NAME = u"nomic-embed-text-v1.5"_s;
static const QString LOCAL_EMBEDDING_MODEL = u"nomic-embed-text-v1.5.f16.gguf"_s;

EmbeddingLLMWorker::EmbeddingLLMWorker()
    : QObject(nullptr)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_stopGenerating(false)
{
    moveToThread(&m_workerThread);
    connect(this, &EmbeddingLLMWorker::requestAtlasQueryEmbedding, this, &EmbeddingLLMWorker::atlasQueryEmbeddingRequested);
    connect(this, &EmbeddingLLMWorker::finished, &m_workerThread, &QThread::quit, Qt::DirectConnection);
    m_workerThread.setObjectName("embedding");
    m_workerThread.start();
}

EmbeddingLLMWorker::~EmbeddingLLMWorker()
{
    m_stopGenerating = true;
    m_workerThread.quit();
    m_workerThread.wait();

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
    constexpr int n_ctx = 2048;

    m_nomicAPIKey.clear();
    m_model = nullptr;

    // TODO(jared): react to setting changes without restarting

    if (MySettings::globalInstance()->localDocsUseRemoteEmbed()) {
        m_nomicAPIKey = MySettings::globalInstance()->localDocsNomicAPIKey();
        return true;
    }

#ifdef Q_OS_DARWIN
    static const QString embPathFmt = u"%1/../Resources/%2"_s;
#else
    static const QString embPathFmt = u"%1/../resources/%2"_s;
#endif

    QString filePath = embPathFmt.arg(QCoreApplication::applicationDirPath(), LOCAL_EMBEDDING_MODEL);
    if (!QFileInfo::exists(filePath)) {
        qWarning() << "embllm WARNING: Local embedding model not found";
        return false;
    }

    QString requestedDevice = MySettings::globalInstance()->localDocsEmbedDevice();
    std::string backend = "auto";
#ifdef Q_OS_MAC
    if (requestedDevice == "Auto" || requestedDevice == "CPU")
        backend = "cpu";
#else
    if (requestedDevice.startsWith("CUDA: "))
        backend = "cuda";
#endif

    try {
        m_model = LLModel::Implementation::construct(filePath.toStdString(), backend, n_ctx);
    } catch (const std::exception &e) {
        qWarning() << "embllm WARNING: Could not load embedding model:" << e.what();
        return false;
    }

    bool actualDeviceIsCPU = true;

#if defined(Q_OS_MAC) && defined(__aarch64__)
    if (m_model->implementation().buildVariant() == "metal")
        actualDeviceIsCPU = false;
#else
    if (requestedDevice != "CPU") {
        const LLModel::GPUDevice *device = nullptr;
        std::vector<LLModel::GPUDevice> availableDevices = m_model->availableGPUDevices(0);
        if (requestedDevice != "Auto") {
            // Use the selected device
            for (const LLModel::GPUDevice &d : availableDevices) {
                if (QString::fromStdString(d.selectionName()) == requestedDevice) {
                    device = &d;
                    break;
                }
            }
        }

        std::string unavail_reason;
        if (!device) {
            // GPU not available
        } else if (!m_model->initializeGPUDevice(device->index, &unavail_reason)) {
            qWarning().noquote() << "embllm WARNING: Did not use GPU:" << QString::fromStdString(unavail_reason);
        } else {
            actualDeviceIsCPU = false;
        }
    }
#endif

    bool success = m_model->loadModel(filePath.toStdString(), n_ctx, 100);

    // CPU fallback
    if (!actualDeviceIsCPU && !success) {
        // llama_init_from_file returned nullptr
        qWarning() << "embllm WARNING: Did not use GPU: GPU loading failed (out of VRAM?)";

        if (backend == "cuda") {
            // For CUDA, make sure we don't use the GPU at all - ngl=0 still offloads matmuls
            try {
                m_model = LLModel::Implementation::construct(filePath.toStdString(), "auto", n_ctx);
            } catch (const std::exception &e) {
                qWarning() << "embllm WARNING: Could not load embedding model:" << e.what();
                return false;
            }
        }

        success = m_model->loadModel(filePath.toStdString(), n_ctx, 0);
    }

    if (!success) {
        qWarning() << "embllm WARNING: Could not load embedding model";
        delete m_model;
        m_model = nullptr;
        return false;
    }

    if (!m_model->supportsEmbedding()) {
        qWarning() << "embllm WARNING: Model type does not support embeddings";
        delete m_model;
        m_model = nullptr;
        return false;
    }

    // FIXME(jared): the user may want this to take effect without having to restart
    int n_threads = MySettings::globalInstance()->threadCount();
    m_model->setThreadCount(n_threads);

    return true;
}

std::vector<float> EmbeddingLLMWorker::generateQueryEmbedding(const QString &text)
{
    {
        QMutexLocker locker(&m_mutex);

        if (!hasModel() && !loadModel()) {
            qWarning() << "WARNING: Could not load model for embeddings";
            return {};
        }

        if (!isNomic()) {
            std::vector<float> embedding(m_model->embeddingSize());

            try {
                m_model->embed({text.toStdString()}, embedding.data(), /*isRetrieval*/ true);
            } catch (const std::exception &e) {
                qWarning() << "WARNING: LLModel::embed failed:" << e.what();
                return {};
            }

            return embedding;
        }
    }

    EmbeddingLLMWorker worker;
    emit worker.requestAtlasQueryEmbedding(text);
    worker.wait();
    return worker.lastResponse();
}

void EmbeddingLLMWorker::sendAtlasRequest(const QStringList &texts, const QString &taskType, const QVariant &userData)
{
    QJsonObject root;
    root.insert("model", "nomic-embed-text-v1");
    root.insert("texts", QJsonArray::fromStringList(texts));
    root.insert("task_type", taskType);

    QJsonDocument doc(root);

    QUrl nomicUrl("https://api-atlas.nomic.ai/v1/embedding/text");
    const QString authorization = u"Bearer %1"_s.arg(m_nomicAPIKey).trimmed();
    QNetworkRequest request(nomicUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", authorization.toUtf8());
    request.setAttribute(QNetworkRequest::User, userData);
    QNetworkReply *reply = m_networkManager->post(request, doc.toJson(QJsonDocument::Compact));
    connect(qGuiApp, &QCoreApplication::aboutToQuit, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::finished, this, &EmbeddingLLMWorker::handleFinished);
}

void EmbeddingLLMWorker::atlasQueryEmbeddingRequested(const QString &text)
{
    {
        QMutexLocker locker(&m_mutex);
        if (!hasModel() && !loadModel()) {
            qWarning() << "WARNING: Could not load model for embeddings";
            return;
        }

        if (!isNomic()) {
            qWarning() << "WARNING: Request to generate sync embeddings for local model invalid";
            return;
        }

        Q_ASSERT(hasModel());
    }

    sendAtlasRequest({text}, "search_query");
}

void EmbeddingLLMWorker::docEmbeddingsRequested(const QVector<EmbeddingChunk> &chunks)
{
    if (m_stopGenerating)
        return;

    bool isNomic;
    {
        QMutexLocker locker(&m_mutex);
        if (!hasModel() && !loadModel()) {
            qWarning() << "WARNING: Could not load model for embeddings";
            return;
        }

        isNomic = this->isNomic();
    }

    if (!isNomic) {
        QVector<EmbeddingResult> results;
        results.reserve(chunks.size());
        std::vector<std::string> texts;
        texts.reserve(chunks.size());
        for (const auto &c: chunks) {
            EmbeddingResult result;
            result.model = c.model;
            result.folder_id = c.folder_id;
            result.chunk_id = c.chunk_id;
            result.embedding.resize(m_model->embeddingSize());
            results << result;
            texts.push_back(c.chunk.toStdString());
        }

        constexpr int BATCH_SIZE = 4;
        std::vector<float> result;
        result.resize(chunks.size() * m_model->embeddingSize());
        for (int j = 0; j < chunks.size(); j += BATCH_SIZE) {
            QMutexLocker locker(&m_mutex);
            std::vector batchTexts(texts.begin() + j, texts.begin() + std::min(j + BATCH_SIZE, int(texts.size())));
            try {
                m_model->embed(batchTexts, result.data() + j * m_model->embeddingSize(), /*isRetrieval*/ false);
            } catch (const std::exception &e) {
                qWarning() << "WARNING: LLModel::embed failed:" << e.what();
                return;
            }
        }
        for (int i = 0; i < chunks.size(); i++)
            memcpy(results[i].embedding.data(), &result[i * m_model->embeddingSize()], m_model->embeddingSize() * sizeof(float));

        emit embeddingsGenerated(results);
        return;
    };

    QStringList texts;
    for (auto &c: chunks)
        texts.append(c.chunk);
    sendAtlasRequest(texts, "search_document", QVariant::fromValue(chunks));
}

std::vector<float> jsonArrayToVector(const QJsonArray &jsonArray)
{
    std::vector<float> result;

    for (const auto &innerValue: jsonArray) {
        if (innerValue.isArray()) {
            QJsonArray innerArray = innerValue.toArray();
            result.reserve(result.size() + innerArray.size());
            for (const auto &value: innerArray) {
                result.push_back(static_cast<float>(value.toDouble()));
            }
        }
    }

    return result;
}

QVector<EmbeddingResult> jsonArrayToEmbeddingResults(const QVector<EmbeddingChunk>& chunks, const QJsonArray& embeddings)
{
    QVector<EmbeddingResult> results;

    if (chunks.size() != embeddings.size()) {
        qWarning() << "WARNING: Size of json array result does not match input!";
        return results;
    }

    for (int i = 0; i < chunks.size(); ++i) {
        const EmbeddingChunk& chunk = chunks.at(i);
        const QJsonArray embeddingArray = embeddings.at(i).toArray();

        std::vector<float> embeddingVector;
        for (const auto &value: embeddingArray)
            embeddingVector.push_back(static_cast<float>(value.toDouble()));

        EmbeddingResult result;
        result.model = chunk.model;
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

    QVariant response = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    Q_ASSERT(response.isValid());
    bool ok;
    int code = response.toInt(&ok);
    if (!ok || code != 200) {
        QString errorDetails;
        QString replyErrorString = reply->errorString().trimmed();
        QByteArray replyContent = reply->readAll().trimmed();
        errorDetails = u"ERROR: Nomic Atlas responded with error code \"%1\""_s.arg(code);
        if (!replyErrorString.isEmpty())
            errorDetails += u". Error Details: \"%1\""_s.arg(replyErrorString);
        if (!replyContent.isEmpty())
            errorDetails += u". Response Content: \"%1\""_s.arg(QString::fromUtf8(replyContent));
        qWarning() << errorDetails;
        emit errorGenerated(chunks, errorDetails);
        return;
    }

    QByteArray jsonData = reply->readAll();

    QJsonParseError err;
    QJsonDocument document = QJsonDocument::fromJson(jsonData, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "ERROR: Couldn't parse Nomic Atlas response:" << jsonData << err.errorString();
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
    connect(this, &EmbeddingLLM::requestDocEmbeddings, m_embeddingWorker,
        &EmbeddingLLMWorker::docEmbeddingsRequested, Qt::QueuedConnection);
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

QString EmbeddingLLM::model()
{
    return EMBEDDING_MODEL_NAME;
}

// TODO(jared): embed using all necessary embedding models given collection
std::vector<float> EmbeddingLLM::generateQueryEmbedding(const QString &text)
{
    return m_embeddingWorker->generateQueryEmbedding(text);
}

void EmbeddingLLM::generateDocEmbeddingsAsync(const QVector<EmbeddingChunk> &chunks)
{
    emit requestDocEmbeddings(chunks);
}
