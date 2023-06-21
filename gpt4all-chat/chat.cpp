#include "chat.h"
#include "llm.h"
#include "localdocs.h"
#include "network.h"
#include "download.h"
#include "server.h"

Chat::Chat(QObject *parent)
    : QObject(parent)
    , m_id(Network::globalInstance()->generateUniqueId())
    , m_name(tr("New Chat"))
    , m_chatModel(new ChatModel(this))
    , m_responseInProgress(false)
    , m_responseState(Chat::ResponseStopped)
    , m_creationDate(QDateTime::currentSecsSinceEpoch())
    , m_llmodel(new ChatLLM(this))
    , m_isServer(false)
    , m_shouldDeleteLater(false)
    , m_isModelLoaded(false)
{
    connectLLM();
}

Chat::Chat(bool isServer, QObject *parent)
    : QObject(parent)
    , m_id(Network::globalInstance()->generateUniqueId())
    , m_name(tr("Server Chat"))
    , m_chatModel(new ChatModel(this))
    , m_responseInProgress(false)
    , m_responseState(Chat::ResponseStopped)
    , m_creationDate(QDateTime::currentSecsSinceEpoch())
    , m_llmodel(new Server(this))
    , m_isServer(true)
    , m_shouldDeleteLater(false)
    , m_isModelLoaded(false)
{
    connectLLM();
}

Chat::~Chat()
{
    delete m_llmodel;
    m_llmodel = nullptr;
}

void Chat::connectLLM()
{
    const QString exePath = QCoreApplication::applicationDirPath() + QDir::separator();
    const QString localPath = Download::globalInstance()->downloadLocalModelsPath();
    m_watcher = new QFileSystemWatcher(this);
    m_watcher->addPath(exePath);
    m_watcher->addPath(localPath);

    // Should be in same thread
    connect(Download::globalInstance(), &Download::modelListChanged, this, &Chat::handleModelListChanged, Qt::DirectConnection);
    connect(this, &Chat::modelNameChanged, this, &Chat::handleModelListChanged, Qt::DirectConnection);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &Chat::handleModelListChanged);

    // Should be in different threads
    connect(m_llmodel, &ChatLLM::isModelLoadedChanged, this, &Chat::handleModelLoadedChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::responseChanged, this, &Chat::handleResponseChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::promptProcessing, this, &Chat::promptProcessing, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::responseStopped, this, &Chat::responseStopped, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::modelLoadingError, this, &Chat::handleModelLoadingError, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::recalcChanged, this, &Chat::handleRecalculating, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::generatedNameChanged, this, &Chat::generatedNameChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::reportSpeed, this, &Chat::handleTokenSpeedChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::databaseResultsChanged, this, &Chat::handleDatabaseResultsChanged, Qt::QueuedConnection);

    connect(this, &Chat::promptRequested, m_llmodel, &ChatLLM::prompt, Qt::QueuedConnection);
    connect(this, &Chat::modelNameChangeRequested, m_llmodel, &ChatLLM::modelNameChangeRequested, Qt::QueuedConnection);
    connect(this, &Chat::loadDefaultModelRequested, m_llmodel, &ChatLLM::loadDefaultModel, Qt::QueuedConnection);
    connect(this, &Chat::loadModelRequested, m_llmodel, &ChatLLM::loadModel, Qt::QueuedConnection);
    connect(this, &Chat::generateNameRequested, m_llmodel, &ChatLLM::generateName, Qt::QueuedConnection);
    connect(this, &Chat::regenerateResponseRequested, m_llmodel, &ChatLLM::regenerateResponse, Qt::QueuedConnection);
    connect(this, &Chat::resetResponseRequested, m_llmodel, &ChatLLM::resetResponse, Qt::QueuedConnection);
    connect(this, &Chat::resetContextRequested, m_llmodel, &ChatLLM::resetContext, Qt::QueuedConnection);

    emit defaultModelChanged(modelList().first());
}

