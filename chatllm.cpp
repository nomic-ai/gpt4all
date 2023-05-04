#include "chatllm.h"
#include "chat.h"
#include "download.h"
#include "network.h"
#include "llmodel/gptj.h"
#include "llmodel/llamamodel.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QResource>
#include <QSettings>
#include <fstream>

//#define DEBUG

static QString modelFilePath(const QString &modelName)
{
    QString appPath = QCoreApplication::applicationDirPath()
        + "/ggml-" + modelName + ".bin";
    QFileInfo infoAppPath(appPath);
    if (infoAppPath.exists())
        return appPath;

    QString downloadPath = Download::globalInstance()->downloadLocalModelsPath()
        + "/ggml-" + modelName + ".bin";

    QFileInfo infoLocalPath(downloadPath);
    if (infoLocalPath.exists())
        return downloadPath;
    return QString();
}

ChatLLM::ChatLLM(Chat *parent)
    : QObject{nullptr}
    , m_llmodel(nullptr)
    , m_promptResponseTokens(0)
    , m_responseLogits(0)
    , m_isRecalc(false)
    , m_chat(parent)
{
    moveToThread(&m_llmThread);
    connect(this, &ChatLLM::sendStartup, Network::globalInstance(), &Network::sendStartup);
    connect(this, &ChatLLM::sendModelLoaded, Network::globalInstance(), &Network::sendModelLoaded);
    connect(m_chat, &Chat::idChanged, this, &ChatLLM::handleChatIdChanged);
    m_llmThread.setObjectName(m_chat->id());
    m_llmThread.start();
}

bool ChatLLM::loadDefaultModel()
{
    const QList<QString> models = m_chat->modelList();
    if (models.isEmpty()) {
        // try again when we get a list of models
        connect(Download::globalInstance(), &Download::modelListChanged, this,
            &ChatLLM::loadDefaultModel, Qt::SingleShotConnection);
        return false;
    }

    QSettings settings;
    settings.sync();
    QString defaultModel = settings.value("defaultModel", "gpt4all-j-v1.3-groovy").toString();
    if (defaultModel.isEmpty() || !models.contains(defaultModel))
        defaultModel = models.first();
    return loadModel(defaultModel);
}

bool ChatLLM::loadModel(const QString &modelName)
{
    if (isModelLoaded() && m_modelName == modelName)
        return true;

    bool isFirstLoad = false;
    if (isModelLoaded()) {
        resetContextPrivate();
        delete m_llmodel;
        m_llmodel = nullptr;
        emit isModelLoadedChanged();
    } else {
        isFirstLoad = true;
    }

    bool isGPTJ = false;
    QString filePath = modelFilePath(modelName);
    QFileInfo info(filePath);
    if (info.exists()) {

        auto fin = std::ifstream(filePath.toStdString(), std::ios::binary);
        uint32_t magic;
        fin.read((char *) &magic, sizeof(magic));
        fin.seekg(0);
        fin.close();
        isGPTJ = magic == 0x67676d6c;
        if (isGPTJ) {
            m_llmodel = new GPTJ;
            m_llmodel->loadModel(filePath.toStdString());
        } else {
            m_llmodel = new LLamaModel;
            m_llmodel->loadModel(filePath.toStdString());
        }

        emit isModelLoadedChanged();

        if (isFirstLoad)
            emit sendStartup();
        else
            emit sendModelLoaded();
    } else {
        qWarning() << "ERROR: Could not find model at" << filePath;
    }

    if (m_llmodel)
        setModelName(info.completeBaseName().remove(0, 5)); // remove the ggml- prefix

    return m_llmodel;
}

bool ChatLLM::isModelLoaded() const
{
    return m_llmodel && m_llmodel->isModelLoaded();
}

void ChatLLM::regenerateResponse()
{
    m_ctx.n_past -= m_promptResponseTokens;
    m_ctx.n_past = std::max(0, m_ctx.n_past);
    // FIXME: This does not seem to be needed in my testing and llama models don't to it. Remove?
    m_ctx.logits.erase(m_ctx.logits.end() -= m_responseLogits, m_ctx.logits.end());
    m_ctx.tokens.erase(m_ctx.tokens.end() -= m_promptResponseTokens, m_ctx.tokens.end());
    m_promptResponseTokens = 0;
    m_responseLogits = 0;
    m_response = std::string();
    emit responseChanged();
}

void ChatLLM::resetResponse()
{
    m_promptResponseTokens = 0;
    m_responseLogits = 0;
    m_response = std::string();
    emit responseChanged();
}

void ChatLLM::resetContext()
{
    resetContextPrivate();
    emit sendResetContext();
}

void ChatLLM::resetContextPrivate()
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
    if (!loadModel(modelName))
        qWarning() << "ERROR: Could not load model" << modelName;
}

bool ChatLLM::handlePrompt(int32_t token)
{
    // m_promptResponseTokens and m_responseLogits are related to last prompt/response not
    // the entire context window which we can reset on regenerate prompt
    ++m_promptResponseTokens;
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
        emit responseChanged();
        return false;
    }

    // m_promptResponseTokens and m_responseLogits are related to last prompt/response not
    // the entire context window which we can reset on regenerate prompt
    ++m_promptResponseTokens;
    Q_ASSERT(!response.empty());
    m_response.append(response);
    emit responseChanged();
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

