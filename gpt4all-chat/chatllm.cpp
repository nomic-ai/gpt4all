#include "chatllm.h"

#include "chat.h"
#include "chatapi.h"
#include "localdocssearch.h"
#include "mysettings.h"
#include "network.h"
#include "tool.h"
#include "toolmodel.h"

#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QGlobalStatic>
#include <QGuiApplication>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QMutexLocker>
#include <QSet>
#include <QStringList>
#include <QWaitCondition>
#include <Qt>
#include <QtLogging>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

using namespace Qt::Literals::StringLiterals;
using namespace ToolEnums;

//#define DEBUG
//#define DEBUG_MODEL_LOADING

#define GPTJ_INTERNAL_STATE_VERSION  0 // GPT-J is gone but old chats still use this
#define LLAMA_INTERNAL_STATE_VERSION 0

class LLModelStore {
public:
    static LLModelStore *globalInstance();

    LLModelInfo acquireModel(); // will block until llmodel is ready
    void releaseModel(LLModelInfo &&info); // must be called when you are done
    void destroy();

private:
    LLModelStore()
    {
        // seed with empty model
        m_availableModel = LLModelInfo();
    }
    ~LLModelStore() {}
    std::optional<LLModelInfo> m_availableModel;
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
    while (!m_availableModel)
        m_condition.wait(locker.mutex());
    auto first = std::move(*m_availableModel);
    m_availableModel.reset();
    return first;
}

void LLModelStore::releaseModel(LLModelInfo &&info)
{
    QMutexLocker locker(&m_mutex);
    Q_ASSERT(!m_availableModel);
    m_availableModel = std::move(info);
    m_condition.wakeAll();
}

void LLModelStore::destroy()
{
    QMutexLocker locker(&m_mutex);
    m_availableModel.reset();
}

void LLModelInfo::resetModel(ChatLLM *cllm, LLModel *model) {
    this->model.reset(model);
    fallbackReason.reset();
    emit cllm->loadedModelInfoChanged();
}

ChatLLM::ChatLLM(Chat *parent, bool isServer)
    : QObject{nullptr}
    , m_promptResponseTokens(0)
    , m_promptTokens(0)
    , m_restoringFromText(false)
    , m_shouldBeLoaded(false)
    , m_forceUnloadModel(false)
    , m_markedForDeletion(false)
    , m_stopGenerating(false)
    , m_timer(nullptr)
    , m_isServer(isServer)
    , m_forceMetal(MySettings::globalInstance()->forceMetal())
    , m_reloadingToChangeVariant(false)
    , m_processedSystemPrompt(false)
    , m_restoreStateFromText(false)
    , m_checkToolCall(false)
    , m_maybeToolCall(false)
    , m_foundToolCall(false)
{
    moveToThread(&m_llmThread);
    connect(this, &ChatLLM::shouldBeLoadedChanged, this, &ChatLLM::handleShouldBeLoadedChanged,
        Qt::QueuedConnection); // explicitly queued
    connect(this, &ChatLLM::trySwitchContextRequested, this, &ChatLLM::trySwitchContextOfLoadedModel,
        Qt::QueuedConnection); // explicitly queued
    connect(parent, &Chat::idChanged, this, &ChatLLM::handleChatIdChanged);
    connect(&m_llmThread, &QThread::started, this, &ChatLLM::handleThreadStarted);
    connect(MySettings::globalInstance(), &MySettings::forceMetalChanged, this, &ChatLLM::handleForceMetalChanged);
    connect(MySettings::globalInstance(), &MySettings::deviceChanged, this, &ChatLLM::handleDeviceChanged);
    m_llmThread.setObjectName(parent->id());
    m_llmThread.start();
}

ChatLLM::~ChatLLM()
{
    destroy();
}

void ChatLLM::destroy()
{
    m_stopGenerating = true;
    m_llmThread.quit();
    m_llmThread.wait();

    // The only time we should have a model loaded here is on shutdown
    // as we explicitly unload the model in all other circumstances
    if (isModelLoaded()) {
        m_llModelInfo.resetModel(this);
    }
}

void ChatLLM::destroyStore()
{
    LLModelStore::globalInstance()->destroy();
}

void ChatLLM::handleThreadStarted()
{
    m_timer = new TokenTimer(this);
    connect(m_timer, &TokenTimer::report, this, &ChatLLM::reportSpeed);
    emit threadStarted();
}

void ChatLLM::handleForceMetalChanged(bool forceMetal)
{
#if defined(Q_OS_MAC) && defined(__aarch64__)
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
        emit modelLoadingError(u"Could not find any model to load"_qs);
        return false;
    }
    return loadModel(defaultModel);
}

