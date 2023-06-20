#include "chatllm.h"
#include "chat.h"
#include "download.h"
#include "network.h"
#include "../gpt4all-backend/llmodel.h"
#include "chatgpt.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QResource>
#include <QSettings>

//#define DEBUG
//#define DEBUG_MODEL_LOADING

#define MPT_INTERNAL_STATE_VERSION 0
#define GPTJ_INTERNAL_STATE_VERSION 0
#define REPLIT_INTERNAL_STATE_VERSION 0
#define LLAMA_INTERNAL_STATE_VERSION 0

static QString modelFilePath(const QString &modelName, bool isChatGPT)
{
    QVector<QString> possibleFilePaths;
    if (isChatGPT)
        possibleFilePaths << "/" + modelName + ".txt";
    else {
        possibleFilePaths << "/ggml-" + modelName + ".bin";
        possibleFilePaths << "/" + modelName + ".bin";
    }
    for (const QString &modelFilename : possibleFilePaths) {
        QString appPath = QCoreApplication::applicationDirPath() + modelFilename;
        QFileInfo infoAppPath(appPath);
        if (infoAppPath.exists())
            return appPath;

        QString downloadPath = Download::globalInstance()->downloadLocalModelsPath() + modelFilename;
        QFileInfo infoLocalPath(downloadPath);
        if (infoLocalPath.exists())
            return downloadPath;
    }
    return QString();
}

class LLModelStore {
public:
    static LLModelStore *globalInstance();

    LLModelInfo acquireModel(); // will block until llmodel is ready
    void releaseModel(const LLModelInfo &info); // must be called when you are done

private:
    LLModelStore()
    {
        // seed with empty model
        m_availableModels.append(LLModelInfo());
    }
    ~LLModelStore() {}
    QVector<LLModelInfo> m_availableModels;
    QMutex m_mutex;
    QWaitCondition m_condition;
    friend class MyLLModelStore;
};

class MyLLModelStore : public LLModelStore { };
Q_GLOBAL_STATIC(MyLLModelStore, storeInstance)
LLModelStore *LLModelStore::globalInstance()
{
    return storeInstance();
}

LLModelInfo LLModelStore::acquireModel()
{
    QMutexLocker locker(&m_mutex);
    while (m_availableModels.isEmpty())
        m_condition.wait(locker.mutex());
    return m_availableModels.takeFirst();
}

void LLModelStore::releaseModel(const LLModelInfo &info)
{
    QMutexLocker locker(&m_mutex);
    m_availableModels.append(info);
    Q_ASSERT(m_availableModels.count() < 2);
    m_condition.wakeAll();
}

ChatLLM::ChatLLM(Chat *parent, bool isServer)
    : QObject{nullptr}
    , m_promptResponseTokens(0)
    , m_promptTokens(0)
    , m_isRecalc(false)
    , m_shouldBeLoaded(true)
    , m_stopGenerating(false)
    , m_timer(nullptr)
    , m_isServer(isServer)
{
    moveToThread(&m_llmThread);
    connect(this, &ChatLLM::sendStartup, Network::globalInstance(), &Network::sendStartup);
    connect(this, &ChatLLM::sendModelLoaded, Network::globalInstance(), &Network::sendModelLoaded);
    connect(this, &ChatLLM::shouldBeLoadedChanged, this, &ChatLLM::handleShouldBeLoadedChanged,
        Qt::QueuedConnection); // explicitly queued
    connect(parent, &Chat::idChanged, this, &ChatLLM::handleChatIdChanged);
    connect(parent, &Chat::defaultModelChanged, this, &ChatLLM::handleDefaultModelChanged);
    connect(&m_llmThread, &QThread::started, this, &ChatLLM::handleThreadStarted);

    // The following are blocking operations and will block the llm thread
    connect(this, &ChatLLM::requestRetrieveFromDB, LocalDocs::globalInstance()->database(), &Database::retrieveFromDB,
        Qt::BlockingQueuedConnection);

    m_llmThread.setObjectName(parent->id());
    m_llmThread.start();
}

ChatLLM::~ChatLLM()
{
    m_llmThread.quit();
    m_llmThread.wait();

    // The only time we should have a model loaded here is on shutdown
    // as we explicitly unload the model in all other circumstances
    if (isModelLoaded()) {
        delete m_modelInfo.model;
        m_modelInfo.model = nullptr;
    }
}

