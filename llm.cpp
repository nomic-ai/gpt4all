#include "llm.h"
#include "download.h"

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

LLMObject::LLMObject()
    : QObject{nullptr}
    , m_llmodel(nullptr)
    , m_responseTokens(0)
    , m_responseLogits(0)
{
    moveToThread(&m_llmThread);
    connect(&m_llmThread, &QThread::started, this, &LLMObject::loadModel);
    m_llmThread.setObjectName("llm thread");
    m_llmThread.start();
}

bool LLMObject::loadModel()
{
    if (modelList().isEmpty()) {
        // try again when we get a list of models
        connect(Download::globalInstance(), &Download::modelListChanged, this,
            &LLMObject::loadModel, Qt::SingleShotConnection);
        return false;
    }

    return loadModelPrivate(modelList().first());
}

bool LLMObject::loadModelPrivate(const QString &modelName)
{
    if (isModelLoaded() && m_modelName == modelName)
        return true;

    if (isModelLoaded()) {
        delete m_llmodel;
        m_llmodel = nullptr;
        emit isModelLoadedChanged();
    }

    m_llmodel = new GPTJ;

    QString filePath = QCoreApplication::applicationDirPath() + QDir::separator() +
         "ggml-" + modelName + ".bin";
    QFileInfo info(filePath);
    if (info.exists()) {

        auto fin = std::ifstream(filePath.toStdString(), std::ios::binary);
        m_llmodel->loadModel(modelName.toStdString(), fin);
        emit isModelLoadedChanged();
        emit threadCountChanged();
    }

    if (m_llmodel)
        setModelName(info.completeBaseName().remove(0, 5)); // remove the ggml- prefix

    return m_llmodel;
}

void LLMObject::setThreadCount(int32_t n_threads) {
    m_llmodel->setThreadCount(n_threads);
    emit threadCountChanged();
}

int32_t LLMObject::threadCount() {
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
    s_ctx.logits.erase(s_ctx.logits.end() -= m_responseLogits, s_ctx.logits.end());
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
    QDir dir(QCoreApplication::applicationDirPath());
    dir.setNameFilters(QStringList() << "ggml-*.bin");
    QStringList fileNames = dir.entryList();
    if (fileNames.isEmpty()) {
        qWarning() << "ERROR: Could not find any applicable models in directory"
            << QCoreApplication::applicationDirPath();
        return QList<QString>();
    }

    QList<QString> list;
    for (QString f : fileNames) {
        QString filePath = QCoreApplication::applicationDirPath() + QDir::separator() + f;
        QFileInfo info(filePath);
        QString name = info.completeBaseName().remove(0, 5);
        if (info.exists()) {
            if (name == m_modelName)
                list.prepend(name);
            else
                list.append(name);
        }
    }

    return list;
}

bool LLMObject::handleResponse(const std::string &response)
{
#if 0
    printf("%s", response.c_str());
    fflush(stdout);
#endif
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

bool LLMObject::prompt(const QString &prompt, const QString &prompt_template, int32_t n_predict, int32_t top_k, float top_p,
                       float temp, int32_t n_batch)
{
    if (!isModelLoaded())
        return false;

    QString instructPrompt = prompt_template.arg(prompt);

    m_stopGenerating = false;
    auto func = std::bind(&LLMObject::handleResponse, this, std::placeholders::_1);
    emit responseStarted();
    qint32 logitsBefore = s_ctx.logits.size();
    m_llmodel->prompt(instructPrompt.toStdString(), func, s_ctx, n_predict, top_k, top_p, temp, n_batch);
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
                 float temp, int32_t n_batch)
{
    emit promptRequested(prompt, prompt_template, n_predict, top_k, top_p, temp, n_batch);
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

void LLM::setThreadCount(int32_t n_threads) {
    emit setThreadCountRequested(n_threads);
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

