#include "chat.h"
#include "network.h"

Chat::Chat(QObject *parent)
    : QObject(parent)
    , m_llmodel(new ChatLLM)
    , m_id(Network::globalInstance()->generateUniqueId())
    , m_name(tr("New Chat"))
    , m_chatModel(new ChatModel(this))
    , m_responseInProgress(false)
    , m_desiredThreadCount(std::min(4, (int32_t) std::thread::hardware_concurrency()))
{
    connect(m_llmodel, &ChatLLM::isModelLoadedChanged, this, &Chat::isModelLoadedChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::responseChanged, this, &Chat::responseChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::responseStarted, this, &Chat::responseStarted, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::responseStopped, this, &Chat::responseStopped, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::modelNameChanged, this, &Chat::modelNameChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::threadCountChanged, this, &Chat::threadCountChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::threadCountChanged, this, &Chat::syncThreadCount, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::recalcChanged, this, &Chat::recalcChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::generatedNameChanged, this, &Chat::generatedNameChanged, Qt::QueuedConnection);

    connect(this, &Chat::promptRequested, m_llmodel, &ChatLLM::prompt, Qt::QueuedConnection);
    connect(this, &Chat::modelNameChangeRequested, m_llmodel, &ChatLLM::modelNameChangeRequested, Qt::QueuedConnection);
    connect(this, &Chat::unloadRequested, m_llmodel, &ChatLLM::unload, Qt::QueuedConnection);
    connect(this, &Chat::reloadRequested, m_llmodel, &ChatLLM::reload, Qt::QueuedConnection);
    connect(this, &Chat::generateNameRequested, m_llmodel, &ChatLLM::generateName, Qt::QueuedConnection);

    // The following are blocking operations and will block the gui thread, therefore must be fast
    // to respond to
    connect(this, &Chat::regenerateResponseRequested, m_llmodel, &ChatLLM::regenerateResponse, Qt::BlockingQueuedConnection);
    connect(this, &Chat::resetResponseRequested, m_llmodel, &ChatLLM::resetResponse, Qt::BlockingQueuedConnection);
    connect(this, &Chat::resetContextRequested, m_llmodel, &ChatLLM::resetContext, Qt::BlockingQueuedConnection);
    connect(this, &Chat::setThreadCountRequested, m_llmodel, &ChatLLM::setThreadCount, Qt::QueuedConnection);
}

void Chat::reset()
{
    stopGenerating();
    emit resetContextRequested(); // blocking queued connection
    m_id = Network::globalInstance()->generateUniqueId();
    emit idChanged();
    m_chatModel->clear();
}

bool Chat::isModelLoaded() const
{
    return m_llmodel->isModelLoaded();
}

void Chat::prompt(const QString &prompt, const QString &prompt_template, int32_t n_predict, int32_t top_k, float top_p,
                 float temp, int32_t n_batch, float repeat_penalty, int32_t repeat_penalty_tokens)
{
    emit promptRequested(prompt, prompt_template, n_predict, top_k, top_p, temp, n_batch, repeat_penalty, repeat_penalty_tokens);
}

void Chat::regenerateResponse()
{
    emit regenerateResponseRequested(); // blocking queued connection
}

void Chat::stopGenerating()
{
    m_llmodel->stopGenerating();
}

QString Chat::response() const
{
    return m_llmodel->response();
}

void Chat::responseStarted()
{
    m_responseInProgress = true;
    emit responseInProgressChanged();
}

void Chat::responseStopped()
{
    m_responseInProgress = false;
    emit responseInProgressChanged();
    if (m_llmodel->generatedName().isEmpty())
        emit generateNameRequested();
}

QString Chat::modelName() const
{
    return m_llmodel->modelName();
}

void Chat::setModelName(const QString &modelName)
{
    // doesn't block but will unload old model and load new one which the gui can see through changes
    // to the isModelLoaded property
    emit modelNameChangeRequested(modelName);
}

void Chat::syncThreadCount() {
    emit setThreadCountRequested(m_desiredThreadCount);
}

void Chat::setThreadCount(int32_t n_threads) {
    if (n_threads <= 0)
        n_threads = std::min(4, (int32_t) std::thread::hardware_concurrency());
    m_desiredThreadCount = n_threads;
    syncThreadCount();
}

int32_t Chat::threadCount() {
    return m_llmodel->threadCount();
}

void Chat::newPromptResponsePair(const QString &prompt)
{
    m_chatModel->appendPrompt(tr("Prompt: "), prompt);
    m_chatModel->appendResponse(tr("Response: "), prompt);
    emit resetResponseRequested(); // blocking queued connection
}

bool Chat::isRecalc() const
{
    return m_llmodel->isRecalc();
}

void Chat::unload()
{
    stopGenerating();
    emit unloadRequested();
}

void Chat::reload()
{
    emit reloadRequested();
}

void Chat::generatedNameChanged()
{
    // Only use the first three words maximum and remove newlines and extra spaces
    QString gen = m_llmodel->generatedName().simplified();
    QStringList words = gen.split(' ', Qt::SkipEmptyParts);
    int wordCount = qMin(3, words.size());
    m_name = words.mid(0, wordCount).join(' ');
    emit nameChanged();
}
