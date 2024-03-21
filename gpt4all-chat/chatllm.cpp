#include "chatllm.h"
#include "chat.h"
#include "chatapi.h"
#include "localdocs.h"
#include "modellist.h"
#include "network.h"
#include "mysettings.h"
#include "../gpt4all-backend/llmodel.h"

//#define DEBUG
//#define DEBUG_MODEL_LOADING

#define GPTJ_INTERNAL_STATE_VERSION 0
#define LLAMA_INTERNAL_STATE_VERSION 0

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
    , m_shouldBeLoaded(false)
    , m_forceUnloadModel(false)
    , m_markedForDeletion(false)
    , m_shouldTrySwitchContext(false)
    , m_stopGenerating(false)
    , m_timer(nullptr)
    , m_isServer(isServer)
    , m_forceMetal(MySettings::globalInstance()->forceMetal())
    , m_reloadingToChangeVariant(false)
    , m_processedSystemPrompt(false)
    , m_restoreStateFromText(false)
{
    moveToThread(&m_llmThread);
    connect(this, &ChatLLM::sendStartup, Network::globalInstance(), &Network::sendStartup);
    connect(this, &ChatLLM::sendModelLoaded, Network::globalInstance(), &Network::sendModelLoaded);
    connect(this, &ChatLLM::shouldBeLoadedChanged, this, &ChatLLM::handleShouldBeLoadedChanged,
        Qt::QueuedConnection); // explicitly queued
    connect(this, &ChatLLM::shouldTrySwitchContextChanged, this, &ChatLLM::handleShouldTrySwitchContextChanged,
        Qt::QueuedConnection); // explicitly queued
    connect(parent, &Chat::idChanged, this, &ChatLLM::handleChatIdChanged);
    connect(&m_llmThread, &QThread::started, this, &ChatLLM::handleThreadStarted);
    connect(MySettings::globalInstance(), &MySettings::forceMetalChanged, this, &ChatLLM::handleForceMetalChanged);
    connect(MySettings::globalInstance(), &MySettings::deviceChanged, this, &ChatLLM::handleDeviceChanged);

    // The following are blocking operations and will block the llm thread
    connect(this, &ChatLLM::requestRetrieveFromDB, LocalDocs::globalInstance()->database(), &Database::retrieveFromDB,
        Qt::BlockingQueuedConnection);

    m_llmThread.setObjectName(parent->id());
    m_llmThread.start();
}

ChatLLM::~ChatLLM()
{
    destroy();
}

void ChatLLM::destroy() {
    m_stopGenerating = true;
    m_llmThread.quit();
    m_llmThread.wait();

    // The only time we should have a model loaded here is on shutdown
    // as we explicitly unload the model in all other circumstances
    if (isModelLoaded()) {
        delete m_llModelInfo.model;
        m_llModelInfo.model = nullptr;
    }
}

void ChatLLM::handleThreadStarted()
{
    m_timer = new TokenTimer(this);
    connect(m_timer, &TokenTimer::report, this, &ChatLLM::reportSpeed);
    emit threadStarted();
}

void ChatLLM::handleForceMetalChanged(bool forceMetal)
{
#if defined(Q_OS_MAC) && defined(__arm__)
    m_forceMetal = forceMetal;
    if (isModelLoaded() && m_shouldBeLoaded) {
        m_reloadingToChangeVariant = true;
        unloadModel();
        reloadModel();
        m_reloadingToChangeVariant = false;
    }
#endif
}

void ChatLLM::handleDeviceChanged()
{
    if (isModelLoaded() && m_shouldBeLoaded) {
        m_reloadingToChangeVariant = true;
        unloadModel();
        reloadModel();
        m_reloadingToChangeVariant = false;
    }
}

bool ChatLLM::loadDefaultModel()
{
    ModelInfo defaultModel = ModelList::globalInstance()->defaultModelInfo();
    if (defaultModel.filename().isEmpty()) {
        emit modelLoadingError(QString("Could not find any model to load"));
        return false;
    }
    return loadModel(defaultModel);
}