void ChatLLM::trySwitchContextOfLoadedModel(const ModelInfo &modelInfo)
{
    // We're trying to see if the store already has the model fully loaded that we wish to use
    // and if so we just acquire it from the store and switch the context and return true. If the
    // store doesn't have it or we're already loaded or in any other case just return false.

    // If we're already loaded or a server or we're reloading to change the variant/device or the
    // modelInfo is empty, then this should fail
    if (
        isModelLoaded() || m_isServer || m_reloadingToChangeVariant || modelInfo.name().isEmpty() || !m_shouldBeLoaded
    ) {
        emit trySwitchContextOfLoadedModelCompleted(0);
        return;
    }

    QString filePath = modelInfo.dirpath + modelInfo.filename();
    QFileInfo fileInfo(filePath);

    acquireModel();
#if defined(DEBUG_MODEL_LOADING)
        qDebug() << "acquired model from store" << m_llmThread.objectName() << m_llModelInfo.model.get();
#endif

    // The store gave us no already loaded model, the wrong type of model, then give it back to the
    // store and fail
    if (!m_llModelInfo.model || m_llModelInfo.fileInfo != fileInfo || !m_shouldBeLoaded) {
        LLModelStore::globalInstance()->releaseModel(std::move(m_llModelInfo));
        emit trySwitchContextOfLoadedModelCompleted(0);
        return;
    }

#if defined(DEBUG_MODEL_LOADING)
    qDebug() << "store had our model" << m_llmThread.objectName() << m_llModelInfo.model.get();
#endif

    emit trySwitchContextOfLoadedModelCompleted(2);

    // Restore, signal and process
    restoreState();
    emit modelLoadingPercentageChanged(1.0f);
    emit trySwitchContextOfLoadedModelCompleted(0);
    processSystemPrompt();
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

    // reset status
    emit modelLoadingPercentageChanged(std::numeric_limits<float>::min()); // small non-zero positive value
    emit modelLoadingError("");
    m_pristineLoadedState = false;

    QString filePath = modelInfo.dirpath + modelInfo.filename();
    QFileInfo fileInfo(filePath);

    // We have a live model, but it isn't the one we want
    bool alreadyAcquired = isModelLoaded();
    if (alreadyAcquired) {
        resetContext();
#if defined(DEBUG_MODEL_LOADING)
        qDebug() << "already acquired model deleted" << m_llmThread.objectName() << m_llModelInfo.model.get();
#endif
        m_llModelInfo.resetModel(this);
    } else if (!m_isServer) {
        // This is a blocking call that tries to retrieve the model we need from the model store.
        // If it succeeds, then we just have to restore state. If the store has never had a model
        // returned to it, then the modelInfo.model pointer should be null which will happen on startup
        acquireModel();
#if defined(DEBUG_MODEL_LOADING)
        qDebug() << "acquired model from store" << m_llmThread.objectName() << m_llModelInfo.model.get();
#endif
        // At this point it is possible that while we were blocked waiting to acquire the model from the
        // store, that our state was changed to not be loaded. If this is the case, release the model
        // back into the store and quit loading
        if (!m_shouldBeLoaded) {
#if defined(DEBUG_MODEL_LOADING)
            qDebug() << "no longer need model" << m_llmThread.objectName() << m_llModelInfo.model.get();
#endif
            LLModelStore::globalInstance()->releaseModel(std::move(m_llModelInfo));
            emit modelLoadingPercentageChanged(0.0f);
            return false;
        }

        // Check if the store just gave us exactly the model we were looking for
        if (m_llModelInfo.model && m_llModelInfo.fileInfo == fileInfo && !m_reloadingToChangeVariant) {
#if defined(DEBUG_MODEL_LOADING)
            qDebug() << "store had our model" << m_llmThread.objectName() << m_llModelInfo.model.get();
#endif
            restoreState();
            emit modelLoadingPercentageChanged(1.0f);
            setModelInfo(modelInfo);
            Q_ASSERT(!m_modelInfo.filename().isEmpty());
            if (m_modelInfo.filename().isEmpty())
                emit modelLoadingError(u"Modelinfo is left null for %1"_s.arg(modelInfo.filename()));
            else
                processSystemPrompt();
            return true;
        } else {
            // Release the memory since we have to switch to a different model.
#if defined(DEBUG_MODEL_LOADING)
            qDebug() << "deleting model" << m_llmThread.objectName() << m_llModelInfo.model.get();
#endif
            m_llModelInfo.resetModel(this);
        }
    }

    // Guarantee we've released the previous models memory
    Q_ASSERT(!m_llModelInfo.model);

    // Store the file info in the modelInfo in case we have an error loading
    m_llModelInfo.fileInfo = fileInfo;

    if (fileInfo.exists()) {
        QVariantMap modelLoadProps;
        if (modelInfo.isOnline) {
            QString apiKey;
            QString requestUrl;
            QString modelName;
            {
                QFile file(filePath);
                bool success = file.open(QIODeviceBase::ReadOnly);
                (void)success;
                Q_ASSERT(success);
                QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                QJsonObject obj = doc.object();
                apiKey = obj["apiKey"].toString();
                modelName = obj["modelName"].toString();
                if (modelInfo.isCompatibleApi) {
                    QString baseUrl(obj["baseUrl"].toString());
                    QUrl apiUrl(QUrl::fromUserInput(baseUrl));
                    if (!Network::isHttpUrlValid(apiUrl)) {
                        return false;
                    }
                    QString currentPath(apiUrl.path());
                    QString suffixPath("%1/chat/completions");
                    apiUrl.setPath(suffixPath.arg(currentPath));
                    requestUrl = apiUrl.toString();
                } else {
                    requestUrl = modelInfo.url();
                }
            }
            m_llModelType = LLModelType::API_;
            ChatAPI *model = new ChatAPI();
            model->setModelName(modelName);
            model->setRequestURL(requestUrl);
            model->setAPIKey(apiKey);
            m_llModelInfo.resetModel(this, model);
        } else if (!loadNewModel(modelInfo, modelLoadProps)) {
            return false; // m_shouldBeLoaded became false
        }
#if defined(DEBUG_MODEL_LOADING)
        qDebug() << "new model" << m_llmThread.objectName() << m_llModelInfo.model.get();
#endif
        restoreState();
#if defined(DEBUG)
        qDebug() << "modelLoadedChanged" << m_llmThread.objectName();
        fflush(stdout);
#endif
        emit modelLoadingPercentageChanged(isModelLoaded() ? 1.0f : 0.0f);
        emit loadedModelInfoChanged();

        modelLoadProps.insert("requestedDevice", MySettings::globalInstance()->device());
        modelLoadProps.insert("model", modelInfo.filename());
        Network::globalInstance()->trackChatEvent("model_load", modelLoadProps);
    } else {
        if (!m_isServer)
            LLModelStore::globalInstance()->releaseModel(std::move(m_llModelInfo)); // release back into the store
        resetModel();
        emit modelLoadingError(u"Could not find file for model %1"_s.arg(modelInfo.filename()));
    }

    if (m_llModelInfo.model) {
        setModelInfo(modelInfo);
        processSystemPrompt();
    }
    return bool(m_llModelInfo.model);
}