void Chat::reset()
{
    stopGenerating();
    // Erase our current on disk representation as we're completely resetting the chat along with id
    LLM::globalInstance()->chatListModel()->removeChatFile(this);
    emit resetContextRequested();
    m_id = Network::globalInstance()->generateUniqueId();
    emit idChanged(m_id);
    // NOTE: We deliberately do no reset the name or creation date to indictate that this was originally
    // an older chat that was reset for another purpose. Resetting this data will lead to the chat
    // name label changing back to 'New Chat' and showing up in the chat model list as a 'New Chat'
    // further down in the list. This might surprise the user. In the future, we me might get rid of
    // the "reset context" button in the UI. Right now, by changing the model in the combobox dropdown
    // we effectively do a reset context. We *have* to do this right now when switching between different
    // types of models. The only way to get rid of that would be a very long recalculate where we rebuild
    // the context if we switch between different types of models. Probably the right way to fix this
    // is to allow switching models but throwing up a dialog warning users if we switch between types
    // of models that a long recalculation will ensue.
    m_chatModel->clear();
}

bool Chat::isModelLoaded() const
{
    return m_isModelLoaded;
}

void Chat::resetResponseState()
{
    if (m_responseInProgress && m_responseState == Chat::LocalDocsRetrieval)
        return;

    m_tokenSpeed = QString();
    emit tokenSpeedChanged();
    m_responseInProgress = true;
    m_responseState = Chat::LocalDocsRetrieval;
    emit responseInProgressChanged();
    emit responseStateChanged();
}

void Chat::prompt(const QString &prompt, const QString &prompt_template, int32_t n_predict,
    int32_t top_k, float top_p, float temp, int32_t n_batch, float repeat_penalty,
    int32_t repeat_penalty_tokens)
{
    resetResponseState();
    emit promptRequested(
        m_collections,
        prompt,
        prompt_template,
        n_predict,
        top_k,
        top_p,
        temp,
        n_batch,
        repeat_penalty,
        repeat_penalty_tokens,
        LLM::globalInstance()->threadCount());
}

void Chat::regenerateResponse()
{
    const int index = m_chatModel->count() - 1;
    m_chatModel->updateReferences(index, QString(), QList<QString>());
    emit regenerateResponseRequested();
}

void Chat::stopGenerating()
{
    m_llmodel->stopGenerating();
}

QString Chat::response() const
{
    return m_response;
}

QString Chat::responseState() const
{
    switch (m_responseState) {
    case ResponseStopped: return QStringLiteral("response stopped");
    case LocalDocsRetrieval: return QStringLiteral("retrieving ") + m_collections.join(", ");
    case LocalDocsProcessing: return QStringLiteral("processing ") + m_collections.join(", ");
    case PromptProcessing: return QStringLiteral("processing");
    case ResponseGeneration: return QStringLiteral("generating response");
    };
    Q_UNREACHABLE();
    return QString();
}

void Chat::handleResponseChanged(const QString &response)
{
    if (m_responseState != Chat::ResponseGeneration) {
        m_responseState = Chat::ResponseGeneration;
        emit responseStateChanged();
    }

    m_response = response;
    const int index = m_chatModel->count() - 1;
    m_chatModel->updateValue(index, this->response());
    emit responseChanged();
}

void Chat::handleModelLoadedChanged(bool loaded)
{
    if (m_shouldDeleteLater)
        deleteLater();

    if (loaded == m_isModelLoaded)
        return;

    m_isModelLoaded = loaded;
    emit isModelLoadedChanged();
}

void Chat::promptProcessing()
{
    m_responseState = !databaseResults().isEmpty() ? Chat::LocalDocsProcessing : Chat::PromptProcessing;
    emit responseStateChanged();
}

