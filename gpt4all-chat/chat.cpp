#include "chat.h"
#include "chatlistmodel.h"
#include "mysettings.h"
#include "modellist.h"
#include "network.h"
#include "server.h"

Chat::Chat(QObject *parent)
    : QObject(parent)
    , m_id(Network::globalInstance()->generateUniqueId())
    , m_name(tr("New Chat"))
    , m_chatModel(new ChatModel(this))
    , m_responseState(Chat::ResponseStopped)
    , m_creationDate(QDateTime::currentSecsSinceEpoch())
    , m_llmodel(new ChatLLM(this))
    , m_collectionModel(new LocalDocsCollectionsModel(this))
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
    , m_shouldLoadModelWhenInstalled(false)
    , m_collectionModel(new LocalDocsCollectionsModel(this))
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
    // Should be in different threads
    connect(m_llmodel, &ChatLLM::isModelLoadedChanged, this, &Chat::handleModelLoadedChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::responseChanged, this, &Chat::handleResponseChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::promptProcessing, this, &Chat::promptProcessing, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::responseStopped, this, &Chat::responseStopped, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::modelLoadingError, this, &Chat::handleModelLoadingError, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::recalcChanged, this, &Chat::handleRecalculating, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::generatedNameChanged, this, &Chat::generatedNameChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::reportSpeed, this, &Chat::handleTokenSpeedChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::reportDevice, this, &Chat::handleDeviceChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::reportFallbackReason, this, &Chat::handleFallbackReasonChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::databaseResultsChanged, this, &Chat::handleDatabaseResultsChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::modelInfoChanged, this, &Chat::handleModelInfoChanged, Qt::QueuedConnection);

    connect(this, &Chat::promptRequested, m_llmodel, &ChatLLM::prompt, Qt::QueuedConnection);
    connect(this, &Chat::modelChangeRequested, m_llmodel, &ChatLLM::modelChangeRequested, Qt::QueuedConnection);
    connect(this, &Chat::loadDefaultModelRequested, m_llmodel, &ChatLLM::loadDefaultModel, Qt::QueuedConnection);
    connect(this, &Chat::loadModelRequested, m_llmodel, &ChatLLM::loadModel, Qt::QueuedConnection);
    connect(this, &Chat::generateNameRequested, m_llmodel, &ChatLLM::generateName, Qt::QueuedConnection);
    connect(this, &Chat::regenerateResponseRequested, m_llmodel, &ChatLLM::regenerateResponse, Qt::QueuedConnection);
    connect(this, &Chat::resetResponseRequested, m_llmodel, &ChatLLM::resetResponse, Qt::QueuedConnection);
    connect(this, &Chat::resetContextRequested, m_llmodel, &ChatLLM::resetContext, Qt::QueuedConnection);
    connect(this, &Chat::processSystemPromptRequested, m_llmodel, &ChatLLM::processSystemPrompt, Qt::QueuedConnection);

    connect(this, &Chat::collectionListChanged, m_collectionModel, &LocalDocsCollectionsModel::setCollections);
    connect(ModelList::globalInstance()->installedModels(), &InstalledModels::countChanged,
        this, &Chat::handleModelInstalled, Qt::QueuedConnection);
}