bool ChatLLM::trySwitchContextOfLoadedModel(const ModelInfo &modelInfo)
{
    // We're trying to see if the store already has the model fully loaded that we wish to use
    // and if so we just acquire it from the store and switch the context and return true. If the
    // store doesn't have it or we're already loaded or in any other case just return false.

    // If we're already loaded or a server or we're reloading to change the variant/device or the
    // modelInfo is empty, then this should fail
    if (isModelLoaded() || m_isServer || m_reloadingToChangeVariant || modelInfo.name().isEmpty()) {
        m_shouldTrySwitchContext = false;
        emit trySwitchContextOfLoadedModelCompleted(false);
        return false;
    }

    QString filePath = modelInfo.dirpath + modelInfo.filename();
    QFileInfo fileInfo(filePath);

    m_llModelInfo = LLModelStore::globalInstance()->acquireModel();
#if defined(DEBUG_MODEL_LOADING)
        qDebug() << "acquired model from store" << m_llmThread.objectName() << m_llModelInfo.model;
#endif

    // The store gave us no already loaded model, the wrong type of model, then give it back to the
    // store and fail
    if (!m_llModelInfo.model || m_llModelInfo.fileInfo != fileInfo) {
        LLModelStore::globalInstance()->releaseModel(m_llModelInfo);
        m_llModelInfo = LLModelInfo();
        m_shouldTrySwitchContext = false;
        emit trySwitchContextOfLoadedModelCompleted(false);
        return false;
    }

#if defined(DEBUG_MODEL_LOADING)
    qDebug() << "store had our model" << m_llmThread.objectName() << m_llModelInfo.model;
#endif

    // We should be loaded and now we are
    m_shouldBeLoaded = true;
    m_shouldTrySwitchContext = false;

    // Restore, signal and process
    restoreState();
    emit modelLoadingPercentageChanged(1.0f);
    emit trySwitchContextOfLoadedModelCompleted(true);
    processSystemPrompt();
    return true;
}