/* Returns false if the model should no longer be loaded (!m_shouldBeLoaded).
 * Otherwise returns true, even on error. */
bool ChatLLM::loadNewModel(const ModelInfo &modelInfo, QVariantMap &modelLoadProps)
{
    QElapsedTimer modelLoadTimer;
    modelLoadTimer.start();

    QString requestedDevice = MySettings::globalInstance()->device();
    int n_ctx = MySettings::globalInstance()->modelContextLength(modelInfo);
    m_ctx.n_ctx = n_ctx;
    int ngl = MySettings::globalInstance()->modelGpuLayers(modelInfo);

    std::string backend = "auto";
#ifdef Q_OS_MAC
    if (requestedDevice == "CPU") {
        backend = "cpu";
    } else if (m_forceMetal) {
#ifdef __aarch64__
        backend = "metal";
#endif
    }
#else // !defined(Q_OS_MAC)
    if (requestedDevice.startsWith("CUDA: "))
        backend = "cuda";
#endif

    QString filePath = modelInfo.dirpath + modelInfo.filename();

    auto construct = [this, &filePath, &modelInfo, &modelLoadProps, n_ctx](std::string const &backend) {
        QString constructError;
        m_llModelInfo.resetModel(this);
        try {
            auto *model = LLModel::Implementation::construct(filePath.toStdString(), backend, n_ctx);
            m_llModelInfo.resetModel(this, model);
        } catch (const LLModel::MissingImplementationError &e) {
            modelLoadProps.insert("error", "missing_model_impl");
            constructError = e.what();
        } catch (const LLModel::UnsupportedModelError &e) {
            modelLoadProps.insert("error", "unsupported_model_file");
            constructError = e.what();
        } catch (const LLModel::BadArchError &e) {
            constructError = e.what();
            modelLoadProps.insert("error", "unsupported_model_arch");
            modelLoadProps.insert("model_arch", QString::fromStdString(e.arch()));
        }

        if (!m_llModelInfo.model) {
            if (!m_isServer)
                LLModelStore::globalInstance()->releaseModel(std::move(m_llModelInfo));
            resetModel();
            emit modelLoadingError(u"Error loading %1: %2"_s.arg(modelInfo.filename(), constructError));
            return false;
        }

        m_llModelInfo.model->setProgressCallback([this](float progress) -> bool {
            progress = std::max(progress, std::numeric_limits<float>::min()); // keep progress above zero
            emit modelLoadingPercentageChanged(progress);
            return m_shouldBeLoaded;
        });
        return true;
    };

    if (!construct(backend))
        return true;

    if (m_llModelInfo.model->isModelBlacklisted(filePath.toStdString())) {
        static QSet<QString> warned;
        auto fname = modelInfo.filename();
        if (!warned.contains(fname)) {
            emit modelLoadingWarning(
                u"%1 is known to be broken. Please get a replacement via the download dialog."_s.arg(fname)
            );
            warned.insert(fname); // don't warn again until restart
        }
    }

    auto approxDeviceMemGB = [](const LLModel::GPUDevice *dev) {
        float memGB = dev->heapSize / float(1024 * 1024 * 1024);
        return std::floor(memGB * 10.f) / 10.f; // truncate to 1 decimal place
    };

    std::vector<LLModel::GPUDevice> availableDevices;
    const LLModel::GPUDevice *defaultDevice = nullptr;
    {
        const size_t requiredMemory = m_llModelInfo.model->requiredMem(filePath.toStdString(), n_ctx, ngl);
        availableDevices = m_llModelInfo.model->availableGPUDevices(requiredMemory);
        // Pick the best device
        // NB: relies on the fact that Kompute devices are listed first
        if (!availableDevices.empty() && availableDevices.front().type == 2 /*a discrete gpu*/) {
            defaultDevice = &availableDevices.front();
            float memGB = defaultDevice->heapSize / float(1024 * 1024 * 1024);
            memGB = std::floor(memGB * 10.f) / 10.f; // truncate to 1 decimal place
            modelLoadProps.insert("default_device", QString::fromStdString(defaultDevice->name));
            modelLoadProps.insert("default_device_mem", approxDeviceMemGB(defaultDevice));
            modelLoadProps.insert("default_device_backend", QString::fromStdString(defaultDevice->backendName()));
        }
    }

    bool actualDeviceIsCPU = true;

#if defined(Q_OS_MAC) && defined(__aarch64__)
    if (m_llModelInfo.model->implementation().buildVariant() == "metal")
        actualDeviceIsCPU = false;
#else
    if (requestedDevice != "CPU") {
        const auto *device = defaultDevice;
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
        } else if (!m_llModelInfo.model->initializeGPUDevice(device->index, &unavail_reason)) {
            m_llModelInfo.fallbackReason = QString::fromStdString(unavail_reason);
        } else {
            actualDeviceIsCPU = false;
            modelLoadProps.insert("requested_device_mem", approxDeviceMemGB(device));
        }
    }
