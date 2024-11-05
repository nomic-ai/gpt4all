#include "chat.h"

#include "chatlistmodel.h"
#include "mysettings.h"
#include "network.h"
#include "server.h"

#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QLatin1String>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <Qt>
#include <QtLogging>

#include <utility>

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

Chat::Chat(server_tag_t, QObject *parent)
    : QObject(parent)
    , m_id(Network::globalInstance()->generateUniqueId())
    , m_name(tr("Server Chat"))
    , m_chatModel(new ChatModel(this))
    , m_responseState(Chat::ResponseStopped)
    , m_creationDate(QDateTime::currentSecsSinceEpoch())
    , m_llmodel(new Server(this))
    , m_isServer(true)
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
    connect(m_llmodel, &ChatLLM::modelLoadingPercentageChanged, this, &Chat::handleModelLoadingPercentageChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::responseChanged, this, &Chat::handleResponseChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::promptProcessing, this, &Chat::promptProcessing, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::generatingQuestions, this, &Chat::generatingQuestions, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::responseStopped, this, &Chat::responseStopped, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::modelLoadingError, this, &Chat::handleModelLoadingError, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::modelLoadingWarning, this, &Chat::modelLoadingWarning, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::restoringFromTextChanged, this, &Chat::handleRestoringFromText, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::generatedNameChanged, this, &Chat::generatedNameChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::generatedQuestionFinished, this, &Chat::generatedQuestionFinished, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::reportSpeed, this, &Chat::handleTokenSpeedChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::loadedModelInfoChanged, this, &Chat::loadedModelInfoChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::databaseResultsChanged, this, &Chat::handleDatabaseResultsChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::modelInfoChanged, this, &Chat::handleModelInfoChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::trySwitchContextOfLoadedModelCompleted, this, &Chat::handleTrySwitchContextOfLoadedModelCompleted, Qt::QueuedConnection);

    connect(this, &Chat::promptRequested, m_llmodel, &ChatLLM::prompt, Qt::QueuedConnection);
    connect(this, &Chat::modelChangeRequested, m_llmodel, &ChatLLM::modelChangeRequested, Qt::QueuedConnection);
    connect(this, &Chat::loadDefaultModelRequested, m_llmodel, &ChatLLM::loadDefaultModel, Qt::QueuedConnection);
    connect(this, &Chat::generateNameRequested, m_llmodel, &ChatLLM::generateName, Qt::QueuedConnection);
    connect(this, &Chat::regenerateResponseRequested, m_llmodel, &ChatLLM::regenerateResponse, Qt::QueuedConnection);
    connect(this, &Chat::resetResponseRequested, m_llmodel, &ChatLLM::resetResponse, Qt::QueuedConnection);
    connect(this, &Chat::resetContextRequested, m_llmodel, &ChatLLM::resetContext, Qt::QueuedConnection);
    connect(this, &Chat::processSystemPromptRequested, m_llmodel, &ChatLLM::processSystemPrompt, Qt::QueuedConnection);

    connect(this, &Chat::collectionListChanged, m_collectionModel, &LocalDocsCollectionsModel::setCollections);
}

void Chat::reset()
{
    stopGenerating();
    // Erase our current on disk representation as we're completely resetting the chat along with id
    ChatListModel::globalInstance()->removeChatFile(this);
    emit resetContextRequested();
    m_id = Network::globalInstance()->generateUniqueId();
    emit idChanged(m_id);
    // NOTE: We deliberately do no reset the name or creation date to indicate that this was originally
    // an older chat that was reset for another purpose. Resetting this data will lead to the chat
    // name label changing back to 'New Chat' and showing up in the chat model list as a 'New Chat'
    // further down in the list. This might surprise the user. In the future, we might get rid of
    // the "reset context" button in the UI. Right now, by changing the model in the combobox dropdown
    // we effectively do a reset context. We *have* to do this right now when switching between different
    // types of models. The only way to get rid of that would be a very long recalculate where we rebuild
    // the context if we switch between different types of models. Probably the right way to fix this
    // is to allow switching models but throwing up a dialog warning users if we switch between types
    // of models that a long recalculation will ensue.
    m_chatModel->clear();
    m_needsSave = true;
}

void Chat::processSystemPrompt()
{
    emit processSystemPromptRequested();
}

void Chat::resetResponseState()
{
    if (m_responseInProgress && m_responseState == Chat::LocalDocsRetrieval)
        return;

    m_generatedQuestions = QList<QString>();
    emit generatedQuestionsChanged();
    m_tokenSpeed = QString();
    emit tokenSpeedChanged();
    m_responseInProgress = true;
    m_responseState = m_collections.empty() ? Chat::PromptProcessing : Chat::LocalDocsRetrieval;
    emit responseInProgressChanged();
    emit responseStateChanged();
}