bool ChatLLM::loadModel(const ModelInfo &modelInfo)
{
    // This is a complicated method because N different possible threads are interested in the outcome
    // of this method. Why? Because we have a main/gui thread trying to monitor the state of N different
    // possible chat threads all vying for a single resource - the currently loaded model - as the user
    // switches back and forth between chats. It is important for our main/gui thread to never block
    // but simultaneously always have up2date information with regards to which chat has the model loaded
    // and what the type and name of that model is. I've tried to comment extensively in this method
    // to provide an overview of what we're doing here.

    // We're already loaded with this model
    if (isModelLoaded() && this->modelInfo() == modelInfo)
        return true;

    QString filePath = modelInfo.dirpath + modelInfo.filename();
    QFileInfo fileInfo(filePath);

    // We have a live model, but it isn't the one we want
    bool alreadyAcquired = isModelLoaded();
    if (alreadyAcquired) {
        resetContext();
#if defined(DEBUG_MODEL_LOADING)
        qDebug() << "already acquired model deleted" << m_llmThread.objectName() << m_llModelInfo.model;
#endif
        delete m_llModelInfo.model;
        m_llModelInfo.model = nullptr;
        emit modelLoadingPercentageChanged(std::numeric_limits<float>::min()); // small non-zero positive value
    } else if (!m_isServer) {
        // This is a blocking call that tries to retrieve the model we need from the model store.
        // If it succeeds, then we just have to restore state. If the store has never had a model
        // returned to it, then the modelInfo.model pointer should be null which will happen on startup
        m_llModelInfo = LLModelStore::globalInstance()->acquireModel();
#if defined(DEBUG_MODEL_LOADING)
        qDebug() << "acquired model from store" << m_llmThread.objectName() << m_llModelInfo.model;
#endif
        // At this point it is possible that while we were blocked waiting to acquire the model from the
        // store, that our state was changed to not be loaded. If this is the case, release the model
        // back into the store and quit loading
        if (!m_shouldBeLoaded) {
#if defined(DEBUG_MODEL_LOADING)
            qDebug() << "no longer need model" << m_llmThread.objectName() << m_llModelInfo.model;
#endif
            LLModelStore::globalInstance()->releaseModel(m_llModelInfo);
            m_llModelInfo = LLModelInfo();
            emit modelLoadingPercentageChanged(0.0f);
            return false;
        }

        // Check if the store just gave us exactly the model we were looking for
        if (m_llModelInfo.model && m_llModelInfo.fileInfo == fileInfo && !m_reloadingToChangeVariant) {
#if defined(DEBUG_MODEL_LOADING)
            qDebug() << "store had our model" << m_llmThread.objectName() << m_llModelInfo.model;
#endif
            restoreState();
            emit modelLoadingPercentageChanged(1.0f);
            setModelInfo(modelInfo);
            Q_ASSERT(!m_modelInfo.filename().isEmpty());
            if (m_modelInfo.filename().isEmpty())
                emit modelLoadingError(QString("Modelinfo is left null for %1").arg(modelInfo.filename()));
            else
                processSystemPrompt();
            return true;
        } else {
            // Release the memory since we have to switch to a different model.
#if defined(DEBUG_MODEL_LOADING)
            qDebug() << "deleting model" << m_llmThread.objectName() << m_llModelInfo.model;
#endif
            delete m_llModelInfo.model;
            m_llModelInfo.model = nullptr;
        }
    }

    // Guarantee we've released the previous models memory
    Q_ASSERT(!m_llModelInfo.model);

    // Store the file info in the modelInfo in case we have an error loading
    m_llModelInfo.fileInfo = fileInfo;

    if (fileInfo.exists()) {
        if (modelInfo.isOnline) {
            QString apiKey;
            QString modelName;
            {
                QFile file(filePath);
                file.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text);
                QTextStream stream(&file);
                QString text = stream.readAll();
                QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8());
                QJsonObject obj = doc.object();
                apiKey = obj["apiKey"].toString();
                modelName = obj["modelName"].toString();
            }
            m_llModelType = LLModelType::API_;
            ChatAPI *model = new ChatAPI();
            model->setModelName(modelName);
            model->setRequestURL(modelInfo.url());
            model->setAPIKey(apiKey);
            m_llModelInfo.model = model;
        } else {
            auto n_ctx = MySettings::globalInstance()->modelContextLength(modelInfo);
            m_ctx.n_ctx = n_ctx;
            auto ngl = MySettings::globalInstance()->modelGpuLayers(modelInfo);

            std::string buildVariant = "auto";
#if defined(Q_OS_MAC) && defined(__arm__)
            if (m_forceMetal)
                buildVariant = "metal";
#endif
            m_llModelInfo.model = LLModel::Implementation::construct(filePath.toStdString(), buildVariant, n_ctx);

            if (m_llModelInfo.model) {
                if (m_llModelInfo.model->isModelBlacklisted(filePath.toStdString())) {
                    static QSet<QString> warned;
                    auto fname = modelInfo.filename();
                    if (!warned.contains(fname)) {
                        emit modelLoadingWarning(QString(
                            "%1 is known to be broken. Please get a replacement via the download dialog."
                        ).arg(fname));
                        warned.insert(fname); // don't warn again until restart
                    }
                }

                m_llModelInfo.model->setProgressCallback([this](float progress) -> bool {
                    emit modelLoadingPercentageChanged(progress);
                    return m_shouldBeLoaded;
                });

                // Pick the best match for the device
                QString actualDevice = m_llModelInfo.model->implementation().buildVariant() == "metal" ? "Metal" : "CPU";
                const QString requestedDevice = MySettings::globalInstance()->device();
                if (requestedDevice == "CPU") {
                    emit reportFallbackReason(""); // fallback not applicable
                } else {
                    const size_t requiredMemory = m_llModelInfo.model->requiredMem(filePath.toStdString(), n_ctx, ngl);
                    std::vector<LLModel::GPUDevice> availableDevices = m_llModelInfo.model->availableGPUDevices(requiredMemory);
                    LLModel::GPUDevice *device = nullptr;

                    if (!availableDevices.empty() && requestedDevice == "Auto" && availableDevices.front().type == 2 /*a discrete gpu*/) {
                        device = &availableDevices.front();
                    } else {
                        for (LLModel::GPUDevice &d : availableDevices) {
                            if (QString::fromStdString(d.name) == requestedDevice) {
                                device = &d;
                                break;
                            }
                        }
                    }

                    emit reportFallbackReason(""); // no fallback yet
                    std::string unavail_reason;
                    if (!device) {
                        // GPU not available
                    } else if (!m_llModelInfo.model->initializeGPUDevice(device->index, &unavail_reason)) {
                        emit reportFallbackReason(QString::fromStdString("<br>" + unavail_reason));
                    } else {
                        actualDevice = QString::fromStdString(device->name);
                    }
                }

                // Report which device we're actually using
                emit reportDevice(actualDevice);

                bool success = m_llModelInfo.model->loadModel(filePath.toStdString(), n_ctx, ngl);
                if (actualDevice == "CPU") {
                    // we asked llama.cpp to use the CPU
                } else if (!success) {
                    // llama_init_from_file returned nullptr
                    emit reportDevice("CPU");
                    emit reportFallbackReason("<br>GPU loading failed (out of VRAM?)");
                    success = m_llModelInfo.model->loadModel(filePath.toStdString(), n_ctx, 0);
                } else if (!m_llModelInfo.model->usingGPUDevice()) {
                    // ggml_vk_init was not called in llama.cpp
                    // We might have had to fallback to CPU after load if the model is not possible to accelerate
                    // for instance if the quantization method is not supported on Vulkan yet
                    emit reportDevice("CPU");
                    emit reportFallbackReason("<br>model or quant has no GPU support");
                }

                if (!success) {
                    delete m_llModelInfo.model;
                    m_llModelInfo.model = nullptr;
                    if (!m_isServer)
                        LLModelStore::globalInstance()->releaseModel(m_llModelInfo); // release back into the store
                    m_llModelInfo = LLModelInfo();
                    emit modelLoadingError(QString("Could not load model due to invalid model file for %1").arg(modelInfo.filename()));
                } else {
                    switch (m_llModelInfo.model->implementation().modelType()[0]) {
                    case 'L': m_llModelType = LLModelType::LLAMA_; break;
                    case 'G': m_llModelType = LLModelType::GPTJ_; break;
                    default:
                        {
                            delete m_llModelInfo.model;
                            m_llModelInfo.model = nullptr;
                            if (!m_isServer)
                                LLModelStore::globalInstance()->releaseModel(m_llModelInfo); // release back into the store
                            m_llModelInfo = LLModelInfo();
                            emit modelLoadingError(QString("Could not determine model type for %1").arg(modelInfo.filename()));
                        }
                    }
                }
            } else {
                if (!m_isServer)
                    LLModelStore::globalInstance()->releaseModel(m_llModelInfo); // release back into the store
                m_llModelInfo = LLModelInfo();
                emit modelLoadingError(QString("Could not load model due to invalid format for %1").arg(modelInfo.filename()));
            }
        }