#endif

    bool success = m_llModelInfo.model->loadModel(filePath.toStdString(), n_ctx, ngl);

    if (!m_shouldBeLoaded) {
        m_llModelInfo.resetModel(this);
        if (!m_isServer)
            LLModelStore::globalInstance()->releaseModel(std::move(m_llModelInfo));
        resetModel();
        emit modelLoadingPercentageChanged(0.0f);
        return false;
    }

    if (actualDeviceIsCPU) {
        // we asked llama.cpp to use the CPU
    } else if (!success) {
        // llama_init_from_file returned nullptr
        m_llModelInfo.fallbackReason = "GPU loading failed (out of VRAM?)";
        modelLoadProps.insert("cpu_fallback_reason", "gpu_load_failed");

        // For CUDA, make sure we don't use the GPU at all - ngl=0 still offloads matmuls
        if (backend == "cuda" && !construct("auto"))
            return true;

        success = m_llModelInfo.model->loadModel(filePath.toStdString(), n_ctx, 0);

        if (!m_shouldBeLoaded) {
            m_llModelInfo.resetModel(this);
            if (!m_isServer)
                LLModelStore::globalInstance()->releaseModel(std::move(m_llModelInfo));
            resetModel();
            emit modelLoadingPercentageChanged(0.0f);
            return false;
        }
    } else if (!m_llModelInfo.model->usingGPUDevice()) {
        // ggml_vk_init was not called in llama.cpp
        // We might have had to fallback to CPU after load if the model is not possible to accelerate
        // for instance if the quantization method is not supported on Vulkan yet
        m_llModelInfo.fallbackReason = "model or quant has no GPU support";
        modelLoadProps.insert("cpu_fallback_reason", "gpu_unsupported_model");
    }

    if (!success) {
        m_llModelInfo.resetModel(this);
        if (!m_isServer)
            LLModelStore::globalInstance()->releaseModel(std::move(m_llModelInfo));
        resetModel();
        emit modelLoadingError(u"Could not load model due to invalid model file for %1"_s.arg(modelInfo.filename()));
        modelLoadProps.insert("error", "loadmodel_failed");
        return true;
    }

    switch (m_llModelInfo.model->implementation().modelType()[0]) {
    case 'L': m_llModelType = LLModelType::LLAMA_; break;
    default:
        {
            m_llModelInfo.resetModel(this);
            if (!m_isServer)
                LLModelStore::globalInstance()->releaseModel(std::move(m_llModelInfo));
            resetModel();
            emit modelLoadingError(u"Could not determine model type for %1"_s.arg(modelInfo.filename()));
        }
    }

    modelLoadProps.insert("$duration", modelLoadTimer.elapsed() / 1000.);
    return true;
};

bool ChatLLM::isModelLoaded() const
{
    return m_llModelInfo.model && m_llModelInfo.model->isModelLoaded();
}

std::string remove_leading_whitespace(const std::string& input)
{
    auto first_non_whitespace = std::find_if(input.begin(), input.end(), [](unsigned char c) {
        return !std::isspace(c);
    });

    if (first_non_whitespace == input.end())
        return std::string();

    return std::string(first_non_whitespace, input.end());
}