void Chat::newPromptResponsePair(const QString &prompt, const QList<QUrl> &attachedUrls)
{
    QStringList attachedContexts;
    QList<PromptAttachment> attachments;
    for (const QUrl &url : attachedUrls) {
        Q_ASSERT(url.isLocalFile());
        const QString localFilePath = url.toLocalFile();
        const QFileInfo info(localFilePath);

        Q_ASSERT(
            info.suffix().toLower() == "xlsx" ||
            info.suffix().toLower() == "txt" ||
            info.suffix().toLower() == "md" ||
            info.suffix().toLower() == "rst"
        );

        PromptAttachment attached;
        attached.url = url;

        QFile file(localFilePath);
        if (file.open(QIODevice::ReadOnly)) {
            attached.content = file.readAll();
            file.close();
        } else {
            qWarning() << "ERROR: Failed to open the attachment:" << localFilePath;
            continue;
        }

        attachments << attached;
        attachedContexts << attached.processedContent();
    }

    QString promptPlusAttached = prompt;
    if (!attachedContexts.isEmpty())
        promptPlusAttached = attachedContexts.join("\n\n") + "\n\n" + prompt;

    newPromptResponsePairInternal(prompt, attachments);
    emit resetResponseRequested();

    this->prompt(promptPlusAttached);
}

void Chat::prompt(const QString &prompt)
{
    resetResponseState();
    emit promptRequested(m_collections, prompt);
    m_needsSave = true;
}

void Chat::regenerateResponse()
{
    const int index = m_chatModel->count() - 1;
    m_chatModel->updateSources(index, QList<ResultInfo>());
    emit regenerateResponseRequested();
    m_needsSave = true;
}

void Chat::stopGenerating()
{
    m_llmodel->stopGenerating();
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

    const int index = m_chatModel->count() - 1;
    m_chatModel->updateValue(index, response);
}

void Chat::handleModelLoadingPercentageChanged(float loadingPercentage)
{
    if (m_shouldDeleteLater)
        deleteLater();

    if (loadingPercentage == m_modelLoadingPercentage)
        return;

    bool wasLoading = isCurrentlyLoading();
    bool wasLoaded = isModelLoaded();

    m_modelLoadingPercentage = loadingPercentage;
    emit modelLoadingPercentageChanged();

    if (isCurrentlyLoading() != wasLoading)
        emit isCurrentlyLoadingChanged();

    if (isModelLoaded() != wasLoaded)
        emit isModelLoadedChanged();
}

void Chat::promptProcessing()
{
    m_responseState = !databaseResults().isEmpty() ? Chat::LocalDocsProcessing : Chat::PromptProcessing;
    emit responseStateChanged();
}

void Chat::generatingQuestions()
{
    m_responseState = Chat::GeneratingQuestions;
    emit responseStateChanged();
}

void Chat::responseStopped(qint64 promptResponseMs)
{
    m_tokenSpeed = QString();
    emit tokenSpeedChanged();

    m_responseInProgress = false;
    m_responseState = Chat::ResponseStopped;
    emit responseInProgressChanged();
    emit responseStateChanged();
    if (m_generatedName.isEmpty())
        emit generateNameRequested();

    Network::globalInstance()->trackChatEvent("response_complete", {
        {"first", m_firstResponse},
        {"message_count", chatModel()->count()},
        {"$duration", promptResponseMs / 1000.},
    });
    m_firstResponse = false;
}

ModelInfo Chat::modelInfo() const
{
    return m_modelInfo;
}

void Chat::setModelInfo(const ModelInfo &modelInfo)
{
    if (m_modelInfo != modelInfo) {
        m_modelInfo = modelInfo;
        m_needsSave = true;
    } else if (isModelLoaded())
        return;

    emit modelInfoChanged();
    emit modelChangeRequested(modelInfo);
}

// the server needs to block until response is reset, so it calls resetResponse on its own m_llmThread
void Chat::serverNewPromptResponsePair(const QString &prompt, const QList<PromptAttachment> &attachments)
{
    newPromptResponsePairInternal(prompt, attachments);
}

void Chat::newPromptResponsePairInternal(const QString &prompt, const QList<PromptAttachment> &attachments)
{
    resetResponseState();
    m_chatModel->updateCurrentResponse(m_chatModel->count() - 1, false);
    m_chatModel->appendPrompt("Prompt: ", prompt, attachments);
    m_chatModel->appendResponse("Response: ");
}