#if defined(DEBUG_MODEL_LOADING)
        qDebug() << "new model" << m_llmThread.objectName() << m_llModelInfo.model;
#endif
        restoreState();
#if defined(DEBUG)
        qDebug() << "modelLoadedChanged" << m_llmThread.objectName();
        fflush(stdout);
#endif
        emit modelLoadingPercentageChanged(isModelLoaded() ? 1.0f : 0.0f);

        static bool isFirstLoad = true;
        if (isFirstLoad) {
            emit sendStartup();
            isFirstLoad = false;
        } else
            emit sendModelLoaded();
    } else {
        if (!m_isServer)
            LLModelStore::globalInstance()->releaseModel(m_llModelInfo); // release back into the store
        m_llModelInfo = LLModelInfo();
        emit modelLoadingError(QString("Could not find file for model %1").arg(modelInfo.filename()));
    }

    if (m_llModelInfo.model) {
        setModelInfo(modelInfo);
        processSystemPrompt();
    }
    return m_llModelInfo.model;
}

bool ChatLLM::isModelLoaded() const
{
    return m_llModelInfo.model && m_llModelInfo.model->isModelLoaded();
}

std::string remove_leading_whitespace(const std::string& input) {
    auto first_non_whitespace = std::find_if(input.begin(), input.end(), [](unsigned char c) {
        return !std::isspace(c);
    });

    if (first_non_whitespace == input.end())
        return std::string();

    return std::string(first_non_whitespace, input.end());
}

std::string trim_whitespace(const std::string& input) {
    auto first_non_whitespace = std::find_if(input.begin(), input.end(), [](unsigned char c) {
        return !std::isspace(c);
    });

    if (first_non_whitespace == input.end())
        return std::string();

    auto last_non_whitespace = std::find_if(input.rbegin(), input.rend(), [](unsigned char c) {
        return !std::isspace(c);
    }).base();

    return std::string(first_non_whitespace, last_non_whitespace);
}

void ChatLLM::regenerateResponse()
{
    // ChatGPT uses a different semantic meaning for n_past than local models. For ChatGPT, the meaning
    // of n_past is of the number of prompt/response pairs, rather than for total tokens.
    if (m_llModelType == LLModelType::API_)
        m_ctx.n_past -= 1;
    else
        m_ctx.n_past -= m_promptResponseTokens;
    m_ctx.n_past = std::max(0, m_ctx.n_past);
    m_ctx.tokens.erase(m_ctx.tokens.end() - m_promptResponseTokens, m_ctx.tokens.end());
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
    resetResponse();
    m_processedSystemPrompt = false;
    m_ctx = LLModel::PromptContext();
}

QString ChatLLM::response() const
{
    return QString::fromStdString(remove_leading_whitespace(m_response));
}

ModelInfo ChatLLM::modelInfo() const
{
    return m_modelInfo;
}

void ChatLLM::setModelInfo(const ModelInfo &modelInfo)
{
    m_modelInfo = modelInfo;
    emit modelInfoChanged(modelInfo);
}

void ChatLLM::modelChangeRequested(const ModelInfo &modelInfo)
{
    m_shouldBeLoaded = true;
    loadModel(modelInfo);
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
    m_timer->start();
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
        emit responseChanged(QString::fromStdString(remove_leading_whitespace(m_response)));
        return false;
    }

    // m_promptResponseTokens is related to last prompt/response not
    // the entire context window which we can reset on regenerate prompt
    ++m_promptResponseTokens;
    m_timer->inc();
    Q_ASSERT(!response.empty());
    m_response.append(response);
    emit responseChanged(QString::fromStdString(remove_leading_whitespace(m_response)));
    return !m_stopGenerating;
}