void Chat::responseStopped()
{
    m_tokenSpeed = QString();
    emit tokenSpeedChanged();

    const QString chatResponse = response();
    QList<QString> references;
    QList<QString> referencesContext;
    int validReferenceNumber = 1;
    for (const ResultInfo &info : databaseResults()) {
        if (info.file.isEmpty())
            continue;
        if (validReferenceNumber == 1)
            references.append((!chatResponse.endsWith("\n") ? "\n" : QString()) + QStringLiteral("\n---"));
        QString reference;
        {
            QTextStream stream(&reference);
            stream << (validReferenceNumber++) << ". ";
            if (!info.title.isEmpty())
                stream << "\"" << info.title << "\". ";
            if (!info.author.isEmpty())
                stream << "By " << info.author << ". ";
            if (!info.date.isEmpty())
                stream << "Date: " << info.date << ". ";
            stream << "In " << info.file << ". ";
            if (info.page != -1)
                stream << "Page " << info.page << ". ";
            if (info.from != -1) {
                stream << "Lines " << info.from;
                if (info.to != -1)
                    stream << "-" << info.to;
                stream << ". ";
            }
            stream << "[Context](context://" << validReferenceNumber - 1 << ")";
        }
        references.append(reference);
        referencesContext.append(info.text);
    }

    const int index = m_chatModel->count() - 1;
    m_chatModel->updateReferences(index, references.join("\n"), referencesContext);
    emit responseChanged();

    m_responseInProgress = false;
    m_responseState = Chat::ResponseStopped;
    emit responseInProgressChanged();
    emit responseStateChanged();
    if (m_generatedName.isEmpty())
        emit generateNameRequested();
    if (chatModel()->count() < 3)
        Network::globalInstance()->sendChatStarted();
}

QString Chat::modelName() const
{
    return m_modelName;
}

void Chat::setModelName(const QString &modelName)
{
    if (m_modelName == modelName)
        return;

    m_modelLoadingError = QString();
    emit modelLoadingErrorChanged();
    m_modelName = modelName;
    emit modelNameChanged();
    emit modelNameChangeRequested(modelName);
}

void Chat::newPromptResponsePair(const QString &prompt)
{
    resetResponseState();
    m_chatModel->updateCurrentResponse(m_chatModel->count() - 1, false);
    m_chatModel->appendPrompt(tr("Prompt: "), prompt);
    m_chatModel->appendResponse(tr("Response: "), prompt);
    emit resetResponseRequested();
}

void Chat::serverNewPromptResponsePair(const QString &prompt)
{
    resetResponseState();
    m_chatModel->updateCurrentResponse(m_chatModel->count() - 1, false);
    m_chatModel->appendPrompt(tr("Prompt: "), prompt);
    m_chatModel->appendResponse(tr("Response: "), prompt);
}

bool Chat::isRecalc() const
{
    return m_llmodel->isRecalc();
}

void Chat::loadDefaultModel()
{
    emit defaultModelChanged(modelList().first());
    m_modelLoadingError = QString();
    emit modelLoadingErrorChanged();
    emit loadDefaultModelRequested();
}

void Chat::loadModel(const QString &modelName)
{
    m_modelLoadingError = QString();
    emit modelLoadingErrorChanged();
    emit loadModelRequested(modelName);
}

void Chat::unloadAndDeleteLater()
{
    if (!isModelLoaded()) {
        deleteLater();
        return;
    }

    m_shouldDeleteLater = true;
    unloadModel();
}

void Chat::unloadModel()
{
    stopGenerating();
    m_llmodel->setShouldBeLoaded(false);
}

void Chat::reloadModel()
{
    m_llmodel->setShouldBeLoaded(true);
}

void Chat::generatedNameChanged(const QString &name)
{
    // Only use the first three words maximum and remove newlines and extra spaces
    m_generatedName = name.simplified();
    QStringList words = m_generatedName.split(' ', Qt::SkipEmptyParts);
    int wordCount = qMin(3, words.size());
    m_name = words.mid(0, wordCount).join(' ');
    emit nameChanged();
}

void Chat::handleRecalculating()
{
    Network::globalInstance()->sendRecalculatingContext(m_chatModel->count());
    emit recalcChanged();
}

void Chat::handleModelLoadingError(const QString &error)
{
    qWarning() << "ERROR:" << qPrintable(error) << "id" << id();
    m_modelLoadingError = error;
    emit modelLoadingErrorChanged();
}

void Chat::handleTokenSpeedChanged(const QString &tokenSpeed)
{
    m_tokenSpeed = tokenSpeed;
    emit tokenSpeedChanged();
}

void Chat::handleDatabaseResultsChanged(const QList<ResultInfo> &results)
{
    m_databaseResults = results;
}

bool Chat::serialize(QDataStream &stream, int version) const
{
    stream << m_creationDate;
    stream << m_id;
    stream << m_name;
    stream << m_userName;
    stream << m_modelName;
    if (version > 2)
        stream << m_collections;
    if (!m_llmodel->serialize(stream, version))
        return false;
    if (!m_chatModel->serialize(stream, version))
        return false;
    return stream.status() == QDataStream::Ok;
}

