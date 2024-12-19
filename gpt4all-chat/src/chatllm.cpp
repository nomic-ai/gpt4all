#include "chatllm.h"

#include "chat.h"
#include "chatapi.h"
#include "chatmodel.h"
#include "jinja_helpers.h"
#include "localdocs.h"
#include "mysettings.h"
#include "network.h"
#include "tool.h"
#include "toolmodel.h"
#include "toolcallparser.h"

#include <fmt/format.h>

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QGlobalStatic>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMap>
#include <QMutex>
#include <QMutexLocker> // IWYU pragma: keep
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QSet>
#include <QUrl>
#include <QWaitCondition>
#include <Qt>
#include <QtLogging>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <ctime>
#include <exception>
#include <iomanip>
#include <limits>
#include <optional>
#include <ranges>
#include <regex>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

using namespace Qt::Literals::StringLiterals;
using namespace ToolEnums;
namespace ranges = std::ranges;

//#define DEBUG
//#define DEBUG_MODEL_LOADING

// NOTE: not threadsafe
static jinja2::TemplateEnv *jinjaEnv()
{
    static std::optional<jinja2::TemplateEnv> environment;
    if (!environment) {
        auto &env = environment.emplace();
        auto &settings = env.GetSettings();
        settings.trimBlocks = true;
        settings.lstripBlocks = true;
        env.AddGlobal("raise_exception", jinja2::UserCallable(
            /*callable*/ [](auto &params) -> jinja2::Value {
                auto messageArg = params.args.find("message");
                if (messageArg == params.args.end() || !messageArg->second.isString())
                    throw std::runtime_error("'message' argument to raise_exception() must be a string");
                throw std::runtime_error(fmt::format("Jinja template error: {}", messageArg->second.asString()));
            },
            /*argsInfo*/ { jinja2::ArgInfo("message", /*isMandatory*/ true) }
        ));
        env.AddGlobal("strftime_now", jinja2::UserCallable(
            /*callable*/ [](auto &params) -> jinja2::Value {
                using Clock = std::chrono::system_clock;
                auto formatArg = params.args.find("format");
                if (formatArg == params.args.end() || !formatArg->second.isString())
                    throw std::runtime_error("'format' argument to strftime_now() must be a string");
                time_t nowUnix = Clock::to_time_t(Clock::now());
                auto localDate = *std::localtime(&nowUnix);
                std::ostringstream ss;
                ss << std::put_time(&localDate, formatArg->second.asString().c_str());
                return ss.str();
            },
            /*argsInfo*/ { jinja2::ArgInfo("format", /*isMandatory*/ true) }
        ));
    }
    return &*environment;
}

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
    , m_chat(parent)
    , m_shouldBeLoaded(false)
    , m_forceUnloadModel(false)
    , m_markedForDeletion(false)
    , m_stopGenerating(false)
    , m_timer(nullptr)
    , m_isServer(isServer)
    , m_forceMetal(MySettings::globalInstance()->forceMetal())
    , m_reloadingToChangeVariant(false)
    , m_chatModel(parent->chatModel())
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
#else
    Q_UNUSED(forceMetal);
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
        emit modelLoadingError(u"Could not find any model to load"_s);
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
    emit modelLoadingPercentageChanged(1.0f);
    emit trySwitchContextOfLoadedModelCompleted(0);
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

    if (isModelLoaded() && this->modelInfo() == modelInfo) {
        // already acquired -> keep it
        return true; // already loaded
    }

    // reset status
    emit modelLoadingPercentageChanged(std::numeric_limits<float>::min()); // small non-zero positive value
    emit modelLoadingError("");

    QString filePath = modelInfo.dirpath + modelInfo.filename();
    QFileInfo fileInfo(filePath);

    // We have a live model, but it isn't the one we want
    bool alreadyAcquired = isModelLoaded();
    if (alreadyAcquired) {
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
            emit modelLoadingPercentageChanged(1.0f);
            setModelInfo(modelInfo);
            Q_ASSERT(!m_modelInfo.filename().isEmpty());
            if (m_modelInfo.filename().isEmpty())
                emit modelLoadingError(u"Modelinfo is left null for %1"_s.arg(modelInfo.filename()));
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
            m_llModelType = LLModelTypeV1::API;
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

    if (m_llModelInfo.model)
        setModelInfo(modelInfo);
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
    case 'L': m_llModelType = LLModelTypeV1::LLAMA; break;
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
}

