#include "llm.h"
#include "download.h"
#include "network.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QResource>
#include <fstream>

class MyLLM: public LLM { };
Q_GLOBAL_STATIC(MyLLM, llmInstance)
LLM *LLM::globalInstance()
{
    return llmInstance();
}

static LLModel::PromptContext s_ctx;

static QString modelFilePath(const QString &modelName)
{
    QString appPath = QCoreApplication::applicationDirPath()
        + QDir::separator() + "ggml-" + modelName + ".bin";
    QFileInfo infoAppPath(appPath);
    if (infoAppPath.exists())
        return appPath;

    QString downloadPath = Download::globalInstance()->downloadLocalModelsPath()
        + QDir::separator() + "ggml-" + modelName + ".bin";

    QFileInfo infoLocalPath(downloadPath);
    if (infoLocalPath.exists())
        return downloadPath;
    return QString();
}

LLMObject::LLMObject()
    : QObject{nullptr}
    , m_llmodel(nullptr)
    , m_responseTokens(0)
    , m_responseLogits(0)
    , m_isRecalc(false)
{
    moveToThread(&m_llmThread);
    connect(&m_llmThread, &QThread::started, this, &LLMObject::loadModel);
    connect(this, &LLMObject::sendStartup, Network::globalInstance(), &Network::sendStartup);
    connect(this, &LLMObject::sendModelLoaded, Network::globalInstance(), &Network::sendModelLoaded);
    connect(this, &LLMObject::sendResetContext, Network::globalInstance(), &Network::sendResetContext);
    m_llmThread.setObjectName("llm thread");
    m_llmThread.start();
}

bool LLMObject::loadModel()
{
    const QList<QString> models = modelList();
    if (models.isEmpty()) {
        // try again when we get a list of models
        connect(Download::globalInstance(), &Download::modelListChanged, this,
            &LLMObject::loadModel, Qt::SingleShotConnection);
        return false;
    }

    return loadModelPrivate(models.first());
}

bool LLMObject::loadModelPrivate(const QString &modelName)
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
        isGPTJ = magic == 0x67676d6c;
        if (isGPTJ) {
            m_llmodel = new GPTJ;
            m_llmodel->loadModel(modelName.toStdString(), fin);
        } else {
            m_llmodel = new LLamaModel;
            m_llmodel->loadModel(filePath.toStdString());
        }

        emit isModelLoadedChanged();
        emit threadCountChanged();

        if (isFirstLoad)
            emit sendStartup();
        else
            emit sendModelLoaded();
    }

    if (m_llmodel)
        setModelName(info.completeBaseName().remove(0, 5)); // remove the ggml- prefix

    return m_llmodel;
}

void LLMObject::setThreadCount(int32_t n_threads) {
    if (m_llmodel && m_llmodel->threadCount() != n_threads) {
        m_llmodel->setThreadCount(n_threads);
        emit threadCountChanged();
    }
}

int32_t LLMObject::threadCount() {
    if (!m_llmodel)
        return 1;
    return m_llmodel->threadCount();
}

bool LLMObject::isModelLoaded() const
{
    return m_llmodel && m_llmodel->isModelLoaded();
}

void LLMObject::regenerateResponse()
{
    s_ctx.n_past -= m_responseTokens;
    s_ctx.n_past = std::max(0, s_ctx.n_past);
    // FIXME: This does not seem to be needed in my testing and llama models don't to it. Remove?
    s_ctx.logits.erase(s_ctx.logits.end() -= m_responseLogits, s_ctx.logits.end());
    s_ctx.tokens.erase(s_ctx.tokens.end() -= m_responseTokens, s_ctx.tokens.end());
    m_responseTokens = 0;
    m_responseLogits = 0;
    m_response = std::string();
    emit responseChanged();
}

void LLMObject::resetResponse()
{
    m_responseTokens = 0;
    m_responseLogits = 0;
    m_response = std::string();
    emit responseChanged();
}

void LLMObject::resetContext()
{
    resetContextPrivate();
    emit sendResetContext();
}