bool ChatLLM::handleRecalculate(bool isRecalc)
{
#if defined(DEBUG)
    qDebug() << "recalculate" << m_llmThread.objectName() << isRecalc;
#endif
    if (m_isRecalc != isRecalc) {
        m_isRecalc = isRecalc;
        emit recalcChanged();
    }
    return !m_stopGenerating;
}
bool ChatLLM::prompt(const QList<QString> &collectionList, const QString &prompt)
{
    if (m_restoreStateFromText) {
        Q_ASSERT(m_state.isEmpty());
        processRestoreStateFromText();
    }

    if (!m_processedSystemPrompt)
        processSystemPrompt();
    const QString promptTemplate = MySettings::globalInstance()->modelPromptTemplate(m_modelInfo);
    const int32_t n_predict = MySettings::globalInstance()->modelMaxLength(m_modelInfo);
    const int32_t top_k = MySettings::globalInstance()->modelTopK(m_modelInfo);
    const float top_p = MySettings::globalInstance()->modelTopP(m_modelInfo);
    const float min_p = MySettings::globalInstance()->modelMinP(m_modelInfo);
    const float temp = MySettings::globalInstance()->modelTemperature(m_modelInfo);
    const int32_t n_batch = MySettings::globalInstance()->modelPromptBatchSize(m_modelInfo);
    const float repeat_penalty = MySettings::globalInstance()->modelRepeatPenalty(m_modelInfo);
    const int32_t repeat_penalty_tokens = MySettings::globalInstance()->modelRepeatPenaltyTokens(m_modelInfo);
    return promptInternal(collectionList, prompt, promptTemplate, n_predict, top_k, top_p, min_p, temp, n_batch,
        repeat_penalty, repeat_penalty_tokens);
}

bool ChatLLM::promptInternal(const QList<QString> &collectionList, const QString &prompt, const QString &promptTemplate,
    int32_t n_predict, int32_t top_k, float top_p, float min_p, float temp, int32_t n_batch, float repeat_penalty,
    int32_t repeat_penalty_tokens)
{
    if (!isModelLoaded())
        return false;

    QList<ResultInfo> databaseResults;
    const int retrievalSize = MySettings::globalInstance()->localDocsRetrievalSize();
    if (!collectionList.isEmpty()) {
        emit requestRetrieveFromDB(collectionList, prompt, retrievalSize, &databaseResults); // blocks
        emit databaseResultsChanged(databaseResults);
    }

    // Augment the prompt template with the results if any
    QList<QString> docsContext;
    if (!databaseResults.isEmpty())
        docsContext.append("### Context:");
    for (const ResultInfo &info : databaseResults)
        docsContext.append(info.text);

    int n_threads = MySettings::globalInstance()->threadCount();

    m_stopGenerating = false;
    auto promptFunc = std::bind(&ChatLLM::handlePrompt, this, std::placeholders::_1);
    auto responseFunc = std::bind(&ChatLLM::handleResponse, this, std::placeholders::_1,
        std::placeholders::_2);
    auto recalcFunc = std::bind(&ChatLLM::handleRecalculate, this, std::placeholders::_1);
    emit promptProcessing();
    m_ctx.n_predict = n_predict;
    m_ctx.top_k = top_k;
    m_ctx.top_p = top_p;
    m_ctx.min_p = min_p;
    m_ctx.temp = temp;
    m_ctx.n_batch = n_batch;
    m_ctx.repeat_penalty = repeat_penalty;
    m_ctx.repeat_last_n = repeat_penalty_tokens;
    m_llModelInfo.model->setThreadCount(n_threads);
#if defined(DEBUG)
    printf("%s", qPrintable(prompt));
    fflush(stdout);
#endif
    m_timer->start();
    if (!docsContext.isEmpty()) {
        auto old_n_predict = std::exchange(m_ctx.n_predict, 0); // decode localdocs context without a response
        m_llModelInfo.model->prompt(docsContext.join("\n").toStdString(), "%1", promptFunc, responseFunc, recalcFunc, m_ctx);
        m_ctx.n_predict = old_n_predict; // now we are ready for a response
    }
    m_llModelInfo.model->prompt(prompt.toStdString(), promptTemplate.toStdString(), promptFunc, responseFunc, recalcFunc, m_ctx);
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
    qDebug() << "setShouldBeLoaded" << m_llmThread.objectName() << b << m_llModelInfo.model;
#endif
    m_shouldBeLoaded = b; // atomic
    emit shouldBeLoadedChanged();
}

void ChatLLM::setShouldTrySwitchContext(bool b)
{
    m_shouldTrySwitchContext = b; // atomic
    emit shouldTrySwitchContextChanged();
}

void ChatLLM::handleShouldBeLoadedChanged()
{
    if (m_shouldBeLoaded)
        reloadModel();
    else
        unloadModel();
}

void ChatLLM::handleShouldTrySwitchContextChanged()
{
    if (m_shouldTrySwitchContext)
        trySwitchContextOfLoadedModel(modelInfo());
}