void ChatLLM::handleThreadStarted()
{
    m_timer = new TokenTimer(this);
    connect(m_timer, &TokenTimer::report, this, &ChatLLM::reportSpeed);
    emit threadStarted();
}

bool ChatLLM::loadDefaultModel()
{
    if (m_defaultModel.isEmpty()) {
        emit modelLoadingError(QString("Could not find default model to load"));
        return false;
    }
    return loadModel(m_defaultModel);
}

bool ChatLLM::loadModel(const QString &modelName)
{
    // This is a complicated method because N different possible threads are interested in the outcome
    // of this method. Why? Because we have a main/gui thread trying to monitor the state of N different
    // possible chat threads all vying for a single resource - the currently loaded model - as the user
    // switches back and forth between chats. It is important for our main/gui thread to never block
    // but simultaneously always have up2date information with regards to which chat has the model loaded
    // and what the type and name of that model is. I've tried to comment extensively in this method
    // to provide an overview of what we're doing here.

    // We're already loaded with this model
    if (isModelLoaded() && this->modelName() == modelName)
        return true;

    bool isChatGPT = modelName.startsWith("chatgpt-");
    QString filePath = modelFilePath(modelName, isChatGPT);
    QFileInfo fileInfo(filePath);

    // We have a live model, but it isn't the one we want
    bool alreadyAcquired = isModelLoaded();
    if (alreadyAcquired) {
        resetContext();
#if defined(DEBUG_MODEL_LOADING)
        qDebug() << "already acquired model deleted" << m_llmThread.objectName() << m_modelInfo.model;
#endif
        delete m_modelInfo.model;
        m_modelInfo.model = nullptr;
        emit isModelLoadedChanged(false);
    } else if (!m_isServer) {
        // This is a blocking call that tries to retrieve the model we need from the model store.
        // If it succeeds, then we just have to restore state. If the store has never had a model
        // returned to it, then the modelInfo.model pointer should be null which will happen on startup
        m_modelInfo = LLModelStore::globalInstance()->acquireModel();
#if defined(DEBUG_MODEL_LOADING)
        qDebug() << "acquired model from store" << m_llmThread.objectName() << m_modelInfo.model;
#endif
        // At this point it is possible that while we were blocked waiting to acquire the model from the
        // store, that our state was changed to not be loaded. If this is the case, release the model
        // back into the store and quit loading
        if (!m_shouldBeLoaded) {
#if defined(DEBUG_MODEL_LOADING)
            qDebug() << "no longer need model" << m_llmThread.objectName() << m_modelInfo.model;
#endif
            LLModelStore::globalInstance()->releaseModel(m_modelInfo);
            m_modelInfo = LLModelInfo();
            emit isModelLoadedChanged(false);
            return false;
        }

        // Check if the store just gave us exactly the model we were looking for
        if (m_modelInfo.model && m_modelInfo.fileInfo == fileInfo) {
#if defined(DEBUG_MODEL_LOADING)
            qDebug() << "store had our model" << m_llmThread.objectName() << m_modelInfo.model;
#endif
            restoreState();
            emit isModelLoadedChanged(true);
            return true;
        } else {
            // Release the memory since we have to switch to a different model.
#if defined(DEBUG_MODEL_LOADING)
            qDebug() << "deleting model" << m_llmThread.objectName() << m_modelInfo.model;
#endif
            delete m_modelInfo.model;
            m_modelInfo.model = nullptr;
        }
    }

    // Guarantee we've released the previous models memory
    Q_ASSERT(!m_modelInfo.model);

    // Store the file info in the modelInfo in case we have an error loading
    m_modelInfo.fileInfo = fileInfo;

    if (fileInfo.exists()) {
        if (isChatGPT) {
            QString apiKey;
            QString chatGPTModel = fileInfo.completeBaseName().remove(0, 8); // remove the chatgpt- prefix
            {
                QFile file(filePath);
                file.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text);
                QTextStream stream(&file);
                apiKey = stream.readAll();
                file.close();
            }
            m_modelType = LLModelType::CHATGPT_;
            ChatGPT *model = new ChatGPT();
            model->setModelName(chatGPTModel);
            model->setAPIKey(apiKey);
            m_modelInfo.model = model;
        } else {
            m_modelInfo.model = LLModel::construct(filePath.toStdString());
            if (m_modelInfo.model) {
                bool success = m_modelInfo.model->loadModel(filePath.toStdString());
                if (!success) {
                    delete std::exchange(m_modelInfo.model, nullptr);
                    if (!m_isServer)
                        LLModelStore::globalInstance()->releaseModel(m_modelInfo); // release back into the store
                    m_modelInfo = LLModelInfo();
                    emit modelLoadingError(QString("Could not load model due to invalid model file for %1").arg(modelName));
                } else {
                    switch (m_modelInfo.model->implementation().modelType[0]) {
                    case 'L': m_modelType = LLModelType::LLAMA_; break;
                    case 'G': m_modelType = LLModelType::GPTJ_; break;
                    case 'M': m_modelType = LLModelType::MPT_; break;
                    case 'R': m_modelType = LLModelType::REPLIT_; break;
                    default:
                        {
                            delete std::exchange(m_modelInfo.model, nullptr);
                            if (!m_isServer)
                                LLModelStore::globalInstance()->releaseModel(m_modelInfo); // release back into the store
                            m_modelInfo = LLModelInfo();
                            emit modelLoadingError(QString("Could not determine model type for %1").arg(modelName));
                        }
                    }
                }
            } else {
                if (!m_isServer)
                    LLModelStore::globalInstance()->releaseModel(m_modelInfo); // release back into the store
                m_modelInfo = LLModelInfo();
                emit modelLoadingError(QString("Could not load model due to invalid format for %1").arg(modelName));
            }
        }
#if defined(DEBUG_MODEL_LOADING)
        qDebug() << "new model" << m_llmThread.objectName() << m_modelInfo.model;
#endif
        restoreState();
#if defined(DEBUG)
        qDebug() << "modelLoadedChanged" << m_llmThread.objectName();
        fflush(stdout);
#endif
        emit isModelLoadedChanged(isModelLoaded());

        static bool isFirstLoad = true;
        if (isFirstLoad) {
            emit sendStartup();
            isFirstLoad = false;
        } else
            emit sendModelLoaded();
    } else {
        if (!m_isServer)
            LLModelStore::globalInstance()->releaseModel(m_modelInfo); // release back into the store
        m_modelInfo = LLModelInfo();
        emit modelLoadingError(QString("Could not find file for model %1").arg(modelName));
    }

    if (m_modelInfo.model) {
        QString basename = fileInfo.completeBaseName();
        if (basename.startsWith("ggml-")) // remove the ggml- prefix
            basename.remove(0, 5);
        setModelName(basename);
    }

    return m_modelInfo.model;
}