void LLMObject::resetContextPrivate()
{
    regenerateResponse();
    s_ctx = LLModel::PromptContext();
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

QString LLMObject::response() const
{
    return QString::fromStdString(remove_leading_whitespace(m_response));
}

QString LLMObject::modelName() const
{
    return m_modelName;
}

void LLMObject::setModelName(const QString &modelName)
{
    m_modelName = modelName;
    emit modelNameChanged();
    emit modelListChanged();
}

void LLMObject::modelNameChangeRequested(const QString &modelName)
{
    if (!loadModelPrivate(modelName))
        qWarning() << "ERROR: Could not load model" << modelName;
}

QList<QString> LLMObject::modelList() const
{
    // Build a model list from exepath and from the localpath
    QList<QString> list;

    QString exePath = QCoreApplication::applicationDirPath() + QDir::separator();
    QString localPath = Download::globalInstance()->downloadLocalModelsPath();

    {
        QDir dir(exePath);
        dir.setNameFilters(QStringList() << "ggml-*.bin");
        QStringList fileNames = dir.entryList();
        for (QString f : fileNames) {
            QString filePath = exePath + f;
            QFileInfo info(filePath);
            QString name = info.completeBaseName().remove(0, 5);
            if (info.exists()) {
                if (name == m_modelName)
                    list.prepend(name);
                else
                    list.append(name);
            }
        }
    }

    if (localPath != exePath) {
        QDir dir(localPath);
        dir.setNameFilters(QStringList() << "ggml-*.bin");
        QStringList fileNames = dir.entryList();
        for (QString f : fileNames) {
            QString filePath = localPath + f;
            QFileInfo info(filePath);
            QString name = info.completeBaseName().remove(0, 5);
            if (info.exists() && !list.contains(name)) { // don't allow duplicates
                if (name == m_modelName)
                    list.prepend(name);
                else
                    list.append(name);
            }
        }
    }

    if (list.isEmpty()) {
        if (exePath != localPath) {
            qWarning() << "ERROR: Could not find any applicable models in"
                       << exePath << "nor" << localPath;
        } else {
            qWarning() << "ERROR: Could not find any applicable models in"
                       << exePath;
        }
        return QList<QString>();
    }

    return list;
}

bool LLMObject::handleResponse(int32_t token, const std::string &response)
{
#if 0
    printf("%s", response.c_str());
    fflush(stdout);
#endif

    // check for error
    if (token < 0) {
        m_response.append(response);
        emit responseChanged();
        return false;
    }

    // Save the token to our prompt ctxt
    if (s_ctx.tokens.size() == s_ctx.n_ctx)
        s_ctx.tokens.erase(s_ctx.tokens.begin());
    s_ctx.tokens.push_back(token);

    // m_responseTokens and m_responseLogits are related to last prompt/response not
    // the entire context window which we can reset on regenerate prompt
    ++m_responseTokens;
    if (!response.empty()) {
        m_response.append(response);
        emit responseChanged();
    }

    // Stop generation if we encounter prompt or response tokens
    QString r = QString::fromStdString(m_response);
    if (r.contains("### Prompt:") || r.contains("### Response:"))
        return false;
    return !m_stopGenerating;
}

bool LLMObject::handleRecalculate(bool isRecalc)
{
    if (m_isRecalc != isRecalc) {
        m_isRecalc = isRecalc;
        emit recalcChanged();
    }
    return !m_stopGenerating;
}

bool LLMObject::prompt(const QString &prompt, const QString &prompt_template, int32_t n_predict, int32_t top_k, float top_p,
                       float temp, int32_t n_batch, float repeat_penalty, int32_t repeat_penalty_tokens)
{
    if (!isModelLoaded())
        return false;

    QString instructPrompt = prompt_template.arg(prompt);

    m_stopGenerating = false;
    auto responseFunc = std::bind(&LLMObject::handleResponse, this, std::placeholders::_1,
        std::placeholders::_2);
    auto recalcFunc = std::bind(&LLMObject::handleRecalculate, this, std::placeholders::_1);
    emit responseStarted();
    qint32 logitsBefore = s_ctx.logits.size();
    s_ctx.n_predict = n_predict;
    s_ctx.top_k = top_k;
    s_ctx.top_p = top_p;
    s_ctx.temp = temp;
    s_ctx.n_batch = n_batch;
    s_ctx.repeat_penalty = repeat_penalty;
    s_ctx.repeat_last_n = repeat_penalty_tokens;
    m_llmodel->prompt(instructPrompt.toStdString(), responseFunc, recalcFunc, s_ctx);
    m_responseLogits += s_ctx.logits.size() - logitsBefore;
    std::string trimmed = trim_whitespace(m_response);
    if (trimmed != m_response) {
        m_response = trimmed;
        emit responseChanged();
    }
    emit responseStopped();

    return true;
}

LLM::LLM()
    : QObject{nullptr}
    , m_llmodel(new LLMObject)
    , m_responseInProgress(false)
{
    connect(Download::globalInstance(), &Download::modelListChanged, this, &LLM::modelListChanged, Qt::QueuedConnection);
    connect(m_llmodel, &LLMObject::isModelLoadedChanged, this, &LLM::isModelLoadedChanged, Qt::QueuedConnection);
    connect(m_llmodel, &LLMObject::responseChanged, this, &LLM::responseChanged, Qt::QueuedConnection);
    connect(m_llmodel, &LLMObject::responseStarted, this, &LLM::responseStarted, Qt::QueuedConnection);
    connect(m_llmodel, &LLMObject::responseStopped, this, &LLM::responseStopped, Qt::QueuedConnection);
    connect(m_llmodel, &LLMObject::modelNameChanged, this, &LLM::modelNameChanged, Qt::QueuedConnection);
    connect(m_llmodel, &LLMObject::modelListChanged, this, &LLM::modelListChanged, Qt::QueuedConnection);
    connect(m_llmodel, &LLMObject::threadCountChanged, this, &LLM::threadCountChanged, Qt::QueuedConnection);
    connect(m_llmodel, &LLMObject::threadCountChanged, this, &LLM::syncThreadCount, Qt::QueuedConnection);
    connect(m_llmodel, &LLMObject::recalcChanged, this, &LLM::recalcChanged, Qt::QueuedConnection);

    connect(this, &LLM::promptRequested, m_llmodel, &LLMObject::prompt, Qt::QueuedConnection);
    connect(this, &LLM::modelNameChangeRequested, m_llmodel, &LLMObject::modelNameChangeRequested, Qt::QueuedConnection);

    // The following are blocking operations and will block the gui thread, therefore must be fast
    // to respond to
    connect(this, &LLM::regenerateResponseRequested, m_llmodel, &LLMObject::regenerateResponse, Qt::BlockingQueuedConnection);
    connect(this, &LLM::resetResponseRequested, m_llmodel, &LLMObject::resetResponse, Qt::BlockingQueuedConnection);
    connect(this, &LLM::resetContextRequested, m_llmodel, &LLMObject::resetContext, Qt::BlockingQueuedConnection);
    connect(this, &LLM::setThreadCountRequested, m_llmodel, &LLMObject::setThreadCount, Qt::QueuedConnection);
}

bool LLM::isModelLoaded() const
{
    return m_llmodel->isModelLoaded();
}

void LLM::prompt(const QString &prompt, const QString &prompt_template, int32_t n_predict, int32_t top_k, float top_p,
                 float temp, int32_t n_batch, float repeat_penalty, int32_t repeat_penalty_tokens)
{
    emit promptRequested(prompt, prompt_template, n_predict, top_k, top_p, temp, n_batch, repeat_penalty, repeat_penalty_tokens);
}

void LLM::regenerateResponse()
{
    emit regenerateResponseRequested(); // blocking queued connection
}

void LLM::resetResponse()
{
    emit resetResponseRequested(); // blocking queued connection
}

void LLM::resetContext()
{
    emit resetContextRequested(); // blocking queued connection
}

void LLM::stopGenerating()
{
    m_llmodel->stopGenerating();
}

QString LLM::response() const
{
    return m_llmodel->response();
}

void LLM::responseStarted()
{
    m_responseInProgress = true;
    emit responseInProgressChanged();
}

void LLM::responseStopped()
{
    m_responseInProgress = false;
    emit responseInProgressChanged();
}

QString LLM::modelName() const
{
    return m_llmodel->modelName();
}

void LLM::setModelName(const QString &modelName)
{
    // doesn't block but will unload old model and load new one which the gui can see through changes
    // to the isModelLoaded property
    emit modelNameChangeRequested(modelName);
}

QList<QString> LLM::modelList() const
{
    return m_llmodel->modelList();
}

void LLM::syncThreadCount() {
    emit setThreadCountRequested(m_desiredThreadCount);
}

void LLM::setThreadCount(int32_t n_threads) {
    if (n_threads <= 0) {
        n_threads = std::min(4, (int32_t) std::thread::hardware_concurrency());
    }
    m_desiredThreadCount = n_threads;
    syncThreadCount();
}

int32_t LLM::threadCount() {
    return m_llmodel->threadCount();
}

bool LLM::checkForUpdates() const
{
#if defined(Q_OS_LINUX)
    QString tool("maintenancetool");
#elif defined(Q_OS_WINDOWS)
    QString tool("maintenancetool.exe");
#elif defined(Q_OS_DARWIN)
    QString tool("../../../maintenancetool.app/Contents/MacOS/maintenancetool");
#endif

    QString fileName = QCoreApplication::applicationDirPath()
        + QDir::separator() + ".." + QDir::separator() + tool;
    if (!QFileInfo::exists(fileName)) {
        qDebug() << "Couldn't find tool at" << fileName << "so cannot check for updates!";
        return false;
    }

    return QProcess::startDetached(fileName);
}

bool LLM::isRecalc() const
{
    return m_llmodel->isRecalc();
}
