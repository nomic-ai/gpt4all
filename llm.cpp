#include "llm.h"

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

static GPTJ::PromptContext s_ctx;

GPTJObject::GPTJObject()
    : QObject{nullptr}
    , m_gptj(new GPTJ)
{
    moveToThread(&m_llmThread);
    connect(&m_llmThread, &QThread::started, this, &GPTJObject::loadModel);
    m_llmThread.setObjectName("llm thread");
    m_llmThread.start();
}

bool GPTJObject::loadModel()
{
    if (isModelLoaded())
        return true;

    QDir dir(QCoreApplication::applicationDirPath());
    dir.setNameFilters(QStringList() << "ggml-*.bin");
    QStringList fileNames = dir.entryList();
    if (fileNames.isEmpty()) {
        qDebug() << "ERROR: Could not find any applicable models in directory"
            << QCoreApplication::applicationDirPath();
    }

    QString modelName = fileNames.first();
    QString filePath = QCoreApplication::applicationDirPath() + QDir::separator() + modelName;
    QFileInfo info(filePath);
    if (info.exists()) {

        auto fin = std::ifstream(filePath.toStdString(), std::ios::binary);
        m_gptj->loadModel(modelName.toStdString(), fin);
        emit isModelLoadedChanged();
    }

    if (m_gptj) {
        m_modelName = info.baseName().remove(0, 5); // remove the ggml- prefix
        emit modelNameChanged();
    }

    return m_gptj;
}

bool GPTJObject::isModelLoaded() const
{
    return m_gptj->isModelLoaded();
}

void GPTJObject::resetResponse()
{
    m_response = std::string();
    emit responseChanged();
}

void GPTJObject::resetContext()
{
    s_ctx = GPTJ::PromptContext();
}

QString GPTJObject::response() const
{
    return QString::fromStdString(m_response);
}

QString GPTJObject::modelName() const
{
    return m_modelName;
}

bool GPTJObject::handleResponse(const std::string &response)
{
#if 0
    printf("%s", response.c_str());
    fflush(stdout);
#endif
    if (!response.empty()) {
        m_response.append(response);
        emit responseChanged();
    }
    return !m_stopGenerating;
}

bool GPTJObject::prompt(const QString &prompt)
{
    if (!isModelLoaded())
        return false;

    m_stopGenerating = false;
    auto func = std::bind(&GPTJObject::handleResponse, this, std::placeholders::_1);
    emit responseStarted();
    m_gptj->prompt(prompt.toStdString(), func, s_ctx, 4096 /*number of chars to predict*/);
    emit responseStopped();
    return true;
}

LLM::LLM()
    : QObject{nullptr}
    , m_gptj(new GPTJObject)
    , m_responseInProgress(false)
{
    connect(m_gptj, &GPTJObject::isModelLoadedChanged, this, &LLM::isModelLoadedChanged, Qt::QueuedConnection);
    connect(m_gptj, &GPTJObject::responseChanged, this, &LLM::responseChanged, Qt::QueuedConnection);
    connect(m_gptj, &GPTJObject::responseStarted, this, &LLM::responseStarted, Qt::QueuedConnection);
    connect(m_gptj, &GPTJObject::responseStopped, this, &LLM::responseStopped, Qt::QueuedConnection);
    connect(m_gptj, &GPTJObject::modelNameChanged, this, &LLM::modelNameChanged, Qt::QueuedConnection);

    connect(this, &LLM::promptRequested, m_gptj, &GPTJObject::prompt, Qt::QueuedConnection);
    connect(this, &LLM::resetResponseRequested, m_gptj, &GPTJObject::resetResponse, Qt::BlockingQueuedConnection);
    connect(this, &LLM::resetContextRequested, m_gptj, &GPTJObject::resetContext, Qt::BlockingQueuedConnection);
}

bool LLM::isModelLoaded() const
{
    return m_gptj->isModelLoaded();
}

void LLM::prompt(const QString &prompt)
{
    emit promptRequested(prompt);
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
    m_gptj->stopGenerating();
}

QString LLM::response() const
{
    return m_gptj->response();
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
    return m_gptj->modelName();
}

bool LLM::checkForUpdates() const
{
#if defined(Q_OS_LINUX)
    QString tool("maintenancetool");
#elif defined(Q_OS_WINDOWS)
    QString tool("maintenancetool.exe");
#elif defined(Q_OS_DARWIN)
    QString tool("maintenancetool.app/Contents/MacOS/maintenancetool");
#endif

    QString fileName = QCoreApplication::applicationDirPath()
        + QDir::separator() + ".." + QDir::separator() + tool;
    if (!QFileInfo::exists(fileName)) {
        qDebug() << "Couldn't find tool at" << fileName << "so cannot check for updates!";
        return false;
    }

    return QProcess::startDetached(fileName);
}