bool ChatLLM::isModelLoaded() const
{
    return m_modelInfo.model && m_modelInfo.model->isModelLoaded();
}

void ChatLLM::regenerateResponse()
{
    // ChatGPT uses a different semantic meaning for n_past than local models. For ChatGPT, the meaning
    // of n_past is of the number of prompt/response pairs, rather than for total tokens.
    if (m_modelType == LLModelType::CHATGPT_)
        m_ctx.n_past -= 1;
    else
        m_ctx.n_past -= m_promptResponseTokens;
    m_ctx.n_past = std::max(0, m_ctx.n_past);
    m_ctx.tokens.erase(m_ctx.tokens.end() -= m_promptResponseTokens, m_ctx.tokens.end());
    m_promptResponseTokens = 0;
    m_promptTokens = 0;
    m_response = std::string();
    emit responseChanged(QString::fromStdString(m_response));
}

void ChatLLM::resetResponse()
{
    m_promptTokens = 0;
    m_promptResponseTokens = 0;
    m_response = std::string();
    emit responseChanged(QString::fromStdString(m_response));
}

void ChatLLM::resetContext()
{
    regenerateResponse();
    m_ctx = LLModel::PromptContext();
}

std::string remove_leading_whitespace(const std::string& input) {
    auto first_non_whitespace = std::find_if(input.begin(), input.end(), [](unsigned char c) {
        return !std::isspace(c);
    });

    return std::string(first_non_whitespace, input.end());
}