bool Chat::restoringFromText() const
{
    return m_llmodel->restoringFromText();
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

void Chat::markForDeletion()
{
    m_llmodel->setMarkedForDeletion(true);
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

void Chat::forceUnloadModel()
{
    stopGenerating();
    m_llmodel->setForceUnloadModel(true);
    m_llmodel->setShouldBeLoaded(false);
}

void Chat::forceReloadModel()
{
    m_llmodel->setForceUnloadModel(true);
    m_llmodel->setShouldBeLoaded(true);
}

void Chat::trySwitchContextOfLoadedModel()
{
    m_trySwitchContextInProgress = 1;
    emit trySwitchContextInProgressChanged();
    m_llmodel->requestTrySwitchContext();
}

void Chat::generatedNameChanged(const QString &name)
{
    // Only use the first three words maximum and remove newlines and extra spaces
    m_generatedName = name.simplified();
    QStringList words = m_generatedName.split(' ', Qt::SkipEmptyParts);
    int wordCount = qMin(7, words.size());
    m_name = words.mid(0, wordCount).join(' ');
    emit nameChanged();
    m_needsSave = true;
}

void Chat::generatedQuestionFinished(const QString &question)
{
    m_generatedQuestions << question;
    emit generatedQuestionsChanged();
    m_needsSave = true;
}

void Chat::handleRestoringFromText()
{
    Network::globalInstance()->trackChatEvent("recalc_context", { {"length", m_chatModel->count()} });
    emit restoringFromTextChanged();
}

void Chat::handleModelLoadingError(const QString &error)
{
    if (!error.isEmpty()) {
        auto stream = qWarning().noquote() << "ERROR:" << error << "id";
        stream.quote() << id();
    }
    m_modelLoadingError = error;
    emit modelLoadingErrorChanged();
}

void Chat::handleTokenSpeedChanged(const QString &tokenSpeed)
{
    m_tokenSpeed = tokenSpeed;
    emit tokenSpeedChanged();
}

QString Chat::deviceBackend() const
{
    return m_llmodel->deviceBackend();
}

QString Chat::device() const
{
    return m_llmodel->device();
}

QString Chat::fallbackReason() const
{
    return m_llmodel->fallbackReason();
}

void Chat::handleDatabaseResultsChanged(const QList<ResultInfo> &results)
{
    m_databaseResults = results;
    const int index = m_chatModel->count() - 1;
    m_chatModel->updateSources(index, m_databaseResults);
    m_needsSave = true;
}

void Chat::handleModelInfoChanged(const ModelInfo &modelInfo)
{
    if (m_modelInfo == modelInfo)
        return;

    m_modelInfo = modelInfo;
    emit modelInfoChanged();
    m_needsSave = true;
}

void Chat::handleTrySwitchContextOfLoadedModelCompleted(int value)
{
    m_trySwitchContextInProgress = value;
    emit trySwitchContextInProgressChanged();
}

bool Chat::serialize(QDataStream &stream, int version) const
{
    stream << m_creationDate;
    stream << m_id;
    stream << m_name;
    stream << m_userName;
    if (version >= 5)
        stream << m_modelInfo.id();
    else
        stream << m_modelInfo.filename();
    if (version >= 3)
        stream << m_collections;

    const bool serializeKV = MySettings::globalInstance()->saveChatsContext();
    if (version >= 6)
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
    if (version >= 5) {
        if (ModelList::globalInstance()->contains(modelId))
            m_modelInfo = ModelList::globalInstance()->modelInfo(modelId);
    } else {
        if (ModelList::globalInstance()->containsByFilename(modelId))
            m_modelInfo = ModelList::globalInstance()->modelInfoByFilename(modelId);
    }
    if (!m_modelInfo.id().isEmpty())
        emit modelInfoChanged();

    bool discardKV = m_modelInfo.id().isEmpty();

    if (version >= 3) {
        stream >> m_collections;
        emit collectionListChanged(m_collections);
    }

    bool deserializeKV = true;
    if (version >= 6)
        stream >> deserializeKV;

    m_llmodel->setModelInfo(m_modelInfo);
    if (!m_llmodel->deserialize(stream, version, deserializeKV, discardKV))
        return false;
    if (!m_chatModel->deserialize(stream, version))
        return false;

    emit chatModelChanged();
    if (stream.status() != QDataStream::Ok)
        return false;

    m_needsSave = false;
    return true;
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
    m_needsSave = true;
}

void Chat::removeCollection(const QString &collection)
{
    if (!hasCollection(collection))
        return;

    m_collections.removeAll(collection);
    emit collectionListChanged(m_collections);
    m_needsSave = true;
}