void Chat::reset()
{
    stopGenerating();
    // Erase our current on disk representation as we're completely resetting the chat along with id
    ChatListModel::globalInstance()->removeChatFile(this);
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

void Chat::processSystemPrompt()
{
    emit processSystemPromptRequested();
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
    m_responseState = m_collections.empty() ? Chat::PromptProcessing : Chat::LocalDocsRetrieval;
    emit responseInProgressChanged();
    emit responseStateChanged();
}

void Chat::prompt(const QString &prompt)
{
    resetResponseState();
    emit promptRequested(m_collections, prompt);
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

Chat::ResponseState Chat::responseState() const
{
    return m_responseState;
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

    if (MySettings::globalInstance()->localDocsShowReferences()) {
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
    }

    m_responseInProgress = false;
    m_responseState = Chat::ResponseStopped;
    emit responseInProgressChanged();
    emit responseStateChanged();
    if (m_generatedName.isEmpty())
        emit generateNameRequested();
    if (chatModel()->count() < 3)
        Network::globalInstance()->sendChatStarted();
}

ModelInfo Chat::modelInfo() const
{
    return m_modelInfo;
}

void Chat::setModelInfo(const ModelInfo &modelInfo)
{
    if (m_modelInfo == modelInfo)
        return;

    m_isModelLoaded = false;
    emit isModelLoadedChanged();
    m_modelLoadingError = QString();
    emit modelLoadingErrorChanged();
    m_modelInfo = modelInfo;
    emit modelInfoChanged();
    emit modelChangeRequested(modelInfo);
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
    // If the installed model list is empty, then we mark a special flag and monitor for when a model
    // is installed
    if (!ModelList::globalInstance()->installedModels()->count()) {
        m_shouldLoadModelWhenInstalled = true;
        return;
    }
    m_llmodel->setShouldBeLoaded(true);
}

void Chat::handleModelInstalled()
{
    if (!m_shouldLoadModelWhenInstalled)
        return;
    m_shouldLoadModelWhenInstalled = false;
    reloadModel();
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

void Chat::handleDeviceChanged(const QString &device)
{
    m_device = device;
    emit deviceChanged();
}

void Chat::handleFallbackReasonChanged(const QString &fallbackReason)
{
    m_fallbackReason = fallbackReason;
    emit fallbackReasonChanged();
}

void Chat::handleDatabaseResultsChanged(const QList<ResultInfo> &results)
{
    m_databaseResults = results;
}

void Chat::handleModelInfoChanged(const ModelInfo &modelInfo)
{
    if (m_modelInfo == modelInfo)
        return;

    m_modelInfo = modelInfo;
    emit modelInfoChanged();
}

bool Chat::serialize(QDataStream &stream, int version) const
{
    stream << m_creationDate;
    stream << m_id;
    stream << m_name;
    stream << m_userName;
    if (version > 4)
        stream << m_modelInfo.id();
    else
        stream << m_modelInfo.filename();
    if (version > 2)
        stream << m_collections;

    const bool serializeKV = MySettings::globalInstance()->saveChatsContext();
    if (version > 5)
        stream << serializeKV;
    if (!m_llmodel->serialize(stream, version, serializeKV))
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
    m_generatedName = QLatin1String("nonempty");
    emit nameChanged();

    QString modelId;
    stream >> modelId;
    if (version > 4) {
        if (ModelList::globalInstance()->contains(modelId))
            m_modelInfo = ModelList::globalInstance()->modelInfo(modelId);
    } else {
        if (ModelList::globalInstance()->containsByFilename(modelId))
            m_modelInfo = ModelList::globalInstance()->modelInfoByFilename(modelId);
    }
    if (!m_modelInfo.id().isEmpty())
        emit modelInfoChanged();

    bool discardKV = m_modelInfo.id().isEmpty();

    // Prior to version 2 gptj models had a bug that fixed the kv_cache to F32 instead of F16 so
    // unfortunately, we cannot deserialize these
    if (version < 2 && m_modelInfo.filename().contains("gpt4all-j"))
        discardKV = true;

    if (version > 2) {
        stream >> m_collections;
        emit collectionListChanged(m_collections);
    }

    bool deserializeKV = true;
    if (version > 5)
        stream >> deserializeKV;

    m_llmodel->setModelInfo(m_modelInfo);
    if (!m_llmodel->deserialize(stream, version, deserializeKV, discardKV))
        return false;
    if (!m_chatModel->deserialize(stream, version))
        return false;

    m_llmodel->setStateFromText(m_chatModel->text());

    emit chatModelChanged();
    return stream.status() == QDataStream::Ok;
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