std::string trim_whitespace(const std::string& input) {
    auto first_non_whitespace = std::find_if(input.begin(), input.end(), [](unsigned char c) {
        return !std::isspace(c);
    });

    auto last_non_whitespace = std::find_if(input.rbegin(), input.rend(), [](unsigned char c) {
        return !std::isspace(c);
    }).base();

    return std::string(first_non_whitespace, last_non_whitespace);
}

QString ChatLLM::response() const
{
    return QString::fromStdString(remove_leading_whitespace(m_response));
}

QString ChatLLM::modelName() const
{
    return m_modelName;
}

void ChatLLM::setModelName(const QString &modelName)
{
    m_modelName = modelName;
    emit modelNameChanged();
}

void ChatLLM::modelNameChangeRequested(const QString &modelName)
{
    loadModel(modelName);
}

bool ChatLLM::handlePrompt(int32_t token)
{
    // m_promptResponseTokens is related to last prompt/response not
    // the entire context window which we can reset on regenerate prompt
#if defined(DEBUG)
    qDebug() << "prompt process" << m_llmThread.objectName() << token;
#endif
    ++m_promptTokens;
    ++m_promptResponseTokens;
    m_timer->inc();
    return !m_stopGenerating;
}

bool ChatLLM::handleResponse(int32_t token, const std::string &response)
{
#if defined(DEBUG)
    printf("%s", response.c_str());
    fflush(stdout);
#endif

    // check for error
    if (token < 0) {
        m_response.append(response);
        emit responseChanged(QString::fromStdString(m_response));
        return false;
    }

    // m_promptResponseTokens is related to last prompt/response not
    // the entire context window which we can reset on regenerate prompt
    ++m_promptResponseTokens;
    m_timer->inc();
    Q_ASSERT(!response.empty());
    m_response.append(response);
    emit responseChanged(QString::fromStdString(m_response));
    return !m_stopGenerating;
}

bool ChatLLM::handleRecalculate(bool isRecalc)
{
    if (m_isRecalc != isRecalc) {
        m_isRecalc = isRecalc;
        emit recalcChanged();
    }
    return !m_stopGenerating;
}

bool ChatLLM::prompt(const QList<QString> &collectionList, const QString &prompt, const QString &prompt_template, int32_t n_predict, int32_t top_k,
    float top_p, float temp, int32_t n_batch, float repeat_penalty, int32_t repeat_penalty_tokens, int n_threads)
{
    if (!isModelLoaded())
        return false;

    QList<ResultInfo> databaseResults;
    const int retrievalSize = LocalDocs::globalInstance()->retrievalSize();
    emit requestRetrieveFromDB(collectionList, prompt, retrievalSize, &databaseResults); // blocks
    emit databaseResultsChanged(databaseResults);

    // Augment the prompt template with the results if any
    QList<QString> augmentedTemplate;
    if (!databaseResults.isEmpty())
        augmentedTemplate.append("### Context:");
    for (const ResultInfo &info : databaseResults)
        augmentedTemplate.append(info.text);
    augmentedTemplate.append(prompt_template);

    QString instructPrompt = augmentedTemplate.join("\n").arg(prompt);

    m_stopGenerating = false;
    auto promptFunc = std::bind(&ChatLLM::handlePrompt, this, std::placeholders::_1);
    auto responseFunc = std::bind(&ChatLLM::handleResponse, this, std::placeholders::_1,
        std::placeholders::_2);
    auto recalcFunc = std::bind(&ChatLLM::handleRecalculate, this, std::placeholders::_1);
    emit promptProcessing();
    qint32 logitsBefore = m_ctx.logits.size();
    m_ctx.n_predict = n_predict;
    m_ctx.top_k = top_k;
    m_ctx.top_p = top_p;
    m_ctx.temp = temp;
    m_ctx.n_batch = n_batch;
    m_ctx.repeat_penalty = repeat_penalty;
    m_ctx.repeat_last_n = repeat_penalty_tokens;
    m_modelInfo.model->setThreadCount(n_threads);
#if defined(DEBUG)
    printf("%s", qPrintable(instructPrompt));
    fflush(stdout);
#endif
    m_timer->start();
    m_modelInfo.model->prompt(instructPrompt.toStdString(), promptFunc, responseFunc, recalcFunc, m_ctx);
#if defined(DEBUG)
    printf("\n");
    fflush(stdout);
#endif
    m_timer->stop();
    std::string trimmed = trim_whitespace(m_response);
    if (trimmed != m_response) {
        m_response = trimmed;
        emit responseChanged(QString::fromStdString(m_response));
    }
    emit responseStopped();
    return true;
}