std::string trim_whitespace(const std::string& input)
{
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

// FIXME(jared): we don't actually have to re-decode the prompt to generate a new response
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

void ChatLLM::acquireModel() {
    m_llModelInfo = LLModelStore::globalInstance()->acquireModel();
    emit loadedModelInfoChanged();
}

void ChatLLM::resetModel() {
    m_llModelInfo = {};
    emit loadedModelInfoChanged();
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

    // If we're checking for a tool called and the response is equal or exceeds 11 chars
    // then we check
    if (m_checkToolCall && m_response.size() >= 11) {
        if (m_response.starts_with("<tool_call>")) {
            m_maybeToolCall = true;
            m_response.erase(0, 11);
        }
        m_checkToolCall = false;
    }

    // Check if we're at the end of tool call and erase the end tag
    if (m_maybeToolCall && m_response.ends_with("</tool_call>")) {
        m_foundToolCall = true;
        m_response.erase(m_response.length() - 12);
        return false;
    }

    // If we're not checking for tool call and haven't detected one, then send along the response
    if (!m_checkToolCall && !m_maybeToolCall)
        emit responseChanged(QString::fromStdString(remove_leading_whitespace(m_response)));

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
    QString toolCallingTemplate = MySettings::globalInstance()->modelToolTemplate(m_modelInfo);
    Q_ASSERT(toolCallingTemplate.isEmpty() || toolCallingTemplate.contains("%1"));
    if (toolCallingTemplate.isEmpty() || !toolCallingTemplate.contains("%1"))
        toolCallingTemplate = u"### Context:\n%1\n\n"_s;

    const bool isToolCallingModel = MySettings::globalInstance()->modelIsToolCalling(m_modelInfo);

    // Iterate over the list of tools and if force usage is set, then we *try* and force usage here
    QList<QString> toolResponses;
    qint64 totalTime = 0;
    bool producedSourceExcerpts = false;
    const int toolCount = ToolModel::globalInstance()->count();
    for (int i = 0; i < toolCount; ++i) {
        Tool *t = ToolModel::globalInstance()->get(i);
        if (t->usageMode() != UsageMode::ForceUsage)
            continue;

        // Local docs search is unique. It is the _only_ tool where we try and force usage even if
        // the model does not support tool calling.
        if (!isToolCallingModel && t->function() != "localdocs_search")
            continue;

        // If this is the localdocs tool call, then we perform the search with the entire prompt as
        // the query
        if (t->function() == "localdocs_search") {
            if (collectionList.isEmpty())
                continue;

            QJsonObject parameters;
            parameters.insert("collections", QJsonArray::fromStringList(collectionList));
            parameters.insert("query", prompt);
            parameters.insert("count", MySettings::globalInstance()->localDocsRetrievalSize());

            // FIXME: Honor the confirmation mode feature
            const QString response = t->run(parameters, 2000 /*msecs to timeout*/);
            if (t->error() != Error::NoError) {
                qWarning() << "ERROR: LocalDocs call produced error:" << t->errorString();
                continue;
            }

            QString parseError;
            QList<SourceExcerpt> localDocsExcerpts = SourceExcerpt::fromJson(response, parseError);
            if (!parseError.isEmpty()) {
                qWarning() << "ERROR: Could not parse source excerpts for localdocs response:" << parseError;
            } else {
                producedSourceExcerpts = true;
                emit sourceExcerptsChanged(localDocsExcerpts);
            }
            toolResponses << QString(toolCallingTemplate).arg(response);
            continue;
        }

        // For all other cases we should have a tool calling model
        Q_ASSERT(isToolCallingModel);

        // Create the tool calling response as if the model has chosen this particular tool
        const QString toolCallingResponse = QString("<tool_call>{\"name\": \"%1\", \"parameters\": {\"").arg(t->function());

        // Mimic that the model has already responded like this to trigger our tool calling detection
        // code and then rely upon it to complete the parameters correctly
        m_response = toolCallingResponse.toStdString();

        // Insert this response as the tool prompt
        const QString toolPrompt = QString(promptTemplate).arg(prompt, toolCallingResponse);

        const QString toolCall = completeToolCall(toolPrompt, n_predict, top_k, top_p,
            min_p, temp, n_batch, repeat_penalty, repeat_penalty_tokens, totalTime);

        // If the tool call is empty, then we failed in our attempt to force usage
        if (toolCall.isEmpty()) {
            qWarning() << "WARNING: Attempt to force usage of toolcall" << t->function() << "failed:"
                << "model could not complete parameters for" << toolPrompt;
            continue;
        }

        QString errorString;
        const QString response = executeToolCall(toolCall, producedSourceExcerpts, errorString);
        if (response.isEmpty()) {
            qWarning() << "WARNING: Attempt to force usage of toolcall" << t->function() << "failed:" << errorString;
            continue;
        }

        toolResponses << QString(toolCallingTemplate).arg(response);
    }

    bool success = promptRecursive({ toolResponses }, prompt, promptTemplate, n_predict, top_k, top_p,
        min_p, temp, n_batch, repeat_penalty, repeat_penalty_tokens, totalTime, producedSourceExcerpts);
    Q_ASSERT(success);
    SuggestionMode mode = MySettings::globalInstance()->suggestionMode();
    if (mode == SuggestionMode::On || (mode == SuggestionMode::SourceExcerptsOnly && producedSourceExcerpts))
        generateQuestions(totalTime);
    else
        emit responseStopped(totalTime);
    return success;
}

QString ChatLLM::completeToolCall(const QString &prompt, int32_t n_predict, int32_t top_k, float top_p,
    float min_p, float temp, int32_t n_batch, float repeat_penalty, int32_t repeat_penalty_tokens,
    qint64 &totalTime)
{
    if (!isModelLoaded())
        return QString();

    int n_threads = MySettings::globalInstance()->threadCount();

    m_stopGenerating = false;
    auto promptFunc = std::bind(&ChatLLM::handlePrompt, this, std::placeholders::_1);
    auto responseFunc = std::bind(&ChatLLM::handleResponse, this, std::placeholders::_1,
        std::placeholders::_2);
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

    QElapsedTimer elapsedTimer;
    elapsedTimer.start();
    m_timer->start();

    m_checkToolCall = true;

    // We pass in the prompt as the completed template as we're mimicking that the respone has already
    // started
    LLModel::PromptContext ctx = m_ctx;
    m_llModelInfo.model->prompt(prompt.toStdString(), "%1", promptFunc, responseFunc,
                                /*allowContextShift*/ false, ctx);

    // After the response has been handled reset this state
    m_checkToolCall = false;
    m_maybeToolCall = false;

    m_timer->stop();
    totalTime = elapsedTimer.elapsed();

    const QString toolCall = QString::fromStdString(trim_whitespace(m_response));
    m_promptResponseTokens = 0;
    m_promptTokens = 0;
    m_response = std::string();

    if (!m_foundToolCall)
        return QString();

    m_foundToolCall = false;
    return toolCall;
}

QString ChatLLM::executeToolCall(const QString &toolCall, bool &producedSourceExcerpts, QString &errorString)
{
    const QString toolTemplate = MySettings::globalInstance()->modelToolTemplate(m_modelInfo);
    if (toolTemplate.isEmpty()) {
        errorString = QString("ERROR: No valid tool template for this model %1").arg(toolCall);
        return QString();
    }

    QJsonParseError err;
    const QJsonDocument toolCallDoc = QJsonDocument::fromJson(toolCall.toUtf8(), &err);

    if (toolCallDoc.isNull() || err.error != QJsonParseError::NoError || !toolCallDoc.isObject()) {
        errorString = QString("ERROR: The tool call had null or invalid json %1").arg(toolCall);
        return QString();
    }

    QJsonObject rootObject = toolCallDoc.object();
    if (!rootObject.contains("name") || !rootObject.contains("parameters")) {
        errorString = QString("ERROR: The tool call did not have required name and argument objects %1").arg(toolCall);
        return QString();
    }

    const QString tool = toolCallDoc["name"].toString();
    const QJsonObject args = toolCallDoc["parameters"].toObject();

    Tool *toolInstance = ToolModel::globalInstance()->get(tool);
    if (!toolInstance) {
        errorString = QString("ERROR: Could not find the tool for %1").arg(toolCall);
        return QString();
    }

    // FIXME: Honor the confirmation mode feature
    // Inform the chat that we're executing a tool call
    emit toolCalled(toolInstance->name().toLower());

    const QString response = toolInstance->run(args, 2000 /*msecs to timeout*/);
    if (toolInstance->error() != Error::NoError) {
        errorString = QString("ERROR: Tool call produced error: %1").arg(toolInstance->errorString());
        return QString();
    }

    // If the tool supports excerpts then try to parse them here, but it isn't strictly an error
    // but rather a warning
    if (toolInstance->excerpts()) {
        QString parseError;
        QList<SourceExcerpt> sourceExcerpts = SourceExcerpt::fromJson(response, parseError);
        if (!parseError.isEmpty()) {
            qWarning() << "WARNING: Could not parse source excerpts for response:" << parseError;
        } else if (!sourceExcerpts.isEmpty()) {
            producedSourceExcerpts = true;
            emit sourceExcerptsChanged(sourceExcerpts);
        }
    }
    return response;
}

bool ChatLLM::promptRecursive(const QList<QString> &toolResponses, const QString &prompt,
    const QString &promptTemplate, int32_t n_predict, int32_t top_k, float top_p, float min_p, float temp,
    int32_t n_batch, float repeat_penalty, int32_t repeat_penalty_tokens, qint64 &totalTime, bool &producedSourceExcerpts, bool isRecursiveCall)
{
    if (!isModelLoaded())
        return false;

    int n_threads = MySettings::globalInstance()->threadCount();

    m_stopGenerating = false;
    auto promptFunc = std::bind(&ChatLLM::handlePrompt, this, std::placeholders::_1);
    auto responseFunc = std::bind(&ChatLLM::handleResponse, this, std::placeholders::_1,
        std::placeholders::_2);
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

    QElapsedTimer elapsedTimer;
    elapsedTimer.start();
    m_timer->start();

    // The list of possible additional responses that come from previous usage of tool calls
    for (const QString &context : toolResponses) {
        auto old_n_predict = std::exchange(m_ctx.n_predict, 0); // decode context without a response
        m_llModelInfo.model->prompt(context.toStdString(), "%1", promptFunc, responseFunc,
                                    /*allowContextShift*/ true, m_ctx);
        m_ctx.n_predict = old_n_predict; // now we are ready for a response
    }

    // We can't handle recursive tool calls right now due to the possibility of the model causing
    // infinite recursion through repeated tool calls
    m_checkToolCall = !isRecursiveCall;

    m_llModelInfo.model->prompt(prompt.toStdString(), promptTemplate.toStdString(), promptFunc, responseFunc,
                                /*allowContextShift*/ true, m_ctx);

    // After the response has been handled reset this state
    m_checkToolCall = false;
    m_maybeToolCall = false;

#if defined(DEBUG)
    printf("\n");
    fflush(stdout);
#endif
    m_timer->stop();
    totalTime = elapsedTimer.elapsed();
    std::string trimmed = trim_whitespace(m_response);

    // If we found a tool call, then deal with it
    if (m_foundToolCall) {
        m_foundToolCall = false;

        QString errorString;
        const QString toolCall = QString::fromStdString(trimmed);
        const QString toolResponse = executeToolCall(toolCall, producedSourceExcerpts, errorString);
        if (toolResponse.isEmpty()) {
            // FIXME: Need to surface errors to the UI
            // Restore the strings that we excluded previously when detecting the tool call
            qWarning() << errorString;
            m_response = "<tool_call>" + toolCall.toStdString() + "</tool_call>";
            emit responseChanged(QString::fromStdString(m_response));
            emit responseStopped(totalTime);
            m_pristineLoadedState = false;
            return false;
        }

        // Reset the state now that we've had a successful tool call response
        m_promptResponseTokens = 0;
        m_promptTokens = 0;
        m_response = std::string();

        // This is a recursive call but flag is checked above to arrest infinite recursive tool calls
        return promptRecursive({ toolResponse }, prompt, promptTemplate,
            n_predict, top_k, top_p, min_p, temp, n_batch, repeat_penalty, repeat_penalty_tokens, totalTime,
            producedSourceExcerpts, true /*isRecursiveCall*/);
    } else {
        if (trimmed != m_response) {
            m_response = trimmed;
            emit responseChanged(QString::fromStdString(m_response));
        }
        m_pristineLoadedState = false;
        return true;
    }
}

void ChatLLM::setShouldBeLoaded(bool b)
{
#if defined(DEBUG_MODEL_LOADING)
    qDebug() << "setShouldBeLoaded" << m_llmThread.objectName() << b << m_llModelInfo.model.get();
#endif
    m_shouldBeLoaded = b; // atomic
    emit shouldBeLoadedChanged();
}

void ChatLLM::requestTrySwitchContext()
{
    m_shouldBeLoaded = true; // atomic
    emit trySwitchContextRequested(modelInfo());
}

void ChatLLM::handleShouldBeLoadedChanged()
{
    if (m_shouldBeLoaded)
        reloadModel();
    else
        unloadModel();
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
    qDebug() << "unloadModel" << m_llmThread.objectName() << m_llModelInfo.model.get();
#endif

    if (m_forceUnloadModel) {
        m_llModelInfo.resetModel(this);
        m_forceUnloadModel = false;
    }

    LLModelStore::globalInstance()->releaseModel(std::move(m_llModelInfo));
    m_pristineLoadedState = false;
}

void ChatLLM::reloadModel()
{
    if (isModelLoaded() && m_forceUnloadModel)
        unloadModel(); // we unload first if we are forcing an unload

    if (isModelLoaded() || m_isServer)
        return;

#if defined(DEBUG_MODEL_LOADING)
    qDebug() << "reloadModel" << m_llmThread.objectName() << m_llModelInfo.model.get();
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

    const QString chatNamePrompt = MySettings::globalInstance()->modelChatNamePrompt(m_modelInfo);
    if (chatNamePrompt.trimmed().isEmpty()) {
        qWarning() << "ChatLLM: not generating chat name because prompt is empty";
        return;
    }

    auto promptTemplate = MySettings::globalInstance()->modelPromptTemplate(m_modelInfo);
    auto promptFunc = std::bind(&ChatLLM::handleNamePrompt, this, std::placeholders::_1);
    auto responseFunc = std::bind(&ChatLLM::handleNameResponse, this, std::placeholders::_1, std::placeholders::_2);
    LLModel::PromptContext ctx = m_ctx;
    m_llModelInfo.model->prompt(chatNamePrompt.toStdString(), promptTemplate.toStdString(),
                                promptFunc, responseFunc, /*allowContextShift*/ false, ctx);
    std::string trimmed = trim_whitespace(m_nameResponse);
    if (trimmed != m_nameResponse) {
        m_nameResponse = trimmed;
        emit generatedNameChanged(QString::fromStdString(m_nameResponse));
    }
    m_pristineLoadedState = false;
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

bool ChatLLM::handleQuestionPrompt(int32_t token)
{
#if defined(DEBUG)
    qDebug() << "question prompt" << m_llmThread.objectName() << token;
#endif
    Q_UNUSED(token);
    return !m_stopGenerating;
}

bool ChatLLM::handleQuestionResponse(int32_t token, const std::string &response)
{
#if defined(DEBUG)
    qDebug() << "question response" << m_llmThread.objectName() << token << response;
#endif
    Q_UNUSED(token);

    // add token to buffer
    m_questionResponse.append(response);

    // match whole question sentences
    // FIXME: This only works with response by the model in english which is not ideal for a multi-language
    // model.
    static const QRegularExpression reQuestion(R"(\b(What|Where|How|Why|When|Who|Which|Whose|Whom)\b[^?]*\?)");

    // extract all questions from response
    int lastMatchEnd = -1;
    for (const auto &match : reQuestion.globalMatch(m_questionResponse)) {
        lastMatchEnd = match.capturedEnd();
        emit generatedQuestionFinished(match.captured());
    }

    // remove processed input from buffer
    if (lastMatchEnd != -1)
        m_questionResponse.erase(m_questionResponse.cbegin(), m_questionResponse.cbegin() + lastMatchEnd);

    return true;
}

void ChatLLM::generateQuestions(qint64 elapsed)
{
    Q_ASSERT(isModelLoaded());
    if (!isModelLoaded()) {
        emit responseStopped(elapsed);
        return;
    }

    const std::string suggestedFollowUpPrompt = MySettings::globalInstance()->modelSuggestedFollowUpPrompt(m_modelInfo).toStdString();
    if (QString::fromStdString(suggestedFollowUpPrompt).trimmed().isEmpty()) {
        emit responseStopped(elapsed);
        return;
    }

    emit generatingQuestions();
    m_questionResponse.clear();
    auto promptTemplate = MySettings::globalInstance()->modelPromptTemplate(m_modelInfo);
    auto promptFunc = std::bind(&ChatLLM::handleQuestionPrompt, this, std::placeholders::_1);
    auto responseFunc = std::bind(&ChatLLM::handleQuestionResponse, this, std::placeholders::_1, std::placeholders::_2);
    LLModel::PromptContext ctx = m_ctx;
    QElapsedTimer totalTime;
    totalTime.start();
    m_llModelInfo.model->prompt(suggestedFollowUpPrompt, promptTemplate.toStdString(), promptFunc, responseFunc,
                                /*allowContextShift*/ false, ctx);
    elapsed += totalTime.elapsed();
    emit responseStopped(elapsed);
}


bool ChatLLM::handleSystemPrompt(int32_t token)
{
#if defined(DEBUG)
    qDebug() << "system prompt" << m_llmThread.objectName() << token << m_stopGenerating;
#endif
    Q_UNUSED(token);
    return !m_stopGenerating;
}

bool ChatLLM::handleRestoreStateFromTextPrompt(int32_t token)
{
#if defined(DEBUG)
    qDebug() << "restore state from text prompt" << m_llmThread.objectName() << token << m_stopGenerating;
#endif
    Q_UNUSED(token);
    return !m_stopGenerating;
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
    if (!deserializeKV || discardKV) {
        m_restoreStateFromText = true;
        m_pristineLoadedState = true;
    }

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

    if (version < 9) {
        quint64 logitsSize;
        stream >> logitsSize;
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
    if (!isModelLoaded() || m_pristineLoadedState)
        return;

    if (m_llModelType == LLModelType::API_) {
        m_state.clear();
        QDataStream stream(&m_state, QIODeviceBase::WriteOnly);
        stream.setVersion(QDataStream::Qt_6_4);
        ChatAPI *chatAPI = static_cast<ChatAPI*>(m_llModelInfo.model.get());
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
        ChatAPI *chatAPI = static_cast<ChatAPI*>(m_llModelInfo.model.get());
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
        m_pristineLoadedState = true;
    } else {
        qWarning() << "restoring state from text because" << m_llModelInfo.model->stateSize() << "!=" << m_state.size();
        m_restoreStateFromText = true;
    }

    // free local state copy unless unload is pending
    if (m_shouldBeLoaded) {
        m_state.clear();
        m_state.squeeze();
        m_pristineLoadedState = false;
    }
}

void ChatLLM::processSystemPrompt()
{
    Q_ASSERT(isModelLoaded());
    if (!isModelLoaded() || m_processedSystemPrompt || m_restoreStateFromText || m_isServer)
        return;

    QString error;
    const std::string systemPrompt = MySettings::globalInstance()->modelSystemPrompt(m_modelInfo, error).toStdString();

    // The GUI should not allow setting an improper template, but it is always possible someone hand
    // edits the settings file to produce an improper one.
    Q_ASSERT(error.isEmpty());
    if (!error.isEmpty())
        qWarning() << "ERROR: Could not parse system prompt template:" << error;

    if (QString::fromStdString(systemPrompt).trimmed().isEmpty()) {
        m_processedSystemPrompt = true;
        return;
    }

    // Start with a whole new context
    m_stopGenerating = false;
    m_ctx = LLModel::PromptContext();

    auto promptFunc = std::bind(&ChatLLM::handleSystemPrompt, this, std::placeholders::_1);

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
    // use "%1%2" and not "%1" to avoid implicit whitespace
    m_llModelInfo.model->prompt(systemPrompt, "%1%2", promptFunc, nullptr, /*allowContextShift*/ true, m_ctx, true);
    m_ctx.n_predict = old_n_predict;
#if defined(DEBUG)
    printf("\n");
    fflush(stdout);
#endif

    m_processedSystemPrompt = m_stopGenerating == false;
    m_pristineLoadedState = false;
}

void ChatLLM::processRestoreStateFromText()
{
    Q_ASSERT(isModelLoaded());
    if (!isModelLoaded() || !m_restoreStateFromText || m_isServer)
        return;

    m_restoringFromText = true;
    emit restoringFromTextChanged();

    m_stopGenerating = false;
    m_ctx = LLModel::PromptContext();

    auto promptFunc = std::bind(&ChatLLM::handleRestoreStateFromTextPrompt, this, std::placeholders::_1);

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
                                    /*allowContextShift*/ true, m_ctx, false, &responseText);
    }

    if (!m_stopGenerating) {
        m_restoreStateFromText = false;
        m_stateFromText.clear();
    }

    m_restoringFromText = false;
    emit restoringFromTextChanged();

    m_pristineLoadedState = false;
}
