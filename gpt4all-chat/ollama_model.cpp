#include "ollama_model.h"

#include "chat.h"
#include "chatapi.h"
#include "localdocs.h"
#include "mysettings.h"
#include "network.h"

#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QGlobalStatic>
#include <QIODevice>
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


#define OLLAMA_INTERNAL_STATE_VERSION 0

OllamaModel::OllamaModel()
    : m_shouldBeLoaded(false)
    , m_forceUnloadModel(false)
    , m_markedForDeletion(false)
    , m_stopGenerating(false)
    , m_timer(new TokenTimer(this))
    , m_processedSystemPrompt(false)
{
    connect(this, &OllamaModel::shouldBeLoadedChanged, this, &OllamaModel::handleShouldBeLoadedChanged);
    connect(this, &OllamaModel::trySwitchContextRequested, this, &OllamaModel::trySwitchContextOfLoadedModel);
    connect(m_timer, &TokenTimer::report, this, &OllamaModel::reportSpeed);

    // The following are blocking operations and will block the llm thread
    connect(this, &OllamaModel::requestRetrieveFromDB, LocalDocs::globalInstance()->database(), &Database::retrieveFromDB,
        Qt::BlockingQueuedConnection);
}

OllamaModel::~OllamaModel()
{
    destroy();
}

void OllamaModel::destroy()
{
    // TODO(jared): cancel pending network requests
}

void OllamaModel::destroyStore()
{
    LLModelStore::globalInstance()->destroy();
}

bool OllamaModel::loadDefaultModel()
{
    ModelInfo defaultModel = ModelList::globalInstance()->defaultModelInfo();
    if (defaultModel.filename().isEmpty()) {
        emit modelLoadingError(u"Could not find any model to load"_s);
        return false;
    }
    return loadModel(defaultModel);
}

void OllamaModel::trySwitchContextOfLoadedModel(const ModelInfo &modelInfo)
{
    // no-op: we require the model to be explicitly loaded for now.
}

bool OllamaModel::loadModel(const ModelInfo &modelInfo)
{
    // We're already loaded with this model
    if (isModelLoaded() && this->modelInfo() == modelInfo)
        return true;

    // reset status
    emit modelLoadingPercentageChanged(std::numeric_limits<float>::min()); // small non-zero positive value
    emit modelLoadingError("");

    QString filePath = modelInfo.dirpath + modelInfo.filename();
    QFileInfo fileInfo(filePath);

    // We have a live model, but it isn't the one we want
    bool alreadyAcquired = isModelLoaded();
    if (alreadyAcquired) {
        resetContext();
        m_llModelInfo.resetModel(this);
    } else {
        // This is a blocking call that tries to retrieve the model we need from the model store.
        // If it succeeds, then we just have to restore state. If the store has never had a model
        // returned to it, then the modelInfo.model pointer should be null which will happen on startup
        acquireModel();
        // At this point it is possible that while we were blocked waiting to acquire the model from the
        // store, that our state was changed to not be loaded. If this is the case, release the model
        // back into the store and quit loading
        if (!m_shouldBeLoaded) {
            LLModelStore::globalInstance()->releaseModel(std::move(m_llModelInfo));
            emit modelLoadingPercentageChanged(0.0f);
            return false;
        }

        // Check if the store just gave us exactly the model we were looking for
        if (m_llModelInfo.model && m_llModelInfo.fileInfo == fileInfo) {
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
            m_llModelInfo.resetModel(this);
        }
    }

    // Guarantee we've released the previous models memory
    Q_ASSERT(!m_llModelInfo.model);

    // Store the file info in the modelInfo in case we have an error loading
    m_llModelInfo.fileInfo = fileInfo;

    if (fileInfo.exists()) {
        QVariantMap modelLoadProps;

        // TODO(jared): load the model here
#if 0
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
                    if (!Network::isHttpUrlValid(apiUrl))
                        return false;

                    QString currentPath(apiUrl.path());
                    QString suffixPath("%1/chat/completions");
                    apiUrl.setPath(suffixPath.arg(currentPath));
                    requestUrl = apiUrl.toString();
                } else {
                    requestUrl = modelInfo.url();
                }
            }
            ChatAPI *model = new ChatAPI();
            model->setModelName(modelName);
            model->setRequestURL(requestUrl);
            model->setAPIKey(apiKey);
            m_llModelInfo.resetModel(this, model);
        } else if (!loadNewModel(modelInfo, modelLoadProps)) {
            return false; // m_shouldBeLoaded became false
        }