bool Chat::deserialize(QDataStream &stream, int version)
{
    stream >> m_creationDate;
    stream >> m_id;
    emit idChanged(m_id);
    stream >> m_name;
    stream >> m_userName;
    emit nameChanged();
    stream >> m_modelName;
    emit modelNameChanged();
    // Prior to version 2 gptj models had a bug that fixed the kv_cache to F32 instead of F16 so
    // unfortunately, we cannot deserialize these
    if (version < 2 && m_modelName.contains("gpt4all-j"))
        return false;
    if (version > 2) {
        stream >> m_collections;
        emit collectionListChanged(m_collections);
    }
    m_llmodel->setModelName(m_modelName);
    if (!m_llmodel->deserialize(stream, version))
        return false;
    if (!m_chatModel->deserialize(stream, version))
        return false;
    emit chatModelChanged();
    return stream.status() == QDataStream::Ok;
}

QList<QString> Chat::modelList() const
{
    // Build a model list from exepath and from the localpath
    QList<QString> list;

    QString exePath = QCoreApplication::applicationDirPath() + QDir::separator();
    QString localPath = Download::globalInstance()->downloadLocalModelsPath();

    QSettings settings;
    settings.sync();
    // The user default model can be set by the user in the settings dialog. The "default" user
    // default model is "Application default" which signals we should use the default model that was
    // specified by the models.json file.
    QString defaultModel = settings.value("userDefaultModel").toString();
    if (defaultModel.isEmpty() || defaultModel == "Application default")
        defaultModel = settings.value("defaultModel").toString();

    QString currentModelName = modelName().isEmpty() ? defaultModel : modelName();

    {
        QDir dir(exePath);
        QStringList allFiles = dir.entryList(QDir::Files);

        // All files that end with .bin and have 'ggml' somewhere in the name
        QStringList fileNames;
        for(const QString& filename : allFiles) {
            if (filename.endsWith(".bin") && filename.contains("ggml")) {
                fileNames.append(filename);
            }
        }

        for (const QString& f : fileNames) {
            QString filePath = exePath + f;
            QFileInfo info(filePath);
            QString basename = info.completeBaseName();
            QString name = basename.startsWith("ggml-") ? basename.remove(0, 5) : basename;
            if (info.exists()) {
                if (name == currentModelName)
                    list.prepend(name);
                else
                    list.append(name);
            }
        }
    }

    if (localPath != exePath) {
        QDir dir(localPath);
        QStringList allFiles = dir.entryList(QDir::Files);
        QStringList fileNames;
        for(const QString& filename : allFiles) {
            if ((filename.endsWith(".bin") && filename.contains("ggml"))
                || (filename.endsWith(".txt") && filename.startsWith("chatgpt-"))) {
                fileNames.append(filename);
            }
        }

        for (const QString &f : fileNames) {
            QString filePath = localPath + f;
            QFileInfo info(filePath);
            QString basename = info.completeBaseName();
            QString name = basename.startsWith("ggml-") ? basename.remove(0, 5) : basename;
            if (info.exists() && !list.contains(name)) { // don't allow duplicates
                if (name == currentModelName)
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

void Chat::handleModelListChanged()
{
    emit modelListChanged();
    emit defaultModelChanged(modelList().first());
}

void Chat::handleDownloadLocalModelsPathChanged()
{
    emit modelListChanged();
    emit defaultModelChanged(modelList().first());
    m_watcher->addPath(Download::globalInstance()->downloadLocalModelsPath());
}

QList<QString> Chat::collectionList() const
{
    return m_collections;
}

bool Chat::hasCollection(const QString &collection) const
{
    return m_collections.contains(collection);
}

void Chat::addCollection(const QString &collection)
{
    if (hasCollection(collection))
        return;

    m_collections.append(collection);
    emit collectionListChanged(m_collections);
}

void Chat::removeCollection(const QString &collection)
{
    if (!hasCollection(collection))
        return;

    m_collections.removeAll(collection);
    emit collectionListChanged(m_collections);
}