void ChatLLM::setShouldBeLoaded(bool b)
{
#if defined(DEBUG_MODEL_LOADING)
    qDebug() << "setShouldBeLoaded" << m_llmThread.objectName() << b << m_modelInfo.model;
#endif
    m_shouldBeLoaded = b; // atomic
    emit shouldBeLoadedChanged();
}

void ChatLLM::handleShouldBeLoadedChanged()
{
    if (m_shouldBeLoaded)
        reloadModel();
    else
        unloadModel();
}

void ChatLLM::forceUnloadModel()
{
    m_shouldBeLoaded = false; // atomic
    unloadModel();
}

void ChatLLM::unloadModel()
{
    if (!isModelLoaded() || m_isServer)
        return;

    saveState();
#if defined(DEBUG_MODEL_LOADING)
    qDebug() << "unloadModel" << m_llmThread.objectName() << m_modelInfo.model;
#endif
    LLModelStore::globalInstance()->releaseModel(m_modelInfo);
    m_modelInfo = LLModelInfo();
    emit isModelLoadedChanged(false);
}

void ChatLLM::reloadModel()
{
    if (isModelLoaded() || m_isServer)
        return;

#if defined(DEBUG_MODEL_LOADING)
    qDebug() << "reloadModel" << m_llmThread.objectName() << m_modelInfo.model;
#endif
    const QString m = modelName();
    if (m.isEmpty())
        loadDefaultModel();
    else
        loadModel(m);
}

void ChatLLM::generateName()
{
    Q_ASSERT(isModelLoaded());
    if (!isModelLoaded())
        return;

    QString instructPrompt("### Instruction:\n"
                           "Describe response above in three words.\n"
                           "### Response:\n");
    auto promptFunc = std::bind(&ChatLLM::handleNamePrompt, this, std::placeholders::_1);
    auto responseFunc = std::bind(&ChatLLM::handleNameResponse, this, std::placeholders::_1,
        std::placeholders::_2);
    auto recalcFunc = std::bind(&ChatLLM::handleNameRecalculate, this, std::placeholders::_1);
    LLModel::PromptContext ctx = m_ctx;
#if defined(DEBUG)
    printf("%s", qPrintable(instructPrompt));
    fflush(stdout);
#endif
    m_modelInfo.model->prompt(instructPrompt.toStdString(), promptFunc, responseFunc, recalcFunc, ctx);
#if defined(DEBUG)
    printf("\n");
    fflush(stdout);
#endif
    std::string trimmed = trim_whitespace(m_nameResponse);
    if (trimmed != m_nameResponse) {
        m_nameResponse = trimmed;
        emit generatedNameChanged(QString::fromStdString(m_nameResponse));
    }
}

void ChatLLM::handleChatIdChanged(const QString &id)
{
    m_llmThread.setObjectName(id);
}

void ChatLLM::handleDefaultModelChanged(const QString &defaultModel)
{
    m_defaultModel = defaultModel;
}

bool ChatLLM::handleNamePrompt(int32_t token)
{
    Q_UNUSED(token);
    qt_noop();
    return true;
}

bool ChatLLM::handleNameResponse(int32_t token, const std::string &response)
{
    Q_UNUSED(token);

    m_nameResponse.append(response);
    emit generatedNameChanged(QString::fromStdString(m_nameResponse));
    QString gen = QString::fromStdString(m_nameResponse).simplified();
    QStringList words = gen.split(' ', Qt::SkipEmptyParts);
    return words.size() <= 3;
}

bool ChatLLM::handleNameRecalculate(bool isRecalc)
{
    Q_UNUSED(isRecalc);
    Q_UNREACHABLE();
    return true;
}

