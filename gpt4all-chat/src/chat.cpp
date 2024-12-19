#include "chat.h"

#include "chatlistmodel.h"
#include "network.h"
#include "server.h"
#include "tool.h"
#include "toolcallparser.h"
#include "toolmodel.h"

#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLatin1String>
#include <QMap>
#include <QRegularExpression>
#include <QString>
#include <QVariant>
#include <Qt>
#include <QtLogging>

#include <utility>

using namespace ToolEnums;

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
    connect(m_llmodel, &ChatLLM::generatedNameChanged, this, &Chat::generatedNameChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::generatedQuestionFinished, this, &Chat::generatedQuestionFinished, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::reportSpeed, this, &Chat::handleTokenSpeedChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::loadedModelInfoChanged, this, &Chat::loadedModelInfoChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::databaseResultsChanged, this, &Chat::handleDatabaseResultsChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::modelInfoChanged, this, &Chat::handleModelChanged, Qt::QueuedConnection);
    connect(m_llmodel, &ChatLLM::trySwitchContextOfLoadedModelCompleted, this, &Chat::handleTrySwitchContextOfLoadedModelCompleted, Qt::QueuedConnection);

    connect(this, &Chat::promptRequested, m_llmodel, &ChatLLM::prompt, Qt::QueuedConnection);
    connect(this, &Chat::modelChangeRequested, m_llmodel, &ChatLLM::modelChangeRequested, Qt::QueuedConnection);
    connect(this, &Chat::loadDefaultModelRequested, m_llmodel, &ChatLLM::loadDefaultModel, Qt::QueuedConnection);
    connect(this, &Chat::generateNameRequested, m_llmodel, &ChatLLM::generateName, Qt::QueuedConnection);
    connect(this, &Chat::regenerateResponseRequested, m_llmodel, &ChatLLM::regenerateResponse, Qt::QueuedConnection);

    connect(this, &Chat::collectionListChanged, m_collectionModel, &LocalDocsCollectionsModel::setCollections);

    connect(ModelList::globalInstance(), &ModelList::modelInfoChanged, this, &Chat::handleModelInfoChanged);
}

void Chat::reset()
{
    stopGenerating();
    // Erase our current on disk representation as we're completely resetting the chat along with id
    ChatListModel::globalInstance()->removeChatFile(this);
    m_id = Network::globalInstance()->generateUniqueId();
    emit idChanged(m_id);
    // NOTE: We deliberately do no reset the name or creation date to indicate that this was originally
    // an older chat that was reset for another purpose. Resetting this data will lead to the chat
    // name label changing back to 'New Chat' and showing up in the chat model list as a 'New Chat'
    // further down in the list. This might surprise the user. In the future, we might get rid of
    // the "reset context" button in the UI.
    m_chatModel->clear();
    m_needsSave = true;
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

    resetResponseState();
    if (int count = m_chatModel->count())
        m_chatModel->updateCurrentResponse(count - 1, false);
    m_chatModel->appendPrompt(prompt, attachments);
    m_chatModel->appendResponse();

    emit promptRequested(m_collections);
    m_needsSave = true;
}

void Chat::regenerateResponse(int index)
{
    resetResponseState();
    emit regenerateResponseRequested(index);
    m_needsSave = true;
}

QVariant Chat::popPrompt(int index)
{
    auto content = m_llmodel->popPrompt(index);
    m_needsSave = true;
    if (content) return *content;
    return QVariant::fromValue(nullptr);
}

void Chat::stopGenerating()
{
    m_llmodel->stopGenerating();
}

Chat::ResponseState Chat::responseState() const
{
    return m_responseState;
}

void Chat::handleResponseChanged()
{
    if (m_responseState != Chat::ResponseGeneration) {
        m_responseState = Chat::ResponseGeneration;
        emit responseStateChanged();
    }
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

    const QString possibleToolcall = m_chatModel->possibleToolcall();

    ToolCallParser parser;
    parser.update(possibleToolcall);

    if (parser.state() == ToolEnums::ParseState::Complete) {
        const QString toolCall = parser.toolCall();

        // Regex to remove the formatting around the code
        static const QRegularExpression regex("^\\s*```javascript\\s*|\\s*```\\s*$");
        QString code = toolCall;
        code.remove(regex);
        code = code.trimmed();

        // Right now the code interpreter is the only available tool
        Tool *toolInstance = ToolModel::globalInstance()->get(ToolCallConstants::CodeInterpreterFunction);
        Q_ASSERT(toolInstance);

        // The param is the code
        const ToolParam param = { "code", ToolEnums::ParamType::String, code };
        const QString result = toolInstance->run({param}, 10000 /*msecs to timeout*/);
        const ToolEnums::Error error = toolInstance->error();
        const QString errorString = toolInstance->errorString();

        // Update the current response with meta information about toolcall and re-parent
        m_chatModel->updateToolCall({
            ToolCallConstants::CodeInterpreterFunction,
            { param },
            result,
            error,
            errorString
        });

        ++m_consecutiveToolCalls;

        // We limit the number of consecutive toolcalls otherwise we get into a potentially endless loop
        if (m_consecutiveToolCalls < 3 || error == ToolEnums::Error::NoError) {
            resetResponseState();
            emit promptRequested(m_collections); // triggers a new response
            return;
        }
    }

    if (m_generatedName.isEmpty())
        emit generateNameRequested();

    m_consecutiveToolCalls = 0;
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
    m_needsSave = true;
}

// we need to notify listeners of the modelInfo property when its properties are updated,
// since it's a gadget and can't do that on its own
void Chat::handleModelInfoChanged(const ModelInfo &modelInfo)
{
    if (!m_modelInfo.id().isNull() && modelInfo.id() == m_modelInfo.id())
        emit modelInfoChanged();
}

// react if a new model is loaded
void Chat::handleModelChanged(const ModelInfo &modelInfo)
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

    if (version >= 3) {
        stream >> m_collections;
        emit collectionListChanged(m_collections);
    }

    m_llmodel->setModelInfo(m_modelInfo);
    if (!m_llmodel->deserialize(stream, version))
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
