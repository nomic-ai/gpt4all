#include "llm.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QResource>

class MyLLM: public LLM { };
Q_GLOBAL_STATIC(MyLLM, llmInstance)
LLM *LLM::globalInstance()
{
    return llmInstance();
}

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

    QString modelName("ggml-model-q4_0.bin");
    QFile file(QCoreApplication::applicationDirPath() + QDir::separator() + modelName);
    if (file.open(QIODevice::ReadOnly)) {

        QByteArray data = file.readAll();
        std::istringstream iss(data.toStdString());

        m_gptj->loadModel(modelName.toStdString(), iss);
        emit isModelLoadedChanged();
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
}

QString GPTJObject::response() const
{
    return QString::fromStdString(m_response);
}

bool GPTJObject::handleResponse(const std::string &response)
{
#if 0
    printf("%s", response.c_str());
    fflush(stdout);
#endif
    m_response.append(response);
    emit responseChanged();
    return !m_stopGenerating;
}

bool GPTJObject::prompt(const QString &prompt)
{
    if (!isModelLoaded())
        return false;

    m_stopGenerating = false;
    auto func = std::bind(&GPTJObject::handleResponse, this, std::placeholders::_1);
    emit responseStarted();
    static GPTJ::PromptContext ctx;
    m_gptj->prompt(prompt.toStdString(), func, ctx, 4096 /*number of chars to predict*/);
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

    connect(this, &LLM::promptRequested, m_gptj, &GPTJObject::prompt, Qt::QueuedConnection);
    connect(this, &LLM::resetResponseRequested, m_gptj, &GPTJObject::resetResponse, Qt::BlockingQueuedConnection);
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