bool ChatLLM::isModelLoaded() const
{
    return m_llModelInfo.model && m_llModelInfo.model->isModelLoaded();
}

static QString &removeLeadingWhitespace(QString &s)
{
    auto firstNonSpace = ranges::find_if_not(s, [](auto c) { return c.isSpace(); });
    s.remove(0, firstNonSpace - s.begin());
    return s;
}

template <ranges::input_range R>
    requires std::convertible_to<ranges::range_reference_t<R>, QChar>
bool isAllSpace(R &&r)
{
    return ranges::all_of(std::forward<R>(r), [](QChar c) { return c.isSpace(); });
}

void ChatLLM::regenerateResponse(int index)
{
    Q_ASSERT(m_chatModel);
    if (m_chatModel->regenerateResponse(index)) {
        emit responseChanged();
        prompt(m_chat->collectionList());
    }
}

std::optional<QString> ChatLLM::popPrompt(int index)
{
    Q_ASSERT(m_chatModel);
    return m_chatModel->popPrompt(index);
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

void ChatLLM::acquireModel()
{
    m_llModelInfo = LLModelStore::globalInstance()->acquireModel();
    emit loadedModelInfoChanged();
}

void ChatLLM::resetModel()
{
    m_llModelInfo = {};
    emit loadedModelInfoChanged();
}

void ChatLLM::modelChangeRequested(const ModelInfo &modelInfo)
{
    // ignore attempts to switch to the same model twice
    if (!isModelLoaded() || this->modelInfo() != modelInfo) {
        m_shouldBeLoaded = true;
        loadModel(modelInfo);
    }
}

static LLModel::PromptContext promptContextFromSettings(const ModelInfo &modelInfo)
{
    auto *mySettings = MySettings::globalInstance();
    return {
        .n_predict      = mySettings->modelMaxLength          (modelInfo),
        .top_k          = mySettings->modelTopK               (modelInfo),
        .top_p          = float(mySettings->modelTopP         (modelInfo)),
        .min_p          = float(mySettings->modelMinP         (modelInfo)),
        .temp           = float(mySettings->modelTemperature  (modelInfo)),
        .n_batch        = mySettings->modelPromptBatchSize    (modelInfo),
        .repeat_penalty = float(mySettings->modelRepeatPenalty(modelInfo)),
        .repeat_last_n  = mySettings->modelRepeatPenaltyTokens(modelInfo),
    };
}

void ChatLLM::prompt(const QStringList &enabledCollections)
{
    if (!isModelLoaded()) {
        emit responseStopped(0);
        return;
    }

    try {
        promptInternalChat(enabledCollections, promptContextFromSettings(m_modelInfo));
    } catch (const std::exception &e) {
        // FIXME(jared): this is neither translated nor serialized
        m_chatModel->setResponseValue(u"Error: %1"_s.arg(QString::fromUtf8(e.what())));
        m_chatModel->setError();
        emit responseStopped(0);
    }
}

std::vector<MessageItem> ChatLLM::forkConversation(const QString &prompt) const
{
    Q_ASSERT(m_chatModel);
    if (m_chatModel->hasError())
        throw std::logic_error("cannot continue conversation with an error");

    std::vector<MessageItem> conversation;
    {
        auto items = m_chatModel->messageItems();
        // It is possible the main thread could have erased the conversation while the llm thread,
        // is busy forking the conversatoin but it must have set stop generating first
        Q_ASSERT(items.size() >= 2 || m_stopGenerating); // should be prompt/response pairs
        conversation.reserve(items.size() + 1);
        conversation.assign(items.begin(), items.end());
    }
    conversation.emplace_back(MessageItem::Type::Prompt, prompt.toUtf8());
    return conversation;
}

// version 0 (default): HF compatible
// version 1: explicit LocalDocs formatting
static uint parseJinjaTemplateVersion(QStringView tmpl)
{
    static uint MAX_VERSION = 1;
    static QRegularExpression reVersion(uR"(\A{#-?\s+gpt4all v(\d+)-?#}\s*$)"_s, QRegularExpression::MultilineOption);
    if (auto match = reVersion.matchView(tmpl); match.hasMatch()) {
        uint ver = match.captured(1).toUInt();
        if (ver > MAX_VERSION)
            throw std::out_of_range(fmt::format("Unknown template version: {}", ver));
        return ver;
    }
    return 0;
}

static auto loadJinjaTemplate(
    std::optional<jinja2::Template> &tmpl /*out*/, const std::string &source
) -> jinja2::Result<void>
{
    tmpl.emplace(jinjaEnv());
    return tmpl->Load(source);
}

std::optional<std::string> ChatLLM::checkJinjaTemplateError(const std::string &source)
{
    std::optional<jinja2::Template> tmpl;
    if (auto res = loadJinjaTemplate(tmpl, source); !res)
        return res.error().ToString();
    return std::nullopt;
}

std::string ChatLLM::applyJinjaTemplate(std::span<const MessageItem> items) const
{
    Q_ASSERT(items.size() >= 1);

    auto *mySettings = MySettings::globalInstance();
    auto &model      = m_llModelInfo.model;

    QString chatTemplate, systemMessage;
    auto chatTemplateSetting = mySettings->modelChatTemplate(m_modelInfo);
    if (auto tmpl = chatTemplateSetting.asModern()) {
        chatTemplate = *tmpl;
    } else if (chatTemplateSetting.isLegacy()) {
        throw std::logic_error("cannot apply Jinja to a legacy prompt template");
    } else {
        throw std::logic_error("cannot apply Jinja without setting a chat template first");
    }
    if (isAllSpace(chatTemplate)) {
        throw std::logic_error("cannot apply Jinja with a blank chat template");
    }
    if (auto tmpl = mySettings->modelSystemMessage(m_modelInfo).asModern()) {
        systemMessage = *tmpl;
    } else {
        throw std::logic_error("cannot apply Jinja with a legacy system message");
    }

    uint version = parseJinjaTemplateVersion(chatTemplate);

    auto makeMap = [version](const MessageItem &item) {
        return jinja2::GenericMap([msg = std::make_shared<JinjaMessage>(version, item)] { return msg.get(); });
    };

    std::unique_ptr<MessageItem> systemItem;
    bool useSystem = !isAllSpace(systemMessage);

    jinja2::ValuesList messages;
    messages.reserve(useSystem + items.size());
    if (useSystem) {
        systemItem = std::make_unique<MessageItem>(MessageItem::Type::System, systemMessage.toUtf8());
        messages.emplace_back(makeMap(*systemItem));
    }
    for (auto &item : items)
        messages.emplace_back(makeMap(item));

    jinja2::ValuesList toolList;
    const int toolCount = ToolModel::globalInstance()->count();
    for (int i = 0; i < toolCount; ++i) {
        Tool *t = ToolModel::globalInstance()->get(i);
        toolList.push_back(t->jinjaValue());
    }

    jinja2::ValuesMap params {
        { "messages",              std::move(messages) },
        { "add_generation_prompt", true                },
        { "toolList",              toolList            },
    };
    for (auto &[name, token] : model->specialTokens())
        params.emplace(std::move(name), std::move(token));

    std::optional<jinja2::Template> tmpl;
    auto maybeRendered = loadJinjaTemplate(tmpl, chatTemplate.toStdString())
        .and_then([&] { return tmpl->RenderAsString(params); });
    if (!maybeRendered)
        throw std::runtime_error(fmt::format("Failed to parse chat template: {}", maybeRendered.error().ToString()));
    return *maybeRendered;
}

auto ChatLLM::promptInternalChat(const QStringList &enabledCollections, const LLModel::PromptContext &ctx,
                                 qsizetype startOffset) -> ChatPromptResult
{
    Q_ASSERT(isModelLoaded());
    Q_ASSERT(m_chatModel);

    // Return a vector of relevant messages for this chat.
    // "startOffset" is used to select only local server messages from the current chat session.
    auto getChat = [&]() {
        auto items = m_chatModel->messageItems();
        if (startOffset > 0)
            items.erase(items.begin(), items.begin() + startOffset);
        Q_ASSERT(items.size() >= 2);
        return items;
    };

    QList<ResultInfo> databaseResults;
    if (!enabledCollections.isEmpty()) {
        std::optional<std::pair<int, QString>> query;
        {
            // Find the prompt that represents the query. Server chats are flexible and may not have one.
            auto items = getChat();
            if (auto peer = m_chatModel->getPeer(items, items.end() - 1)) // peer of response
                query = { *peer - items.begin(), (*peer)->content() };
        }

        if (query) {
            auto &[promptIndex, queryStr] = *query;
            const int retrievalSize = MySettings::globalInstance()->localDocsRetrievalSize();
            emit requestRetrieveFromDB(enabledCollections, queryStr, retrievalSize, &databaseResults); // blocks
            m_chatModel->updateSources(promptIndex, databaseResults);
            emit databaseResultsChanged(databaseResults);
        }
    }

    auto messageItems = getChat();
    messageItems.pop_back(); // exclude new response

    auto result = promptInternal(messageItems, ctx, !databaseResults.isEmpty());
    return {
        /*PromptResult*/ {
            .response       = std::move(result.response),
            .promptTokens   = result.promptTokens,
            .responseTokens = result.responseTokens,
        },
        /*databaseResults*/ std::move(databaseResults),
    };
}

auto ChatLLM::promptInternal(
    const std::variant<std::span<const MessageItem>, std::string_view> &prompt,
    const LLModel::PromptContext &ctx,
    bool usedLocalDocs
) -> PromptResult
{
    Q_ASSERT(isModelLoaded());

    auto *mySettings = MySettings::globalInstance();

    // unpack prompt argument
    const std::span<const MessageItem> *messageItems = nullptr;
    std::string                      jinjaBuffer;
    std::string_view                 conversation;
    if (auto *nonChat = std::get_if<std::string_view>(&prompt)) {
        conversation = *nonChat; // complete the string without a template
    } else {
        messageItems    = &std::get<std::span<const MessageItem>>(prompt);
        jinjaBuffer  = applyJinjaTemplate(*messageItems);
        conversation = jinjaBuffer;
    }

    // check for overlength last message
    if (!dynamic_cast<const ChatAPI *>(m_llModelInfo.model.get())) {
        auto nCtx = m_llModelInfo.model->contextLength();
        std::string jinjaBuffer2;
        auto lastMessageRendered = (messageItems && messageItems->size() > 1)
            ? std::string_view(jinjaBuffer2 = applyJinjaTemplate({ &messageItems->back(), 1 }))
            : conversation;
        int32_t lastMessageLength = m_llModelInfo.model->countPromptTokens(lastMessageRendered);
        if (auto limit = nCtx - 4; lastMessageLength > limit) {
            throw std::invalid_argument(
                tr("Your message was too long and could not be processed (%1 > %2). "
                   "Please try again with something shorter.").arg(lastMessageLength).arg(limit).toUtf8().constData()
            );
        }
    }

    PromptResult result {};

    auto handlePrompt = [this, &result](std::span<const LLModel::Token> batch, bool cached) -> bool {
        Q_UNUSED(cached)
        result.promptTokens += batch.size();
        m_timer->start();
        return !m_stopGenerating;
    };

    ToolCallParser toolCallParser;
    auto handleResponse = [this, &result, &toolCallParser](LLModel::Token token, std::string_view piece) -> bool {
        Q_UNUSED(token)
        result.responseTokens++;
        m_timer->inc();

        // FIXME: This is *not* necessarily fully formed utf data because it can be partial at this point
        // handle this like below where we have a QByteArray
        toolCallParser.update(QString::fromStdString(piece.data()));

        // Create a toolcall and split the response if needed
        if (!toolCallParser.hasSplit() && toolCallParser.state() == ToolEnums::ParseState::Partial) {
            const QPair<QString, QString> pair = toolCallParser.split();
            m_chatModel->splitToolCall(pair);
        }

        result.response.append(piece.data(), piece.size());
        auto respStr = QString::fromUtf8(result.response);

        try {
            if (toolCallParser.hasSplit())
                m_chatModel->setResponseValue(toolCallParser.buffer());
            else
                m_chatModel->setResponseValue(removeLeadingWhitespace(respStr));
        } catch (const std::exception &e) {
            // We have a try/catch here because the main thread might have removed the response from
            // the chatmodel by erasing the conversation during the response... the main thread sets
            // m_stopGenerating before doing so, but it doesn't wait after that to reset the chatmodel
            Q_ASSERT(m_stopGenerating);
            return false;
        }

        emit responseChanged();

        const bool foundToolCall = toolCallParser.state() == ToolEnums::ParseState::Complete;
        return !foundToolCall && !m_stopGenerating;
    };

    QElapsedTimer totalTime;
    totalTime.start();
    m_timer->start();

    try {
        emit promptProcessing();
        m_llModelInfo.model->setThreadCount(mySettings->threadCount());
        m_stopGenerating = false;
        m_llModelInfo.model->prompt(conversation, handlePrompt, handleResponse, ctx);
    } catch (...) {
        m_timer->stop();
        throw;
    }

    m_timer->stop();
    qint64 elapsed = totalTime.elapsed();

    const bool foundToolCall = toolCallParser.state() == ToolEnums::ParseState::Complete;

    // trim trailing whitespace
    auto respStr = QString::fromUtf8(result.response);
    if (!respStr.isEmpty() && (std::as_const(respStr).back().isSpace() || foundToolCall)) {
        if (toolCallParser.hasSplit())
            m_chatModel->setResponseValue(toolCallParser.buffer());
        else
            m_chatModel->setResponseValue(respStr.trimmed());
        emit responseChanged();
    }

    bool doQuestions = false;
    if (!m_isServer && messageItems && !foundToolCall) {
        switch (mySettings->suggestionMode()) {
            case SuggestionMode::On:            doQuestions = true;          break;
            case SuggestionMode::LocalDocsOnly: doQuestions = usedLocalDocs; break;
            case SuggestionMode::Off:           ;
        }
    }
    if (doQuestions)
        generateQuestions(elapsed);
    else
        emit responseStopped(elapsed);

    return result;
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

#if defined(DEBUG_MODEL_LOADING)
    qDebug() << "unloadModel" << m_llmThread.objectName() << m_llModelInfo.model.get();
#endif

    if (m_forceUnloadModel) {
        m_llModelInfo.resetModel(this);
        m_forceUnloadModel = false;
    }

    LLModelStore::globalInstance()->releaseModel(std::move(m_llModelInfo));
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
    if (!isModelLoaded() || m_isServer)
        return;

    Q_ASSERT(m_chatModel);

    auto *mySettings = MySettings::globalInstance();

    const QString chatNamePrompt = mySettings->modelChatNamePrompt(m_modelInfo);
    if (isAllSpace(chatNamePrompt)) {
        qWarning() << "ChatLLM: not generating chat name because prompt is empty";
        return;
    }

    QByteArray response; // raw UTF-8

    auto handleResponse = [this, &response](LLModel::Token token, std::string_view piece) -> bool {
        Q_UNUSED(token)

        response.append(piece.data(), piece.size());
        QStringList words = QString::fromUtf8(response).simplified().split(u' ', Qt::SkipEmptyParts);
        emit generatedNameChanged(words.join(u' '));
        return words.size() <= 3;
    };

    try {
        m_llModelInfo.model->prompt(
            applyJinjaTemplate(forkConversation(chatNamePrompt)),
            [this](auto &&...) { return !m_stopGenerating; },
            handleResponse,
            promptContextFromSettings(m_modelInfo)
        );
    } catch (const std::exception &e) {
        qWarning() << "ChatLLM failed to generate name:" << e.what();
    }
}

void ChatLLM::handleChatIdChanged(const QString &id)
{
    m_llmThread.setObjectName(id);
}

void ChatLLM::generateQuestions(qint64 elapsed)
{
    // FIXME: This only works with response by the model in english which is not ideal for a multi-language
    // model.
    // match whole question sentences
    static const std::regex reQuestion(R"(\b(?:What|Where|How|Why|When|Who|Which|Whose|Whom)\b[^?]*\?)");

    Q_ASSERT(isModelLoaded());
    if (!isModelLoaded()) {
        emit responseStopped(elapsed);
        return;
    }

    auto *mySettings = MySettings::globalInstance();

    QString suggestedFollowUpPrompt = mySettings->modelSuggestedFollowUpPrompt(m_modelInfo);
    if (isAllSpace(suggestedFollowUpPrompt)) {
        qWarning() << "ChatLLM: not generating follow-up questions because prompt is empty";
        emit responseStopped(elapsed);
        return;
    }

    emit generatingQuestions();

    std::string response; // raw UTF-8

    auto handleResponse = [this, &response](LLModel::Token token, std::string_view piece) -> bool {
        Q_UNUSED(token)

        // add token to buffer
        response.append(piece);

        // extract all questions from response
        ptrdiff_t lastMatchEnd = -1;
        auto it = std::sregex_iterator(response.begin(), response.end(), reQuestion);
        auto end = std::sregex_iterator();
        for (; it != end; ++it) {
            auto pos = it->position();
            auto len = it->length();
            lastMatchEnd = pos + len;
            emit generatedQuestionFinished(QString::fromUtf8(&response[pos], len));
        }

        // remove processed input from buffer
        if (lastMatchEnd != -1)
            response.erase(0, lastMatchEnd);
        return true;
    };

    QElapsedTimer totalTime;
    totalTime.start();
    try {
        m_llModelInfo.model->prompt(
            applyJinjaTemplate(forkConversation(suggestedFollowUpPrompt)),
            [this](auto &&...) { return !m_stopGenerating; },
            handleResponse,
            promptContextFromSettings(m_modelInfo)
        );
    } catch (const std::exception &e) {
        qWarning() << "ChatLLM failed to generate follow-up questions:" << e.what();
    }
    elapsed += totalTime.elapsed();
    emit responseStopped(elapsed);
}

// this function serialized the cached model state to disk.
// we want to also serialize n_ctx, and read it at load time.
bool ChatLLM::serialize(QDataStream &stream, int version)
{
    if (version < 11) {
        if (version >= 6) {
            stream << false; // serializeKV
        }
        if (version >= 2) {
            if (m_llModelType == LLModelTypeV1::NONE) {
                qWarning() << "ChatLLM ERROR: attempted to serialize a null model for chat id" << m_chat->id()
                           << "name" << m_chat->name();
                return false;
            }
            stream << m_llModelType;
            stream << 0; // state version
        }
        {
            QString dummy;
            stream << dummy; // response
            stream << dummy; // generated name
        }
        stream << quint32(0); // prompt + response tokens

        if (version < 6) { // serialize binary state
            if (version < 4) {
                stream << 0; // responseLogits
            }
            stream << int32_t(0); // n_past
            stream << quint64(0); // input token count
            stream << QByteArray(); // KV cache state
        }
    }
    return stream.status() == QDataStream::Ok;
}

bool ChatLLM::deserialize(QDataStream &stream, int version)
{
    // discard all state since we are initialized from the ChatModel as of v11
    if (version < 11) {
        union { int intval; quint32 u32; quint64 u64; };

        bool deserializeKV = true;
        if (version >= 6)
            stream >> deserializeKV;

        if (version >= 2) {
            stream >> intval; // model type
            auto llModelType = (version >= 6 ? parseLLModelTypeV1 : parseLLModelTypeV0)(intval);
            if (llModelType == LLModelTypeV1::NONE) {
                qWarning().nospace() << "error loading chat id " << m_chat->id() << ": unrecognized model type: "
                                     << intval;
                return false;
            }

            /* note: prior to chat version 10, API models and chats with models removed in v2.5.0 only wrote this because of
             * undefined behavior in Release builds */
            stream >> intval; // state version
            if (intval) {
                qWarning().nospace() << "error loading chat id " << m_chat->id() << ": unrecognized internal state version";
                return false;
            }
        }

        {
            QString dummy;
            stream >> dummy; // response
            stream >> dummy; // name response
        }
        stream >> u32; // prompt + response token count

        // We don't use the raw model state anymore.
        if (deserializeKV) {
            if (version < 4) {
                stream >> u32; // response logits
            }
            stream >> u32; // n_past
            if (version >= 7) {
                stream >> u32; // n_ctx
            }
            if (version < 9) {
                stream >> u64; // logits size
                stream.skipRawData(u64 * sizeof(float)); // logits
            }
            stream >> u64; // token cache size
            stream.skipRawData(u64 * sizeof(int)); // token cache
            QByteArray dummy;
            stream >> dummy; // state
        }
    }
    return stream.status() == QDataStream::Ok;
}