bool ChatLLM::prompt(const QString &prompt, const QString &prompt_template, int32_t n_predict, int32_t top_k,
    float top_p, float temp, int32_t n_batch, float repeat_penalty, int32_t repeat_penalty_tokens, int n_threads)
{
    if (!isModelLoaded())
        return false;

    QString instructPrompt = prompt_template.arg(prompt);

    m_stopGenerating = false;
    auto promptFunc = std::bind(&ChatLLM::handlePrompt, this, std::placeholders::_1);
    auto responseFunc = std::bind(&ChatLLM::handleResponse, this, std::placeholders::_1,
        std::placeholders::_2);
    auto recalcFunc = std::bind(&ChatLLM::handleRecalculate, this, std::placeholders::_1);
    emit responseStarted();
    qint32 logitsBefore = m_ctx.logits.size();
    m_ctx.n_predict = n_predict;
    m_ctx.top_k = top_k;
    m_ctx.top_p = top_p;
    m_ctx.temp = temp;
    m_ctx.n_batch = n_batch;
    m_ctx.repeat_penalty = repeat_penalty;
    m_ctx.repeat_last_n = repeat_penalty_tokens;
    m_llmodel->setThreadCount(n_threads);
#if defined(DEBUG)
    printf("%s", qPrintable(instructPrompt));
    fflush(stdout);
#endif
    m_llmodel->prompt(instructPrompt.toStdString(), promptFunc, responseFunc, recalcFunc, m_ctx);
#if defined(DEBUG)
    printf("\n");
    fflush(stdout);
#endif
    m_responseLogits += m_ctx.logits.size() - logitsBefore;
    std::string trimmed = trim_whitespace(m_response);
    if (trimmed != m_response) {
        m_response = trimmed;
        emit responseChanged();
    }
    emit responseStopped();
    return true;
}

void ChatLLM::unloadModel()
{
    saveState();
    delete m_llmodel;
    m_llmodel = nullptr;
    emit isModelLoadedChanged();
}

void ChatLLM::reloadModel(const QString &modelName)
{
    if (modelName.isEmpty()) {
        loadDefaultModel();
    } else {
        loadModel(modelName);
    }
    restoreState();
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
    m_llmodel->prompt(instructPrompt.toStdString(), promptFunc, responseFunc, recalcFunc, ctx);
#if defined(DEBUG)
    printf("\n");
    fflush(stdout);
#endif
    std::string trimmed = trim_whitespace(m_nameResponse);
    if (trimmed != m_nameResponse) {
        m_nameResponse = trimmed;
        emit generatedNameChanged();
    }
}

void ChatLLM::handleChatIdChanged()
{
    m_llmThread.setObjectName(m_chat->id());
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
    emit generatedNameChanged();
    return true;
}

bool ChatLLM::handleNameRecalculate(bool isRecalc)
{
    Q_UNUSED(isRecalc);
    Q_UNREACHABLE();
    return true;
}

bool ChatLLM::serialize(QDataStream &stream)
{
    stream << response();
    stream << generatedName();
    stream << m_promptResponseTokens;
    stream << m_responseLogits;
    stream << m_ctx.n_past;
    stream << quint64(m_ctx.logits.size());
    stream.writeRawData(reinterpret_cast<const char*>(m_ctx.logits.data()), m_ctx.logits.size() * sizeof(float));
    stream << quint64(m_ctx.tokens.size());
    stream.writeRawData(reinterpret_cast<const char*>(m_ctx.tokens.data()), m_ctx.tokens.size() * sizeof(int));
    saveState();
    stream << m_state;
    return stream.status() == QDataStream::Ok;
}

bool ChatLLM::deserialize(QDataStream &stream)
{
    QString response;
    stream >> response;
    m_response = response.toStdString();
    QString nameResponse;
    stream >> nameResponse;
    m_nameResponse = nameResponse.toStdString();
    stream >> m_promptResponseTokens;
    stream >> m_responseLogits;
    stream >> m_ctx.n_past;
    quint64 logitsSize;
    stream >> logitsSize;
    m_ctx.logits.resize(logitsSize);
    stream.readRawData(reinterpret_cast<char*>(m_ctx.logits.data()), logitsSize * sizeof(float));
    quint64 tokensSize;
    stream >> tokensSize;
    m_ctx.tokens.resize(tokensSize);
    stream.readRawData(reinterpret_cast<char*>(m_ctx.tokens.data()), tokensSize * sizeof(int));
    stream >> m_state;
    return stream.status() == QDataStream::Ok;
}

void ChatLLM::saveState()
{
    if (!isModelLoaded())
        return;

    const size_t stateSize = m_llmodel->stateSize();
    m_state.resize(stateSize);
    m_llmodel->saveState(static_cast<uint8_t*>(reinterpret_cast<void*>(m_state.data())));
}

void ChatLLM::restoreState()
{
    if (!isModelLoaded())
        return;

    m_llmodel->restoreState(static_cast<const uint8_t*>(reinterpret_cast<void*>(m_state.data())));
}