#endif

        restoreState();
        emit modelLoadingPercentageChanged(isModelLoaded() ? 1.0f : 0.0f);
        emit loadedModelInfoChanged();

        modelLoadProps.insert("model", modelInfo.filename());
        Network::globalInstance()->trackChatEvent("model_load", modelLoadProps);
    } else {
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

bool OllamaModel::isModelLoaded() const
{
    return m_llModelInfo.model && m_llModelInfo.model->isModelLoaded();
}

// FIXME(jared): we don't actually have to re-decode the prompt to generate a new response
void OllamaModel::regenerateResponse()
{
    m_ctx.n_past = std::max(0, m_ctx.n_past - m_promptResponseTokens);
    m_ctx.tokens.erase(m_ctx.tokens.end() - m_promptResponseTokens, m_ctx.tokens.end());
    m_promptResponseTokens = 0;
    m_promptTokens = 0;
    m_response = std::string();
    emit responseChanged(QString::fromStdString(m_response));
}

void OllamaModel::resetResponse()
{
    m_promptTokens = 0;
    m_promptResponseTokens = 0;
    m_response = std::string();
    emit responseChanged(QString::fromStdString(m_response));
}

void OllamaModel::resetContext()
{
    resetResponse();
    m_processedSystemPrompt = false;
    m_ctx = ModelBackend::PromptContext();
}

QString OllamaModel::response() const
{
    return QString::fromStdString(remove_leading_whitespace(m_response));
}

void OllamaModel::setModelInfo(const ModelInfo &modelInfo)
{
    m_modelInfo = modelInfo;
    emit modelInfoChanged(modelInfo);
}

void OllamaModel::acquireModel()
{
    m_llModelInfo = LLModelStore::globalInstance()->acquireModel();
    emit loadedModelInfoChanged();
}

void OllamaModel::resetModel()
{
    m_llModelInfo = {};
    emit loadedModelInfoChanged();
}

void OllamaModel::modelChangeRequested(const ModelInfo &modelInfo)
{
    m_shouldBeLoaded = true;
    loadModel(modelInfo);
}

bool OllamaModel::handlePrompt(int32_t token)
{
    // m_promptResponseTokens is related to last prompt/response not
    // the entire context window which we can reset on regenerate prompt
    ++m_promptTokens;
    ++m_promptResponseTokens;
    m_timer->start();
    return !m_stopGenerating;
}

bool OllamaModel::handleResponse(int32_t token, const std::string &response)
{
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

bool OllamaModel::prompt(const QList<QString> &collectionList, const QString &prompt)
{
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

bool OllamaModel::promptInternal(const QList<QString> &collectionList, const QString &prompt, const QString &promptTemplate,
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
    QString docsContext;
    if (!databaseResults.isEmpty()) {
        QStringList results;
        for (const ResultInfo &info : databaseResults)
            results << u"Collection: %1\nPath: %2\nExcerpt: %3"_s.arg(info.collection, info.path, info.text);

        // FIXME(jared): use a Jinja prompt template instead of hardcoded Alpaca-style localdocs template
        docsContext = u"### Context:\n%1\n\n"_s.arg(results.join("\n\n"));
    }

    int n_threads = MySettings::globalInstance()->threadCount();

    m_stopGenerating = false;
    auto promptFunc = std::bind(&OllamaModel::handlePrompt, this, std::placeholders::_1);
    auto responseFunc = std::bind(&OllamaModel::handleResponse, this, std::placeholders::_1,
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

    QElapsedTimer totalTime;
    totalTime.start();
    m_timer->start();
    if (!docsContext.isEmpty()) {
        auto old_n_predict = std::exchange(m_ctx.n_predict, 0); // decode localdocs context without a response
        m_llModelInfo.model->prompt(docsContext.toStdString(), "%1", promptFunc, responseFunc,
                                    /*allowContextShift*/ true, m_ctx);
        m_ctx.n_predict = old_n_predict; // now we are ready for a response
    }
    m_llModelInfo.model->prompt(prompt.toStdString(), promptTemplate.toStdString(), promptFunc, responseFunc,
                                /*allowContextShift*/ true, m_ctx);

    m_timer->stop();
    qint64 elapsed = totalTime.elapsed();
    std::string trimmed = trim_whitespace(m_response);
    if (trimmed != m_response) {
        m_response = trimmed;
        emit responseChanged(QString::fromStdString(m_response));
    }

    SuggestionMode mode = MySettings::globalInstance()->suggestionMode();
    if (mode == SuggestionMode::On || (!databaseResults.isEmpty() && mode == SuggestionMode::LocalDocsOnly))
        generateQuestions(elapsed);
    else
        emit responseStopped(elapsed);

    return true;
}

void OllamaModel::setShouldBeLoaded(bool value, bool forceUnload)
{
    m_shouldBeLoaded = b; // atomic
    emit shouldBeLoadedChanged(forceUnload);
}

void OllamaModel::requestTrySwitchContext()
{
    m_shouldBeLoaded = true; // atomic
    emit trySwitchContextRequested(modelInfo());
}

void OllamaModel::handleShouldBeLoadedChanged()
{
    if (m_shouldBeLoaded)
        reloadModel();
    else
        unloadModel();
}

void OllamaModel::unloadModel()
{
    if (!isModelLoaded())
        return;

    if (!m_forceUnloadModel || !m_shouldBeLoaded)
        emit modelLoadingPercentageChanged(0.0f);
    else
        emit modelLoadingPercentageChanged(std::numeric_limits<float>::min()); // small non-zero positive value

    if (!m_markedForDeletion)
        saveState();

    if (m_forceUnloadModel) {
        m_llModelInfo.resetModel(this);
        m_forceUnloadModel = false;
    }

    LLModelStore::globalInstance()->releaseModel(std::move(m_llModelInfo));
}

void OllamaModel::reloadModel()
{
    if (isModelLoaded() && m_forceUnloadModel)
        unloadModel(); // we unload first if we are forcing an unload

    if (isModelLoaded())
        return;

    const ModelInfo m = modelInfo();
    if (m.name().isEmpty())
        loadDefaultModel();
    else
        loadModel(m);
}

void OllamaModel::generateName()
{
    Q_ASSERT(isModelLoaded());
    if (!isModelLoaded())
        return;

    const QString chatNamePrompt = MySettings::globalInstance()->modelChatNamePrompt(m_modelInfo);
    if (chatNamePrompt.trimmed().isEmpty()) {
        qWarning() << "OllamaModel: not generating chat name because prompt is empty";
        return;
    }

    auto promptTemplate = MySettings::globalInstance()->modelPromptTemplate(m_modelInfo);
    auto promptFunc = std::bind(&OllamaModel::handleNamePrompt, this, std::placeholders::_1);
    auto responseFunc = std::bind(&OllamaModel::handleNameResponse, this, std::placeholders::_1, std::placeholders::_2);
    ModelBackend::PromptContext ctx = m_ctx;
    m_llModelInfo.model->prompt(chatNamePrompt.toStdString(), promptTemplate.toStdString(),
                                promptFunc, responseFunc, /*allowContextShift*/ false, ctx);
    std::string trimmed = trim_whitespace(m_nameResponse);
    if (trimmed != m_nameResponse) {
        m_nameResponse = trimmed;
        emit generatedNameChanged(QString::fromStdString(m_nameResponse));
    }
}

bool OllamaModel::handleNamePrompt(int32_t token)
{
    Q_UNUSED(token);
    return !m_stopGenerating;
}

bool OllamaModel::handleNameResponse(int32_t token, const std::string &response)
{
    Q_UNUSED(token);

    m_nameResponse.append(response);
    emit generatedNameChanged(QString::fromStdString(m_nameResponse));
    QString gen = QString::fromStdString(m_nameResponse).simplified();
    QStringList words = gen.split(' ', Qt::SkipEmptyParts);
    return words.size() <= 3;
}

bool OllamaModel::handleQuestionPrompt(int32_t token)
{
    Q_UNUSED(token);
    return !m_stopGenerating;
}

bool OllamaModel::handleQuestionResponse(int32_t token, const std::string &response)
{
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

void OllamaModel::generateQuestions(qint64 elapsed)
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
    auto promptFunc = std::bind(&OllamaModel::handleQuestionPrompt, this, std::placeholders::_1);
    auto responseFunc = std::bind(&OllamaModel::handleQuestionResponse, this, std::placeholders::_1, std::placeholders::_2);
    ModelBackend::PromptContext ctx = m_ctx;
    QElapsedTimer totalTime;
    totalTime.start();
    m_llModelInfo.model->prompt(suggestedFollowUpPrompt, promptTemplate.toStdString(), promptFunc, responseFunc,
                                /*allowContextShift*/ false, ctx);
    elapsed += totalTime.elapsed();
    emit responseStopped(elapsed);
}


bool OllamaModel::handleSystemPrompt(int32_t token)
{
    Q_UNUSED(token);
    return !m_stopGenerating;
}

// this function serialized the cached model state to disk.
// we want to also serialize n_ctx, and read it at load time.
bool OllamaModel::serialize(QDataStream &stream, int version, bool serializeKV)
{
    Q_UNUSED(serializeKV);

    if (version < 10)
        throw std::out_of_range("ollama not avaliable until chat version 10, attempted to serialize version " + std::to_string(version));

    stream << OLLAMA_INTERNAL_STATE_VERSION;

    stream << response();
    stream << generatedName();
    // TODO(jared): do not save/restore m_promptResponseTokens, compute the appropriate value instead
    stream << m_promptResponseTokens;

    stream << m_ctx.n_ctx;
    saveState();
    QByteArray compressed = qCompress(m_state);
    stream << compressed;

    return stream.status() == QDataStream::Ok;
}

bool OllamaModel::deserialize(QDataStream &stream, int version, bool deserializeKV, bool discardKV)
{
    Q_UNUSED(deserializeKV);
    Q_UNUSED(discardKV);

    Q_ASSERT(version >= 10);

    int internalStateVersion;
    stream >> internalStateVersion; // for future use

    QString response;
    stream >> response;
    m_response = response.toStdString();
    QString nameResponse;
    stream >> nameResponse;
    m_nameResponse = nameResponse.toStdString();
    stream >> m_promptResponseTokens;

    uint32_t n_ctx;
    stream >> n_ctx;
    m_ctx.n_ctx = n_ctx;

    QByteArray compressed;
    stream >> compressed;
    m_state = qUncompress(compressed);

    return stream.status() == QDataStream::Ok;
}

void OllamaModel::saveState()
{
    if (!isModelLoaded())
        return;

    // m_llModelType == LLModelType::API_
    m_state.clear();
    QDataStream stream(&m_state, QIODeviceBase::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_4);
    ChatAPI *chatAPI = static_cast<ChatAPI *>(m_llModelInfo.model.get());
    stream << chatAPI->context();
    // end API
}

void OllamaModel::restoreState()
{
    if (!isModelLoaded())
        return;

    // m_llModelType == LLModelType::API_
    QDataStream stream(&m_state, QIODeviceBase::ReadOnly);
    stream.setVersion(QDataStream::Qt_6_4);
    ChatAPI *chatAPI = static_cast<ChatAPI *>(m_llModelInfo.model.get());
    QList<QString> context;
    stream >> context;
    chatAPI->setContext(context);
    m_state.clear();
    m_state.squeeze();
    // end API
}

void OllamaModel::processSystemPrompt()
{
    Q_ASSERT(isModelLoaded());
    if (!isModelLoaded() || m_processedSystemPrompt || m_restoreStateFromText)
        return;

    const std::string systemPrompt = MySettings::globalInstance()->modelSystemPrompt(m_modelInfo).toStdString();
    if (QString::fromStdString(systemPrompt).trimmed().isEmpty()) {
        m_processedSystemPrompt = true;
        return;
    }

    // Start with a whole new context
    m_stopGenerating = false;
    m_ctx = ModelBackend::PromptContext();

    auto promptFunc = std::bind(&OllamaModel::handleSystemPrompt, this, std::placeholders::_1);

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

    auto old_n_predict = std::exchange(m_ctx.n_predict, 0); // decode system prompt without a response
    // use "%1%2" and not "%1" to avoid implicit whitespace
    m_llModelInfo.model->prompt(systemPrompt, "%1%2", promptFunc, nullptr, /*allowContextShift*/ true, m_ctx, true);
    m_ctx.n_predict = old_n_predict;

    m_processedSystemPrompt = m_stopGenerating == false;
}