void ChatLLM::unloadModel()
{
    if (!isModelLoaded() || m_isServer)
        return;

    if (!m_forceUnloadModel || !m_shouldBeLoaded)
        emit modelLoadingPercentageChanged(0.0f);
    else
        emit modelLoadingPercentageChanged(std::numeric_limits<float>::min()); // small non-zero positive value

    if (!m_markedForDeletion)
        saveState();

#if defined(DEBUG_MODEL_LOADING)
    qDebug() << "unloadModel" << m_llmThread.objectName() << m_llModelInfo.model;
#endif

    if (m_forceUnloadModel) {
        delete m_llModelInfo.model;
        m_llModelInfo.model = nullptr;
        m_forceUnloadModel = false;
    }

    LLModelStore::globalInstance()->releaseModel(m_llModelInfo);
    m_llModelInfo = LLModelInfo();
}

void ChatLLM::reloadModel()
{
    if (isModelLoaded() && m_forceUnloadModel)
        unloadModel(); // we unload first if we are forcing an unload

    if (isModelLoaded() || m_isServer)
        return;

#if defined(DEBUG_MODEL_LOADING)
    qDebug() << "reloadModel" << m_llmThread.objectName() << m_llModelInfo.model;
#endif
    const ModelInfo m = modelInfo();
    if (m.name().isEmpty())
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
    m_llModelInfo.model->prompt(instructPrompt.toStdString(), "%1", promptFunc, responseFunc, recalcFunc, ctx);
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

bool ChatLLM::handleNamePrompt(int32_t token)
{
#if defined(DEBUG)
    qDebug() << "name prompt" << m_llmThread.objectName() << token;
#endif
    Q_UNUSED(token);
    qt_noop();
    return !m_stopGenerating;
}

bool ChatLLM::handleNameResponse(int32_t token, const std::string &response)
{
#if defined(DEBUG)
    qDebug() << "name response" << m_llmThread.objectName() << token << response;
#endif
    Q_UNUSED(token);

    m_nameResponse.append(response);
    emit generatedNameChanged(QString::fromStdString(m_nameResponse));
    QString gen = QString::fromStdString(m_nameResponse).simplified();
    QStringList words = gen.split(' ', Qt::SkipEmptyParts);
    return words.size() <= 3;
}

bool ChatLLM::handleNameRecalculate(bool isRecalc)
{
#if defined(DEBUG)
    qDebug() << "name recalc" << m_llmThread.objectName() << isRecalc;
#endif
    Q_UNUSED(isRecalc);
    qt_noop();
    return true;
}

bool ChatLLM::handleSystemPrompt(int32_t token)
{
#if defined(DEBUG)
    qDebug() << "system prompt" << m_llmThread.objectName() << token << m_stopGenerating;
#endif
    Q_UNUSED(token);
    return !m_stopGenerating;
}

bool ChatLLM::handleSystemRecalculate(bool isRecalc)
{
#if defined(DEBUG)
    qDebug() << "system recalc" << m_llmThread.objectName() << isRecalc;
#endif
    Q_UNUSED(isRecalc);
    return false;
}

bool ChatLLM::handleRestoreStateFromTextPrompt(int32_t token)
{
#if defined(DEBUG)
    qDebug() << "restore state from text prompt" << m_llmThread.objectName() << token << m_stopGenerating;
#endif
    Q_UNUSED(token);
    return !m_stopGenerating;
}

bool ChatLLM::handleRestoreStateFromTextRecalculate(bool isRecalc)
{
#if defined(DEBUG)
    qDebug() << "restore state from text recalc" << m_llmThread.objectName() << isRecalc;
#endif
    Q_UNUSED(isRecalc);
    return false;
}

// this function serialized the cached model state to disk.
// we want to also serialize n_ctx, and read it at load time.
bool ChatLLM::serialize(QDataStream &stream, int version, bool serializeKV)
{
    if (version > 1) {
        stream << m_llModelType;
        switch (m_llModelType) {
        case GPTJ_: stream << GPTJ_INTERNAL_STATE_VERSION; break;
        case LLAMA_: stream << LLAMA_INTERNAL_STATE_VERSION; break;
        default: Q_UNREACHABLE();
        }
    }
    stream << response();
    stream << generatedName();
    stream << m_promptResponseTokens;

    if (!serializeKV) {
#if defined(DEBUG)
        qDebug() << "serialize" << m_llmThread.objectName() << m_state.size();
#endif
        return stream.status() == QDataStream::Ok;
    }

    if (version <= 3) {
        int responseLogits = 0;
        stream << responseLogits;
    }
    stream << m_ctx.n_past;
    if (version >= 7) {
        stream << m_ctx.n_ctx;
    }
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

bool ChatLLM::deserialize(QDataStream &stream, int version, bool deserializeKV, bool discardKV)
{
    if (version > 1) {
        int internalStateVersion;
        stream >> m_llModelType;
        stream >> internalStateVersion; // for future use
    }
    QString response;
    stream >> response;
    m_response = response.toStdString();
    QString nameResponse;
    stream >> nameResponse;
    m_nameResponse = nameResponse.toStdString();
    stream >> m_promptResponseTokens;

    // If we do not deserialize the KV or it is discarded, then we need to restore the state from the
    // text only. This will be a costly operation, but the chat has to be restored from the text archive
    // alone.
    m_restoreStateFromText = !deserializeKV || discardKV;

    if (!deserializeKV) {
#if defined(DEBUG)
        qDebug() << "deserialize" << m_llmThread.objectName();
#endif
        return stream.status() == QDataStream::Ok;
    }

    if (version <= 3) {
        int responseLogits;
        stream >> responseLogits;
    }

    int32_t n_past;
    stream >> n_past;
    if (!discardKV) m_ctx.n_past = n_past;

    if (version >= 7) {
        uint32_t n_ctx;
        stream >> n_ctx;
        if (!discardKV) m_ctx.n_ctx = n_ctx;
    }

    quint64 logitsSize;
    stream >> logitsSize;
    if (!discardKV) {
        m_ctx.logits.resize(logitsSize);
        stream.readRawData(reinterpret_cast<char*>(m_ctx.logits.data()), logitsSize * sizeof(float));
    } else {
        stream.skipRawData(logitsSize * sizeof(float));
    }

    quint64 tokensSize;
    stream >> tokensSize;
    if (!discardKV) {
        m_ctx.tokens.resize(tokensSize);
        stream.readRawData(reinterpret_cast<char*>(m_ctx.tokens.data()), tokensSize * sizeof(int));
    } else {
        stream.skipRawData(tokensSize * sizeof(int));
    }

    if (version > 0) {
        QByteArray compressed;
        stream >> compressed;
        if (!discardKV)
            m_state = qUncompress(compressed);
    } else {
        if (!discardKV) {
            stream >> m_state;
        } else {
            QByteArray state;
            stream >> state;
        }
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

    if (m_llModelType == LLModelType::API_) {
        m_state.clear();
        QDataStream stream(&m_state, QIODeviceBase::WriteOnly);
        stream.setVersion(QDataStream::Qt_6_4);
        ChatAPI *chatAPI = static_cast<ChatAPI*>(m_llModelInfo.model);
        stream << chatAPI->context();
        return;
    }

    const size_t stateSize = m_llModelInfo.model->stateSize();
    m_state.resize(stateSize);
#if defined(DEBUG)
    qDebug() << "saveState" << m_llmThread.objectName() << "size:" << m_state.size();
#endif
    m_llModelInfo.model->saveState(static_cast<uint8_t*>(reinterpret_cast<void*>(m_state.data())));
}

void ChatLLM::restoreState()
{
    if (!isModelLoaded())
        return;

    if (m_llModelType == LLModelType::API_) {
        QDataStream stream(&m_state, QIODeviceBase::ReadOnly);
        stream.setVersion(QDataStream::Qt_6_4);
        ChatAPI *chatAPI = static_cast<ChatAPI*>(m_llModelInfo.model);
        QList<QString> context;
        stream >> context;
        chatAPI->setContext(context);
        m_state.clear();
        m_state.squeeze();
        return;
    }

#if defined(DEBUG)
    qDebug() << "restoreState" << m_llmThread.objectName() << "size:" << m_state.size();
#endif

    if (m_state.isEmpty())
        return;

    if (m_llModelInfo.model->stateSize() == m_state.size()) {
        m_llModelInfo.model->restoreState(static_cast<const uint8_t*>(reinterpret_cast<void*>(m_state.data())));
        m_processedSystemPrompt = true;
    } else {
        qWarning() << "restoring state from text because" << m_llModelInfo.model->stateSize() << "!=" << m_state.size();
        m_restoreStateFromText = true;
    }

    m_state.clear();
    m_state.squeeze();
}

void ChatLLM::processSystemPrompt()
{
    Q_ASSERT(isModelLoaded());
    if (!isModelLoaded() || m_processedSystemPrompt || m_restoreStateFromText || m_isServer)
        return;

    const std::string systemPrompt = MySettings::globalInstance()->modelSystemPrompt(m_modelInfo).toStdString();
    if (QString::fromStdString(systemPrompt).trimmed().isEmpty()) {
        m_processedSystemPrompt = true;
        return;
    }

    // Start with a whole new context
    m_stopGenerating = false;
    m_ctx = LLModel::PromptContext();

    auto promptFunc = std::bind(&ChatLLM::handleSystemPrompt, this, std::placeholders::_1);
    auto recalcFunc = std::bind(&ChatLLM::handleSystemRecalculate, this, std::placeholders::_1);

    const int32_t n_predict = MySettings::globalInstance()->modelMaxLength(m_modelInfo);
    const int32_t top_k = MySettings::globalInstance()->modelTopK(m_modelInfo);
    const float top_p = MySettings::globalInstance()->modelTopP(m_modelInfo);
    const float min_p = MySettings::globalInstance()->modelMinP(m_modelInfo);
    const float temp = MySettings::globalInstance()->modelTemperature(m_modelInfo);
    const int32_t n_batch = MySettings::globalInstance()->modelPromptBatchSize(m_modelInfo);
    const float repeat_penalty = MySettings::globalInstance()->modelRepeatPenalty(m_modelInfo);
    const int32_t repeat_penalty_tokens = MySettings::globalInstance()->modelRepeatPenaltyTokens(m_modelInfo);
    int n_threads = MySettings::globalInstance()->threadCount();
    m_ctx.n_predict = n_predict;
    m_ctx.top_k = top_k;
    m_ctx.top_p = top_p;
    m_ctx.min_p = min_p;
    m_ctx.temp = temp;
    m_ctx.n_batch = n_batch;
    m_ctx.repeat_penalty = repeat_penalty;
    m_ctx.repeat_last_n = repeat_penalty_tokens;
    m_llModelInfo.model->setThreadCount(n_threads);
#if defined(DEBUG)
    printf("%s", qPrintable(QString::fromStdString(systemPrompt)));
    fflush(stdout);
#endif
    auto old_n_predict = std::exchange(m_ctx.n_predict, 0); // decode system prompt without a response
    m_llModelInfo.model->prompt(systemPrompt, "%1", promptFunc, nullptr, recalcFunc, m_ctx, true);
    m_ctx.n_predict = old_n_predict;
#if defined(DEBUG)
    printf("\n");
    fflush(stdout);
#endif

    m_processedSystemPrompt = m_stopGenerating == false;
}

void ChatLLM::processRestoreStateFromText()
{
    Q_ASSERT(isModelLoaded());
    if (!isModelLoaded() || !m_restoreStateFromText || m_isServer)
        return;

    m_isRecalc = true;
    emit recalcChanged();

    m_stopGenerating = false;
    m_ctx = LLModel::PromptContext();

    auto promptFunc = std::bind(&ChatLLM::handleRestoreStateFromTextPrompt, this, std::placeholders::_1);
    auto recalcFunc = std::bind(&ChatLLM::handleRestoreStateFromTextRecalculate, this, std::placeholders::_1);

    const QString promptTemplate = MySettings::globalInstance()->modelPromptTemplate(m_modelInfo);
    const int32_t n_predict = MySettings::globalInstance()->modelMaxLength(m_modelInfo);
    const int32_t top_k = MySettings::globalInstance()->modelTopK(m_modelInfo);
    const float top_p = MySettings::globalInstance()->modelTopP(m_modelInfo);
    const float min_p = MySettings::globalInstance()->modelMinP(m_modelInfo);
    const float temp = MySettings::globalInstance()->modelTemperature(m_modelInfo);
    const int32_t n_batch = MySettings::globalInstance()->modelPromptBatchSize(m_modelInfo);
    const float repeat_penalty = MySettings::globalInstance()->modelRepeatPenalty(m_modelInfo);
    const int32_t repeat_penalty_tokens = MySettings::globalInstance()->modelRepeatPenaltyTokens(m_modelInfo);
    int n_threads = MySettings::globalInstance()->threadCount();
    m_ctx.n_predict = n_predict;
    m_ctx.top_k = top_k;
    m_ctx.top_p = top_p;
    m_ctx.min_p = min_p;
    m_ctx.temp = temp;
    m_ctx.n_batch = n_batch;
    m_ctx.repeat_penalty = repeat_penalty;
    m_ctx.repeat_last_n = repeat_penalty_tokens;
    m_llModelInfo.model->setThreadCount(n_threads);

    auto it = m_stateFromText.begin();
    while (it < m_stateFromText.end()) {
        auto &prompt = *it++;
        Q_ASSERT(prompt.first == "Prompt: ");
        Q_ASSERT(it < m_stateFromText.end());

        auto &response = *it++;
        Q_ASSERT(response.first != "Prompt: ");
        auto responseText = response.second.toStdString();

        m_llModelInfo.model->prompt(prompt.second.toStdString(), promptTemplate.toStdString(), promptFunc, nullptr,
                                    recalcFunc, m_ctx, false, &responseText);
    }

    if (!m_stopGenerating) {
        m_restoreStateFromText = false;
        m_stateFromText.clear();
    }

    m_isRecalc = false;
    emit recalcChanged();
}