bool ChatLLM::serialize(QDataStream &stream, int version)
{
    if (version > 1) {
        stream << m_modelType;
        switch (m_modelType) {
        case REPLIT_: stream << REPLIT_INTERNAL_STATE_VERSION; break;
        case MPT_: stream << MPT_INTERNAL_STATE_VERSION; break;
        case GPTJ_: stream << GPTJ_INTERNAL_STATE_VERSION; break;
        case LLAMA_: stream << LLAMA_INTERNAL_STATE_VERSION; break;
        default: Q_UNREACHABLE();
        }
    }
    stream << response();
    stream << generatedName();
    stream << m_promptResponseTokens;
    if (version <= 3) {
        int responseLogits;
        stream << responseLogits;
    }
    stream << m_ctx.n_past;
    stream << quint64(m_ctx.logits.size());
    stream.writeRawData(reinterpret_cast<const char*>(m_ctx.logits.data()), m_ctx.logits.size() * sizeof(float));
    stream << quint64(m_ctx.tokens.size());
    stream.writeRawData(reinterpret_cast<const char*>(m_ctx.tokens.data()), m_ctx.tokens.size() * sizeof(int));
    saveState();
    QByteArray compressed = qCompress(m_state);
    stream << compressed;
#if defined(DEBUG)
    qDebug() << "serialize" << m_llmThread.objectName() << m_state.size();
#endif
    return stream.status() == QDataStream::Ok;
}

bool ChatLLM::deserialize(QDataStream &stream, int version)
{
    if (version > 1) {
        int internalStateVersion;
        stream >> m_modelType;
        stream >> internalStateVersion; // for future use
    }
    QString response;
    stream >> response;
    m_response = response.toStdString();
    QString nameResponse;
    stream >> nameResponse;
    m_nameResponse = nameResponse.toStdString();
    stream >> m_promptResponseTokens;
    if (version <= 3) {
        int responseLogits;
        stream >> responseLogits;
    }
    stream >> m_ctx.n_past;
    quint64 logitsSize;
    stream >> logitsSize;
    m_ctx.logits.resize(logitsSize);
    stream.readRawData(reinterpret_cast<char*>(m_ctx.logits.data()), logitsSize * sizeof(float));
    quint64 tokensSize;
    stream >> tokensSize;
    m_ctx.tokens.resize(tokensSize);
    stream.readRawData(reinterpret_cast<char*>(m_ctx.tokens.data()), tokensSize * sizeof(int));
    if (version > 0) {
        QByteArray compressed;
        stream >> compressed;
        m_state = qUncompress(compressed);
    } else {
        stream >> m_state;
    }
#if defined(DEBUG)
    qDebug() << "deserialize" << m_llmThread.objectName();
#endif
    return stream.status() == QDataStream::Ok;
}

void ChatLLM::saveState()
{
    if (!isModelLoaded())
        return;

    if (m_modelType == LLModelType::CHATGPT_) {
        m_state.clear();
        QDataStream stream(&m_state, QIODeviceBase::WriteOnly);
        stream.setVersion(QDataStream::Qt_6_5);
        ChatGPT *chatGPT = static_cast<ChatGPT*>(m_modelInfo.model);
        stream << chatGPT->context();
        return;
    }

    const size_t stateSize = m_modelInfo.model->stateSize();
    m_state.resize(stateSize);
#if defined(DEBUG)
    qDebug() << "saveState" << m_llmThread.objectName() << "size:" << m_state.size();
#endif
    m_modelInfo.model->saveState(static_cast<uint8_t*>(reinterpret_cast<void*>(m_state.data())));
}

void ChatLLM::restoreState()
{
    if (!isModelLoaded() || m_state.isEmpty())
        return;

    if (m_modelType == LLModelType::CHATGPT_) {
        QDataStream stream(&m_state, QIODeviceBase::ReadOnly);
        stream.setVersion(QDataStream::Qt_6_5);
        ChatGPT *chatGPT = static_cast<ChatGPT*>(m_modelInfo.model);
        QList<QString> context;
        stream >> context;
        chatGPT->setContext(context);
        m_state.clear();
        m_state.resize(0);
        return;
    }

#if defined(DEBUG)
    qDebug() << "restoreState" << m_llmThread.objectName() << "size:" << m_state.size();
#endif
    m_modelInfo.model->restoreState(static_cast<const uint8_t*>(reinterpret_cast<void*>(m_state.data())));
    m_state.clear();
    m_state.resize(0);
}
