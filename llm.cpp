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

static LLModel::PromptContext s_ctx;

LLMObject::LLMObject()
    : QObject{nullptr}
    , m_llmodel(new GPTJ)
{
    moveToThread(&m_llmThread);
    connect(&m_llmThread, &QThread::started, this, &LLMObject::loadModel);
    m_llmThread.setObjectName("llm thread");
    m_llmThread.start();
}

bool LLMObject::loadModel()
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
        m_llmodel->loadModel(modelName.toStdString(), fin);
        emit isModelLoadedChanged();
    }

    if (m_llmodel) {
        m_modelName = info.baseName().remove(0, 5); // remove the ggml- prefix
        emit modelNameChanged();
    }

    return m_llmodel;
}

bool LLMObject::isModelLoaded() const
{
    return m_llmodel->isModelLoaded();
}

void LLMObject::resetResponse()
{
    m_response = std::string();
    emit responseChanged();
}

void LLMObject::resetContext()
{
    s_ctx = LLModel::PromptContext();
}

QString LLMObject::response() const
{
    return QString::fromStdString(m_response);
}

QString LLMObject::modelName() const
{
    return m_modelName;
}

bool LLMObject::handleResponse(const std::string &response)
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

bool LLMObject::prompt(const QString &prompt)
{
    if (!isModelLoaded())
        return false;

    m_stopGenerating = false;
    auto func = std::bind(&LLMObject::handleResponse, this, std::placeholders::_1);
    emit responseStarted();
    m_llmodel->prompt(prompt.toStdString(), func, s_ctx, 4096 /*number of chars to predict*/);
    emit responseStopped();
    return true;
}

LLM::LLM()
    : QObject{nullptr}
    , m_llmodel(new LLMObject)
    , m_responseInProgress(false)
{
    connect(m_llmodel, &LLMObject::isModelLoadedChanged, this, &LLM::isModelLoadedChanged, Qt::QueuedConnection);
    connect(m_llmodel, &LLMObject::responseChanged, this, &LLM::responseChanged, Qt::QueuedConnection);
    connect(m_llmodel, &LLMObject::responseStarted, this, &LLM::responseStarted, Qt::QueuedConnection);
    connect(m_llmodel, &LLMObject::responseStopped, this, &LLM::responseStopped, Qt::QueuedConnection);
    connect(m_llmodel, &LLMObject::modelNameChanged, this, &LLM::modelNameChanged, Qt::QueuedConnection);

    connect(this, &LLM::promptRequested, m_llmodel, &LLMObject::prompt, Qt::QueuedConnection);
    connect(this, &LLM::resetResponseRequested, m_llmodel, &LLMObject::resetResponse, Qt::BlockingQueuedConnection);
    connect(this, &LLM::resetContextRequested, m_llmodel, &LLMObject::resetContext, Qt::BlockingQueuedConnection);
}

bool LLM::isModelLoaded() const
{
    return m_llmodel->isModelLoaded();
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

