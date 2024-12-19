#include "modellist.h"

#include "download.h"
#include "jinja_replacements.h"
#include "mysettings.h"
#include "network.h"

#include <gpt4all-backend/llmodel.h>

#include <QChar>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QGlobalStatic>
#include <QGuiApplication>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkRequest>
#include <QObject>
#include <QRegularExpression>
#include <QSettings>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QStandardPaths>
#include <QStringList>
#include <QTextStream>
#include <QTimer>
#include <QUrl>
#include <QtLogging>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include <string>
#include <utility>

using namespace Qt::Literals::StringLiterals;

//#define USE_LOCAL_MODELSJSON

#define MODELS_JSON_VERSION "3"


static const QStringList FILENAME_BLACKLIST { u"gpt4all-nomic-embed-text-v1.rmodel"_s };

static const QString RMODEL_CHAT_TEMPLATE = uR"(<chat>
{%- set loop_messages = messages %}
{%- for message in loop_messages %}
    {%- if not message['role'] in ['user', 'assistant', 'system'] %}
        {{- raise_exception('Unknown role: ' + message['role']) }}
    {%- endif %}
    {{- '<' + message['role'] + '>' }}
    {%- if message['role'] == 'user' %}
        {%- for source in message.sources %}
            {%- if loop.first %}
                {{- '### Context:\n' }}
            {%- endif %}
            {{- ('Collection: ' + source.collection + '\n'    +
                 'Path: '       + source.path       + '\n'    +
                 'Excerpt: '    + source.text       + '\n\n') | escape }}
        {%- endfor %}
    {%- endif %}
    {%- for attachment in message.prompt_attachments %}
        {{- (attachment.processed_content + '\n\n') | escape }}
    {%- endfor %}
    {{- message.content | escape }}
    {{- '</' + message['role'] + '>' }}
{%- endfor %}
</chat>)"_s;


QString ModelInfo::id() const
{
    return m_id;
}

void ModelInfo::setId(const QString &id)
{
    m_id = id;
}

QString ModelInfo::name() const
{
    return MySettings::globalInstance()->modelName(*this);
}

void ModelInfo::setName(const QString &name)
{
    if (shouldSaveMetadata()) MySettings::globalInstance()->setModelName(*this, name, true /*force*/);
    m_name = name;
}

QString ModelInfo::filename() const
{
    return MySettings::globalInstance()->modelFilename(*this);
}

void ModelInfo::setFilename(const QString &filename)
{
    if (shouldSaveMetadata()) MySettings::globalInstance()->setModelFilename(*this, filename, true /*force*/);
    m_filename = filename;
}

QString ModelInfo::description() const
{
    return MySettings::globalInstance()->modelDescription(*this);
}

void ModelInfo::setDescription(const QString &d)
{
    if (shouldSaveMetadata()) MySettings::globalInstance()->setModelDescription(*this, d, true /*force*/);
    m_description = d;
}

QString ModelInfo::url() const
{
    return MySettings::globalInstance()->modelUrl(*this);
}

void ModelInfo::setUrl(const QString &u)
{
    if (shouldSaveMetadata()) MySettings::globalInstance()->setModelUrl(*this, u, true /*force*/);
    m_url = u;
}

QString ModelInfo::quant() const
{
    return MySettings::globalInstance()->modelQuant(*this);
}

void ModelInfo::setQuant(const QString &q)
{
    if (shouldSaveMetadata()) MySettings::globalInstance()->setModelQuant(*this, q, true /*force*/);
    m_quant = q;
}

QString ModelInfo::type() const
{
    return MySettings::globalInstance()->modelType(*this);
}

void ModelInfo::setType(const QString &t)
{
    if (shouldSaveMetadata()) MySettings::globalInstance()->setModelType(*this, t, true /*force*/);
    m_type = t;
}

bool ModelInfo::isClone() const
{
    return MySettings::globalInstance()->modelIsClone(*this);
}

void ModelInfo::setIsClone(bool b)
{
    if (shouldSaveMetadata()) MySettings::globalInstance()->setModelIsClone(*this, b, true /*force*/);
    m_isClone = b;
}

bool ModelInfo::isDiscovered() const
{
    return MySettings::globalInstance()->modelIsDiscovered(*this);
}

void ModelInfo::setIsDiscovered(bool b)
{
    if (shouldSaveMetadata()) MySettings::globalInstance()->setModelIsDiscovered(*this, b, true /*force*/);
    m_isDiscovered = b;
}

int ModelInfo::likes() const
{
    return MySettings::globalInstance()->modelLikes(*this);
}

void ModelInfo::setLikes(int l)
{
    if (shouldSaveMetadata()) MySettings::globalInstance()->setModelLikes(*this, l, true /*force*/);
    m_likes = l;
}

int ModelInfo::downloads() const
{
    return MySettings::globalInstance()->modelDownloads(*this);
}

void ModelInfo::setDownloads(int d)
{
    if (shouldSaveMetadata()) MySettings::globalInstance()->setModelDownloads(*this, d, true /*force*/);
    m_downloads = d;
}

QDateTime ModelInfo::recency() const
{
    return MySettings::globalInstance()->modelRecency(*this);
}

void ModelInfo::setRecency(const QDateTime &r)
{
    if (shouldSaveMetadata()) MySettings::globalInstance()->setModelRecency(*this, r, true /*force*/);
    m_recency = r;
}

double ModelInfo::temperature() const
{
    return MySettings::globalInstance()->modelTemperature(*this);
}

void ModelInfo::setTemperature(double t)
{
    if (shouldSaveMetadata()) MySettings::globalInstance()->setModelTemperature(*this, t, true /*force*/);
    m_temperature = t;
}

double ModelInfo::topP() const
{
    return MySettings::globalInstance()->modelTopP(*this);
}

double ModelInfo::minP() const
{
    return MySettings::globalInstance()->modelMinP(*this);
}

void ModelInfo::setTopP(double p)
{
    if (shouldSaveMetadata()) MySettings::globalInstance()->setModelTopP(*this, p, true /*force*/);
    m_topP = p;
}

void ModelInfo::setMinP(double p)
{
    if (shouldSaveMetadata()) MySettings::globalInstance()->setModelMinP(*this, p, true /*force*/);
    m_minP = p;
}

int ModelInfo::topK() const
{
    return MySettings::globalInstance()->modelTopK(*this);
}

void ModelInfo::setTopK(int k)
{
    if (shouldSaveMetadata()) MySettings::globalInstance()->setModelTopK(*this, k, true /*force*/);
    m_topK = k;
}

int ModelInfo::maxLength() const
{
    return MySettings::globalInstance()->modelMaxLength(*this);
}

void ModelInfo::setMaxLength(int l)
{
    if (shouldSaveMetadata()) MySettings::globalInstance()->setModelMaxLength(*this, l, true /*force*/);
    m_maxLength = l;
}

int ModelInfo::promptBatchSize() const
{
    return MySettings::globalInstance()->modelPromptBatchSize(*this);
}

void ModelInfo::setPromptBatchSize(int s)
{
    if (shouldSaveMetadata()) MySettings::globalInstance()->setModelPromptBatchSize(*this, s, true /*force*/);
    m_promptBatchSize = s;
}

int ModelInfo::contextLength() const
{
    return MySettings::globalInstance()->modelContextLength(*this);
}

void ModelInfo::setContextLength(int l)
{
    if (shouldSaveMetadata()) MySettings::globalInstance()->setModelContextLength(*this, l, true /*force*/);
    m_contextLength = l;
}

int ModelInfo::maxContextLength() const
{
    if (!installed || isOnline) return -1;
    if (m_maxContextLength != -1) return m_maxContextLength;
    auto path = (dirpath + filename()).toStdString();
    int n_ctx = LLModel::Implementation::maxContextLength(path);
    if (n_ctx < 0) {
        n_ctx = 4096; // fallback value
    }
    m_maxContextLength = n_ctx;
    return m_maxContextLength;
}

int ModelInfo::gpuLayers() const
{
    return MySettings::globalInstance()->modelGpuLayers(*this);
}

void ModelInfo::setGpuLayers(int l)
{
    if (shouldSaveMetadata()) MySettings::globalInstance()->setModelGpuLayers(*this, l, true /*force*/);
    m_gpuLayers = l;
}

int ModelInfo::maxGpuLayers() const
{
    if (!installed || isOnline) return -1;
    if (m_maxGpuLayers != -1) return m_maxGpuLayers;
    auto path = (dirpath + filename()).toStdString();
    int layers = LLModel::Implementation::layerCount(path);
    if (layers < 0) {
        layers = 100; // fallback value
    }
    m_maxGpuLayers = layers;
    return m_maxGpuLayers;
}

double ModelInfo::repeatPenalty() const
{
    return MySettings::globalInstance()->modelRepeatPenalty(*this);
}

void ModelInfo::setRepeatPenalty(double p)
{
    if (shouldSaveMetadata()) MySettings::globalInstance()->setModelRepeatPenalty(*this, p, true /*force*/);
    m_repeatPenalty = p;
}

int ModelInfo::repeatPenaltyTokens() const
{
    return MySettings::globalInstance()->modelRepeatPenaltyTokens(*this);
}

void ModelInfo::setRepeatPenaltyTokens(int t)
{
    if (shouldSaveMetadata()) MySettings::globalInstance()->setModelRepeatPenaltyTokens(*this, t, true /*force*/);
    m_repeatPenaltyTokens = t;
}

QVariant ModelInfo::defaultChatTemplate() const
{
    auto res = m_chatTemplate.or_else([this]() -> std::optional<QString> {
        if (!installed || isOnline)
            return std::nullopt;
        if (!m_modelChatTemplate) {
            auto path = (dirpath + filename()).toUtf8();
            auto res = LLModel::Implementation::chatTemplate(path.constData());
            if (res) {
                std::string ggufTmpl(std::move(*res));
                if (ggufTmpl.size() >= 2 && ggufTmpl.end()[-2] != '\n' && ggufTmpl.end()[-1] == '\n')
                    ggufTmpl.erase(ggufTmpl.end() - 1); // strip trailing newline for e.g. Llama-3.2-3B-Instruct
                if (
                    auto replacement = CHAT_TEMPLATE_SUBSTITUTIONS.find(ggufTmpl);
                    replacement != CHAT_TEMPLATE_SUBSTITUTIONS.end()
                ) {
                    qWarning() << "automatically substituting chat template for" << filename();
                    auto &[badTemplate, goodTemplate] = *replacement;
                    ggufTmpl = goodTemplate;
                }
                m_modelChatTemplate = QString::fromStdString(ggufTmpl);
            } else {
                qWarning().nospace() << "failed to get chat template for " << filename() << ": " << res.error().c_str();
                m_modelChatTemplate = QString(); // do not retry
            }
        }
        if (m_modelChatTemplate->isNull())
            return std::nullopt;
        return m_modelChatTemplate;
    });

    if (res)
        return std::move(*res);
    return QVariant::fromValue(nullptr);
}

auto ModelInfo::chatTemplate() const -> UpgradeableSetting
{
    return MySettings::globalInstance()->modelChatTemplate(*this);
}

QString ModelInfo::defaultSystemMessage() const
{
    return m_systemMessage;
}

auto ModelInfo::systemMessage() const -> UpgradeableSetting
{
    return MySettings::globalInstance()->modelSystemMessage(*this);
}

QString ModelInfo::chatNamePrompt() const
{
    return MySettings::globalInstance()->modelChatNamePrompt(*this);
}

void ModelInfo::setChatNamePrompt(const QString &p)
{
    if (shouldSaveMetadata()) MySettings::globalInstance()->setModelChatNamePrompt(*this, p, true /*force*/);
    m_chatNamePrompt = p;
}

QString ModelInfo::suggestedFollowUpPrompt() const
{
    return MySettings::globalInstance()->modelSuggestedFollowUpPrompt(*this);
}

void ModelInfo::setSuggestedFollowUpPrompt(const QString &p)
{
    if (shouldSaveMetadata()) MySettings::globalInstance()->setModelSuggestedFollowUpPrompt(*this, p, true /*force*/);
    m_suggestedFollowUpPrompt = p;
}

// FIXME(jared): this should not be used for model settings that have meaningful defaults, such as temperature
bool ModelInfo::shouldSaveMetadata() const
{
    return installed && (isClone() || isDiscovered() || description() == "" /*indicates sideloaded*/);
}

QVariant ModelInfo::getField(QLatin1StringView name) const
{
    static const std::unordered_map<QLatin1StringView, QVariant(*)(const ModelInfo &)> s_fields = {
        { "filename"_L1,                [](auto &i) -> QVariant { return i.m_filename;                } },
        { "description"_L1,             [](auto &i) -> QVariant { return i.m_description;             } },
        { "url"_L1,                     [](auto &i) -> QVariant { return i.m_url;                     } },
        { "quant"_L1,                   [](auto &i) -> QVariant { return i.m_quant;                   } },
        { "type"_L1,                    [](auto &i) -> QVariant { return i.m_type;                    } },
        { "isClone"_L1,                 [](auto &i) -> QVariant { return i.m_isClone;                 } },
        { "isDiscovered"_L1,            [](auto &i) -> QVariant { return i.m_isDiscovered;            } },
        { "likes"_L1,                   [](auto &i) -> QVariant { return i.m_likes;                   } },
        { "downloads"_L1,               [](auto &i) -> QVariant { return i.m_downloads;               } },
        { "recency"_L1,                 [](auto &i) -> QVariant { return i.m_recency;                 } },
        { "temperature"_L1,             [](auto &i) -> QVariant { return i.m_temperature;             } },
        { "topP"_L1,                    [](auto &i) -> QVariant { return i.m_topP;                    } },
        { "minP"_L1,                    [](auto &i) -> QVariant { return i.m_minP;                    } },
        { "topK"_L1,                    [](auto &i) -> QVariant { return i.m_topK;                    } },
        { "maxLength"_L1,               [](auto &i) -> QVariant { return i.m_maxLength;               } },
        { "promptBatchSize"_L1,         [](auto &i) -> QVariant { return i.m_promptBatchSize;         } },
        { "contextLength"_L1,           [](auto &i) -> QVariant { return i.m_contextLength;           } },
        { "gpuLayers"_L1,               [](auto &i) -> QVariant { return i.m_gpuLayers;               } },
        { "repeatPenalty"_L1,           [](auto &i) -> QVariant { return i.m_repeatPenalty;           } },
        { "repeatPenaltyTokens"_L1,     [](auto &i) -> QVariant { return i.m_repeatPenaltyTokens;     } },
        { "chatTemplate"_L1,            [](auto &i) -> QVariant { return i.defaultChatTemplate();     } },
        { "systemMessage"_L1,           [](auto &i) -> QVariant { return i.m_systemMessage;           } },
        { "chatNamePrompt"_L1,          [](auto &i) -> QVariant { return i.m_chatNamePrompt;          } },
        { "suggestedFollowUpPrompt"_L1, [](auto &i) -> QVariant { return i.m_suggestedFollowUpPrompt; } },
    };
    return s_fields.at(name)(*this);
}

InstalledModels::InstalledModels(QObject *parent, bool selectable)
    : QSortFilterProxyModel(parent)
    , m_selectable(selectable)
{
    connect(this, &InstalledModels::rowsInserted, this, &InstalledModels::countChanged);
    connect(this, &InstalledModels::rowsRemoved, this, &InstalledModels::countChanged);
    connect(this, &InstalledModels::modelReset, this, &InstalledModels::countChanged);
}

bool InstalledModels::filterAcceptsRow(int sourceRow,
                                       const QModelIndex &sourceParent) const
{
    /* TODO(jared): We should list incomplete models alongside installed models on the
     * Models page. Simply replacing isDownloading with isIncomplete here doesn't work for
     * some reason - the models show up as something else. */
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    bool isInstalled = sourceModel()->data(index, ModelList::InstalledRole).toBool();
    bool isDownloading = sourceModel()->data(index, ModelList::DownloadingRole).toBool();
    bool isEmbeddingModel = sourceModel()->data(index, ModelList::IsEmbeddingModelRole).toBool();
    // list installed chat models
    return (isInstalled || (!m_selectable && isDownloading)) && !isEmbeddingModel;
}

GPT4AllDownloadableModels::GPT4AllDownloadableModels(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    connect(this, &GPT4AllDownloadableModels::rowsInserted, this, &GPT4AllDownloadableModels::countChanged);
    connect(this, &GPT4AllDownloadableModels::rowsRemoved, this, &GPT4AllDownloadableModels::countChanged);
    connect(this, &GPT4AllDownloadableModels::modelReset, this, &GPT4AllDownloadableModels::countChanged);
}

void GPT4AllDownloadableModels::filter(const QVector<QString> &keywords)
{
    m_keywords = keywords;
    invalidateFilter();
}

bool GPT4AllDownloadableModels::filterAcceptsRow(int sourceRow,
                                          const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    const QString description = sourceModel()->data(index, ModelList::DescriptionRole).toString();
    bool hasDescription = !description.isEmpty();
    bool isClone = sourceModel()->data(index, ModelList::IsCloneRole).toBool();
    bool isDiscovered = sourceModel()->data(index, ModelList::IsDiscoveredRole).toBool();
    bool satisfiesKeyword = m_keywords.isEmpty();
    for (const QString &k : m_keywords)
        satisfiesKeyword = description.contains(k) ? true : satisfiesKeyword;
    return !isDiscovered && hasDescription && !isClone && satisfiesKeyword;
}

int GPT4AllDownloadableModels::count() const
{
    return rowCount();
}

HuggingFaceDownloadableModels::HuggingFaceDownloadableModels(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_limit(5)
{
    connect(this, &HuggingFaceDownloadableModels::rowsInserted, this, &HuggingFaceDownloadableModels::countChanged);
    connect(this, &HuggingFaceDownloadableModels::rowsRemoved, this, &HuggingFaceDownloadableModels::countChanged);
    connect(this, &HuggingFaceDownloadableModels::modelReset, this, &HuggingFaceDownloadableModels::countChanged);
}

bool HuggingFaceDownloadableModels::filterAcceptsRow(int sourceRow,
                                          const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    bool hasDescription = !sourceModel()->data(index, ModelList::DescriptionRole).toString().isEmpty();
    bool isClone = sourceModel()->data(index, ModelList::IsCloneRole).toBool();
    bool isDiscovered = sourceModel()->data(index, ModelList::IsDiscoveredRole).toBool();
    return isDiscovered && hasDescription && !isClone;
}

int HuggingFaceDownloadableModels::count() const
{
    return rowCount();
}

void HuggingFaceDownloadableModels::discoverAndFilter(const QString &discover)
{
    m_discoverFilter = discover;
    ModelList *ml = qobject_cast<ModelList*>(parent());
    ml->discoverSearch(discover);
}

class MyModelList: public ModelList { };
Q_GLOBAL_STATIC(MyModelList, modelListInstance)
ModelList *ModelList::globalInstance()
{
    return modelListInstance();
}

ModelList::ModelList()
    : QAbstractListModel(nullptr)
    , m_installedModels(new InstalledModels(this))
    , m_selectableModels(new InstalledModels(this, /*selectable*/ true))
    , m_gpt4AllDownloadableModels(new GPT4AllDownloadableModels(this))
    , m_huggingFaceDownloadableModels(new HuggingFaceDownloadableModels(this))
    , m_asyncModelRequestOngoing(false)
    , m_discoverLimit(20)
    , m_discoverSortDirection(-1)
    , m_discoverSort(Likes)
    , m_discoverNumberOfResults(0)
    , m_discoverResultsCompleted(0)
    , m_discoverInProgress(false)
{
    QCoreApplication::instance()->installEventFilter(this);

    m_installedModels->setSourceModel(this);
    m_selectableModels->setSourceModel(this);
    m_gpt4AllDownloadableModels->setSourceModel(this);
    m_huggingFaceDownloadableModels->setSourceModel(this);

    auto *mySettings = MySettings::globalInstance();
    connect(mySettings, &MySettings::nameChanged,                this, &ModelList::updateDataForSettings     );
    connect(mySettings, &MySettings::temperatureChanged,         this, &ModelList::updateDataForSettings     );
    connect(mySettings, &MySettings::topPChanged,                this, &ModelList::updateDataForSettings     );
    connect(mySettings, &MySettings::minPChanged,                this, &ModelList::updateDataForSettings     );
    connect(mySettings, &MySettings::topKChanged,                this, &ModelList::updateDataForSettings     );
    connect(mySettings, &MySettings::maxLengthChanged,           this, &ModelList::updateDataForSettings     );
    connect(mySettings, &MySettings::promptBatchSizeChanged,     this, &ModelList::updateDataForSettings     );
    connect(mySettings, &MySettings::contextLengthChanged,       this, &ModelList::updateDataForSettings     );
    connect(mySettings, &MySettings::gpuLayersChanged,           this, &ModelList::updateDataForSettings     );
    connect(mySettings, &MySettings::repeatPenaltyChanged,       this, &ModelList::updateDataForSettings     );
    connect(mySettings, &MySettings::repeatPenaltyTokensChanged, this, &ModelList::updateDataForSettings     );
    connect(mySettings, &MySettings::chatTemplateChanged,        this, &ModelList::maybeUpdateDataForSettings);
    connect(mySettings, &MySettings::systemMessageChanged,       this, &ModelList::maybeUpdateDataForSettings);

    connect(this, &ModelList::dataChanged, this, &ModelList::onDataChanged);

    connect(&m_networkManager, &QNetworkAccessManager::sslErrors, this, &ModelList::handleSslErrors);

    updateModelsFromJson();
    updateModelsFromSettings();
    updateModelsFromDirectory();

    connect(mySettings, &MySettings::modelPathChanged, this, &ModelList::updateModelsFromDirectory);
    connect(mySettings, &MySettings::modelPathChanged, this, &ModelList::updateModelsFromJson     );
    connect(mySettings, &MySettings::modelPathChanged, this, &ModelList::updateModelsFromSettings );

    QCoreApplication::instance()->installEventFilter(this);
}

// an easier way to listen for model info and setting changes
void ModelList::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles)
{
    Q_UNUSED(roles)
    for (int row = topLeft.row(); row <= bottomRight.row(); row++) {
        auto index = topLeft.siblingAtRow(row);
        auto id = index.data(ModelList::IdRole).toString();
        if (auto info = modelInfo(id); !info.id().isNull())
            emit modelInfoChanged(info);
    }
}

QString ModelList::compatibleModelNameHash(QUrl baseUrl, QString modelName) {
    QCryptographicHash sha256(QCryptographicHash::Sha256);
    sha256.addData((baseUrl.toString() + "_" + modelName).toUtf8());
    return sha256.result().toHex();
}

QString ModelList::compatibleModelFilename(QUrl baseUrl, QString modelName) {
    QString hash(compatibleModelNameHash(baseUrl, modelName));
    return QString(u"gpt4all-%1-capi.rmodel"_s).arg(hash);
}

bool ModelList::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == QCoreApplication::instance() && ev->type() == QEvent::LanguageChange)
        emit dataChanged(index(0, 0), index(m_models.size() - 1, 0));
    return false;
}

QString ModelList::incompleteDownloadPath(const QString &modelFile)
{
    return MySettings::globalInstance()->modelPath() + "incomplete-" + modelFile;
}

const QList<ModelInfo> ModelList::selectableModelList() const
{
    // FIXME: This needs to be kept in sync with m_selectableModels so should probably be merged
    QMutexLocker locker(&m_mutex);
    QList<ModelInfo> infos;
    for (ModelInfo *info : m_models)
        if (info->installed && !info->isEmbeddingModel)
            infos.append(*info);
    return infos;
}

ModelInfo ModelList::defaultModelInfo() const
{
    QMutexLocker locker(&m_mutex);

    QSettings settings;

    // The user default model can be set by the user in the settings dialog. The "default" user
    // default model is "Application default" which signals we should use the logic here.
    const QString userDefaultModelName = MySettings::globalInstance()->userDefaultModel();
    const bool hasUserDefaultName = !userDefaultModelName.isEmpty() && userDefaultModelName != "Application default";

    ModelInfo *defaultModel = nullptr;
    for (ModelInfo *info : m_models) {
        if (!info->installed)
            continue;
        defaultModel = info;

        const size_t ramrequired = defaultModel->ramrequired;

        // If we don't have either setting, then just use the first model that requires less than 16GB that is installed
        if (!hasUserDefaultName && !info->isOnline && ramrequired > 0 && ramrequired < 16)
            break;

        // If we have a user specified default and match, then use it
        if (hasUserDefaultName && (defaultModel->id() == userDefaultModelName))
            break;
    }
    if (defaultModel)
        return *defaultModel;
    return ModelInfo();
}

bool ModelList::contains(const QString &id) const
{
    QMutexLocker locker(&m_mutex);
    return m_modelMap.contains(id);
}

bool ModelList::containsByFilename(const QString &filename) const
{
    QMutexLocker locker(&m_mutex);
    for (ModelInfo *info : m_models)
        if (info->filename() == filename)
            return true;
    return false;
}

bool ModelList::lessThan(const ModelInfo* a, const ModelInfo* b, DiscoverSort s, int d)
{
    // Rule -1a: Discover sort
    if (a->isDiscovered() && b->isDiscovered()) {
        switch (s) {
        case Default: break;
        case Likes: return (d > 0 ? a->likes() < b->likes() : a->likes() > b->likes());
        case Downloads: return (d > 0 ? a->downloads() < b->downloads() : a->downloads() > b->downloads());
        case Recent: return (d > 0 ? a->recency() < b->recency() : a->recency() > b->recency());
        }
    }

    // Rule -1: Discovered before non-discovered
    if (a->isDiscovered() != b->isDiscovered()) {
        return a->isDiscovered();
    }

    // Rule 0: Non-clone before clone
    if (a->isClone() != b->isClone()) {
        return !a->isClone();
    }

    // Rule 1: Non-empty 'order' before empty
    if (a->order.isEmpty() != b->order.isEmpty()) {
        return !a->order.isEmpty();
    }

    // Rule 2: Both 'order' are non-empty, sort alphanumerically
    if (!a->order.isEmpty() && !b->order.isEmpty()) {
        return a->order < b->order;
    }

    // Rule 3: Both 'order' are empty, sort by id
    return a->id() < b->id();
}

void ModelList::addModel(const QString &id)
{
    const bool hasModel = contains(id);
    Q_ASSERT(!hasModel);
    if (hasModel) {
        qWarning() << "ERROR: model list already contains" << id;
        return;
    }

    ModelInfo *info = new ModelInfo;
    info->setId(id);

    m_mutex.lock();
    auto s = m_discoverSort;
    auto d = m_discoverSortDirection;
    const auto insertPosition = std::lower_bound(m_models.begin(), m_models.end(), info,
        [s, d](const ModelInfo* lhs, const ModelInfo* rhs) {
            return ModelList::lessThan(lhs, rhs, s, d);
        });
    const int index = std::distance(m_models.begin(), insertPosition);
    m_mutex.unlock();

    // NOTE: The begin/end rows cannot have a lock placed around them. We calculate the index ahead
    // of time and this works because this class is designed carefully so that only one thread is
    // responsible for insertion, deletion, and update

    beginInsertRows(QModelIndex(), index, index);
    m_mutex.lock();
    m_models.insert(insertPosition, info);
    m_modelMap.insert(id, info);
    m_mutex.unlock();
    endInsertRows();

    emit selectableModelListChanged();
}

void ModelList::changeId(const QString &oldId, const QString &newId)
{
    const bool hasModel = contains(oldId);
    Q_ASSERT(hasModel);
    if (!hasModel) {
        qWarning() << "ERROR: model list does not contain" << oldId;
        return;
    }

    QMutexLocker locker(&m_mutex);
    ModelInfo *info = m_modelMap.take(oldId);
    info->setId(newId);
    m_modelMap.insert(newId, info);
}

int ModelList::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    QMutexLocker locker(&m_mutex);
    return m_models.size();
}

QVariant ModelList::dataInternal(const ModelInfo *info, int role) const
{
    switch (role) {
        case IdRole:
            return info->id();
        case NameRole:
            return info->name();
        case FilenameRole:
            return info->filename();
        case DirpathRole:
            return info->dirpath;
        case FilesizeRole:
            return info->filesize;
        case HashRole:
            return info->hash;
        case HashAlgorithmRole:
            return info->hashAlgorithm;
        case CalcHashRole:
            return info->calcHash;
        case InstalledRole:
            return info->installed;
        case DefaultRole:
            return info->isDefault;
        case OnlineRole:
            return info->isOnline;
        case CompatibleApiRole:
            return info->isCompatibleApi;
        case DescriptionRole:
            return info->description();
        case RequiresVersionRole:
            return info->requiresVersion;
        case VersionRemovedRole:
            return info->versionRemoved;
        case UrlRole:
            return info->url();
        case BytesReceivedRole:
            return info->bytesReceived;
        case BytesTotalRole:
            return info->bytesTotal;
        case TimestampRole:
            return info->timestamp;
        case SpeedRole:
            return info->speed;
        case DownloadingRole:
            return info->isDownloading;
        case IncompleteRole:
            return info->isIncomplete;
        case DownloadErrorRole:
            return info->downloadError;
        case OrderRole:
            return info->order;
        case RamrequiredRole:
            return info->ramrequired;
        case ParametersRole:
            return info->parameters;
        case QuantRole:
            return info->quant();
        case TypeRole:
            return info->type();
        case IsCloneRole:
            return info->isClone();
        case IsDiscoveredRole:
            return info->isDiscovered();
        case IsEmbeddingModelRole:
            return info->isEmbeddingModel;
        case TemperatureRole:
            return info->temperature();
        case TopPRole:
            return info->topP();
        case MinPRole:
            return info->minP();
        case TopKRole:
            return info->topK();
        case MaxLengthRole:
            return info->maxLength();
        case PromptBatchSizeRole:
            return info->promptBatchSize();
        case ContextLengthRole:
            return info->contextLength();
        case GpuLayersRole:
            return info->gpuLayers();
        case RepeatPenaltyRole:
            return info->repeatPenalty();
        case RepeatPenaltyTokensRole:
            return info->repeatPenaltyTokens();
        case ChatTemplateRole:
            return QVariant::fromValue(info->chatTemplate());
        case SystemMessageRole:
            return QVariant::fromValue(info->systemMessage());
        case ChatNamePromptRole:
            return info->chatNamePrompt();
        case SuggestedFollowUpPromptRole:
            return info->suggestedFollowUpPrompt();
        case LikesRole:
            return info->likes();
        case DownloadsRole:
            return info->downloads();
        case RecencyRole:
            return info->recency();

    }

    return QVariant();
}

QVariant ModelList::data(const QString &id, int role) const
{
    QMutexLocker locker(&m_mutex);
    ModelInfo *info = m_modelMap.value(id);
    return dataInternal(info, role);
}

QVariant ModelList::dataByFilename(const QString &filename, int role) const
{
    QMutexLocker locker(&m_mutex);
    for (ModelInfo *info : m_models)
        if (info->filename() == filename)
            return dataInternal(info, role);
    return QVariant();
}

QVariant ModelList::data(const QModelIndex &index, int role) const
{
    QMutexLocker locker(&m_mutex);
    if (!index.isValid() || index.row() < 0 || index.row() >= m_models.size())
        return QVariant();
    const ModelInfo *info = m_models.at(index.row());
    return dataInternal(info, role);
}

void ModelList::updateData(const QString &id, const QVector<QPair<int, QVariant>> &data)
{
    // We only sort when one of the fields used by the sorting algorithm actually changes that
    // is implicated or used by the sorting algorithm
    bool shouldSort = false;
    int index;

    {
        QMutexLocker locker(&m_mutex);
        if (!m_modelMap.contains(id)) {
            qWarning() << "ERROR: cannot update as model map does not contain" << id;
            return;
        }

        ModelInfo *info = m_modelMap.value(id);
        index = m_models.indexOf(info);
        if (index == -1) {
            qWarning() << "ERROR: cannot update as model list does not contain" << id;
            return;
        }

        for (const auto &d : data) {
            const int role = d.first;
            const QVariant value = d.second;
            switch (role) {
            case IdRole:
                {
                    if (info->id() != value.toString()) {
                        info->setId(value.toString());
                        shouldSort = true;
                    }
                    break;
                }
            case NameRole:
                info->setName(value.toString()); break;
            case FilenameRole:
                info->setFilename(value.toString()); break;
            case DirpathRole:
                info->dirpath = value.toString(); break;
            case FilesizeRole:
                info->filesize = value.toString(); break;
            case HashRole:
                info->hash = value.toByteArray(); break;
            case HashAlgorithmRole:
                info->hashAlgorithm = static_cast<ModelInfo::HashAlgorithm>(value.toInt()); break;
            case CalcHashRole:
                info->calcHash = value.toBool(); break;
            case InstalledRole:
                info->installed = value.toBool(); break;
            case DefaultRole:
                info->isDefault = value.toBool(); break;
            case OnlineRole:
                info->isOnline = value.toBool(); break;
            case CompatibleApiRole:
                info->isCompatibleApi = value.toBool(); break;
            case DescriptionRole:
                info->setDescription(value.toString()); break;
            case RequiresVersionRole:
                info->requiresVersion = value.toString(); break;
            case VersionRemovedRole:
                info->versionRemoved = value.toString(); break;
            case UrlRole:
                info->setUrl(value.toString()); break;
            case BytesReceivedRole:
                info->bytesReceived = value.toLongLong(); break;
            case BytesTotalRole:
                info->bytesTotal = value.toLongLong(); break;
            case TimestampRole:
                info->timestamp = value.toLongLong(); break;
            case SpeedRole:
                info->speed = value.toString(); break;
            case DownloadingRole:
                info->isDownloading = value.toBool(); break;
            case IncompleteRole:
                info->isIncomplete = value.toBool(); break;
            case DownloadErrorRole:
                info->downloadError = value.toString(); break;
            case OrderRole:
                {
                    if (info->order != value.toString()) {
                        info->order = value.toString();
                        shouldSort = true;
                    }
                    break;
                }
            case RamrequiredRole:
                info->ramrequired = value.toInt(); break;
            case ParametersRole:
                info->parameters = value.toString(); break;
            case QuantRole:
                info->setQuant(value.toString()); break;
            case TypeRole:
                info->setType(value.toString()); break;
            case IsCloneRole:
                {
                    if (info->isClone() != value.toBool()) {
                        info->setIsClone(value.toBool());
                        shouldSort = true;
                    }
                    break;
                }
            case IsDiscoveredRole:
                {
                    if (info->isDiscovered() != value.toBool()) {
                        info->setIsDiscovered(value.toBool());
                        shouldSort = true;
                    }
                    break;
                }
            case IsEmbeddingModelRole:
                info->isEmbeddingModel = value.toBool(); break;
            case TemperatureRole:
                info->setTemperature(value.toDouble()); break;
            case TopPRole:
                info->setTopP(value.toDouble()); break;
            case MinPRole:
                info->setMinP(value.toDouble()); break;
            case TopKRole:
                info->setTopK(value.toInt()); break;
            case MaxLengthRole:
                info->setMaxLength(value.toInt()); break;
            case PromptBatchSizeRole:
                info->setPromptBatchSize(value.toInt()); break;
            case ContextLengthRole:
                info->setContextLength(value.toInt()); break;
            case GpuLayersRole:
                info->setGpuLayers(value.toInt()); break;
            case RepeatPenaltyRole:
                info->setRepeatPenalty(value.toDouble()); break;
            case RepeatPenaltyTokensRole:
                info->setRepeatPenaltyTokens(value.toInt()); break;
            case ChatTemplateRole:
                info->m_chatTemplate = value.toString(); break;
            case SystemMessageRole:
                info->m_systemMessage = value.toString(); break;
            case ChatNamePromptRole:
                info->setChatNamePrompt(value.toString()); break;
            case SuggestedFollowUpPromptRole:
                info->setSuggestedFollowUpPrompt(value.toString()); break;
            case LikesRole:
                {
                    if (info->likes() != value.toInt()) {
                        info->setLikes(value.toInt());
                        shouldSort = true;
                    }
                    break;
                }
            case DownloadsRole:
                {
                    if (info->downloads() != value.toInt()) {
                        info->setDownloads(value.toInt());
                        shouldSort = true;
                    }
                    break;
                }
            case RecencyRole:
                {
                    if (info->recency() != value.toDateTime()) {
                        info->setRecency(value.toDateTime());
                        shouldSort = true;
                    }
                    break;
                }
            }
        }

        // Extra guarantee that these always remains in sync with filesystem
        QString modelPath = info->dirpath + info->filename();
        const QFileInfo fileInfo(modelPath);
        info->installed = fileInfo.exists();
        const QFileInfo incompleteInfo(incompleteDownloadPath(info->filename()));
        info->isIncomplete = incompleteInfo.exists();

        // check installed, discovered/sideloaded models only (including clones)
        if (!info->checkedEmbeddingModel && !info->isEmbeddingModel && info->installed
            && (info->isDiscovered() || info->description().isEmpty()))
        {
            // read GGUF and decide based on model architecture
            info->isEmbeddingModel = LLModel::Implementation::isEmbeddingModel(modelPath.toStdString());
            info->checkedEmbeddingModel = true;
        }
    }

    emit dataChanged(createIndex(index, 0), createIndex(index, 0));

    if (shouldSort)
        resortModel();

    emit selectableModelListChanged();
}

void ModelList::resortModel()
{
    emit layoutAboutToBeChanged();
    {
        QMutexLocker locker(&m_mutex);
        auto s = m_discoverSort;
        auto d = m_discoverSortDirection;
        std::stable_sort(m_models.begin(), m_models.end(), [s, d](const ModelInfo* lhs, const ModelInfo* rhs) {
            return ModelList::lessThan(lhs, rhs, s, d);
        });
    }
    emit layoutChanged();
}

void ModelList::updateDataByFilename(const QString &filename, QVector<QPair<int, QVariant>> data)
{
    if (data.isEmpty())
        return; // no-op

    QVector<QString> modelsById;
    {
        QMutexLocker locker(&m_mutex);
        for (ModelInfo *info : m_models)
            if (info->filename() == filename)
                modelsById.append(info->id());
    }

    if (modelsById.isEmpty()) {
        qWarning() << "ERROR: cannot update model as list does not contain file" << filename;
        return;
    }

    for (const QString &id : modelsById)
        updateData(id, data);
}

ModelInfo ModelList::modelInfo(const QString &id) const
{
    QMutexLocker locker(&m_mutex);
    if (!m_modelMap.contains(id))
        return ModelInfo();
    return *m_modelMap.value(id);
}

ModelInfo ModelList::modelInfoByFilename(const QString &filename, bool allowClone) const
{
    QMutexLocker locker(&m_mutex);
    for (ModelInfo *info : m_models)
        if (info->filename() == filename && (allowClone || !info->isClone()))
            return *info;
    return ModelInfo();
}

bool ModelList::isUniqueName(const QString &name) const
{
    QMutexLocker locker(&m_mutex);
    for (const ModelInfo *info : m_models) {
        if(info->name() == name)
            return false;
    }
    return true;
}

QString ModelList::clone(const ModelInfo &model)
{
    auto *mySettings = MySettings::globalInstance();

    const QString id = Network::globalInstance()->generateUniqueId();
    addModel(id);

    QString tmplSetting, sysmsgSetting;
    if (auto tmpl = model.chatTemplate().asModern()) {
        tmplSetting = *tmpl;
    } else {
        qWarning("ModelList Warning: attempted to clone model with legacy chat template");
        return {};
    }
    if (auto msg = model.systemMessage().asModern()) {
        sysmsgSetting = *msg;
    } else {
        qWarning("ModelList Warning: attempted to clone model with legacy system message");
        return {};
    }

    QVector<QPair<int, QVariant>> data {
        { ModelList::InstalledRole, model.installed },
        { ModelList::IsCloneRole, true },
        { ModelList::NameRole, uniqueModelName(model) },
        { ModelList::FilenameRole, model.filename() },
        { ModelList::DirpathRole, model.dirpath },
        { ModelList::OnlineRole, model.isOnline },
        { ModelList::CompatibleApiRole, model.isCompatibleApi },
        { ModelList::IsEmbeddingModelRole, model.isEmbeddingModel },
        { ModelList::TemperatureRole, model.temperature() },
        { ModelList::TopPRole, model.topP() },
        { ModelList::MinPRole, model.minP() },
        { ModelList::TopKRole, model.topK() },
        { ModelList::MaxLengthRole, model.maxLength() },
        { ModelList::PromptBatchSizeRole, model.promptBatchSize() },
        { ModelList::ContextLengthRole, model.contextLength() },
        { ModelList::GpuLayersRole, model.gpuLayers() },
        { ModelList::RepeatPenaltyRole, model.repeatPenalty() },
        { ModelList::RepeatPenaltyTokensRole, model.repeatPenaltyTokens() },
        { ModelList::SystemMessageRole, model.m_systemMessage },
        { ModelList::ChatNamePromptRole, model.chatNamePrompt() },
        { ModelList::SuggestedFollowUpPromptRole, model.suggestedFollowUpPrompt() },
    };
    if (auto tmpl = model.m_chatTemplate)
        data.emplace_back(ModelList::ChatTemplateRole, *tmpl); // copy default chat template, if known
    updateData(id, data);

    // Ensure setting overrides are copied in case the base model overrides change.
    // This is necessary because setting these roles on ModelInfo above does not write to settings.
    auto cloneInfo = modelInfo(id);
    if (mySettings->isModelChatTemplateSet (model))
        mySettings->setModelChatTemplate (cloneInfo, tmplSetting  );
    if (mySettings->isModelSystemMessageSet(model))
        mySettings->setModelSystemMessage(cloneInfo, sysmsgSetting);

    return id;
}

void ModelList::removeClone(const ModelInfo &model)
{
    Q_ASSERT(model.isClone());
    if (!model.isClone())
        return;

    removeInternal(model);
}

void ModelList::removeInstalled(const ModelInfo &model)
{
    Q_ASSERT(model.installed);
    Q_ASSERT(!model.isClone());
    Q_ASSERT(model.isDiscovered() || model.isCompatibleApi || model.description() == "" /*indicates sideloaded*/);
    removeInternal(model);
}

int ModelList::indexByModelId(const QString &id) const
{
    QMutexLocker locker(&m_mutex);
    if (auto it = m_modelMap.find(id); it != m_modelMap.cend())
        return m_models.indexOf(*it);
    return -1;
}

void ModelList::removeInternal(const ModelInfo &model)
{
    int indexOfModel = indexByModelId(model.id());
    Q_ASSERT(indexOfModel != -1);
    if (indexOfModel == -1) {
        qWarning() << "ERROR: model list does not contain" << model.id();
        return;
    }

    beginRemoveRows(QModelIndex(), indexOfModel, indexOfModel);
    {
        QMutexLocker locker(&m_mutex);
        ModelInfo *info = m_models.takeAt(indexOfModel);
        m_modelMap.remove(info->id());
        delete info;
    }
    endRemoveRows();
    emit selectableModelListChanged();
    MySettings::globalInstance()->eraseModel(model);
}

QString ModelList::uniqueModelName(const ModelInfo &model) const
{
    QMutexLocker locker(&m_mutex);
    static const QRegularExpression re("^(.*)~(\\d+)$");
    QRegularExpressionMatch match = re.match(model.name());
    QString baseName;
    if (match.hasMatch())
        baseName = match.captured(1);
    else
        baseName = model.name();

    int maxSuffixNumber = 0;
    bool baseNameExists = false;

    for (const ModelInfo *info : m_models) {
        if(info->name() == baseName)
            baseNameExists = true;

        QRegularExpressionMatch match = re.match(info->name());
        if (match.hasMatch()) {
            QString currentBaseName = match.captured(1);
            int currentSuffixNumber = match.captured(2).toInt();
            if (currentBaseName == baseName && currentSuffixNumber > maxSuffixNumber)
                maxSuffixNumber = currentSuffixNumber;
        }
    }

    if (baseNameExists)
        return baseName + "~" + QString::number(maxSuffixNumber + 1);

    return baseName;
}

bool ModelList::modelExists(const QString &modelFilename) const
{
    QString appPath = QCoreApplication::applicationDirPath() + modelFilename;
    QFileInfo infoAppPath(appPath);
    if (infoAppPath.exists())
        return true;

    QString downloadPath = MySettings::globalInstance()->modelPath() + modelFilename;
    QFileInfo infoLocalPath(downloadPath);
    if (infoLocalPath.exists())
        return true;
    return false;
}

void ModelList::updateOldRemoteModels(const QString &path)
{
    QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QFileInfo info = it.nextFileInfo();
        QString filename = it.fileName();
        if (!filename.startsWith("chatgpt-") || !filename.endsWith(".txt"))
            continue;

        QString apikey;
        QString modelname(filename);
        modelname.chop(4); // strip ".txt" extension
        modelname.remove(0, 8); // strip "chatgpt-" prefix
        QFile file(info.filePath());
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning().noquote() << tr("cannot open \"%1\": %2").arg(file.fileName(), file.errorString());
            continue;
        }

        {
            QTextStream in(&file);
            apikey = in.readAll();
            file.close();
        }

        QFile newfile(u"%1/gpt4all-%2.rmodel"_s.arg(info.dir().path(), modelname));
        if (!newfile.open(QIODevice::ReadWrite)) {
            qWarning().noquote() << tr("cannot create \"%1\": %2").arg(newfile.fileName(), file.errorString());
            continue;
        }

        QJsonObject obj {
            { "apiKey",    apikey    },
            { "modelName", modelname },
        };

        QTextStream out(&newfile);
        out << QJsonDocument(obj).toJson();
        newfile.close();

        file.remove();
    }
}

void ModelList::processModelDirectory(const QString &path)
{
    QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QFileInfo info = it.nextFileInfo();

        QString filename = it.fileName();
        if (filename.startsWith("incomplete") || FILENAME_BLACKLIST.contains(filename))
            continue;
        if (!filename.endsWith(".gguf") && !filename.endsWith(".rmodel"))
            continue;

        bool isOnline(filename.endsWith(".rmodel"));
        bool isCompatibleApi(filename.endsWith("-capi.rmodel"));

        QString name;
        QString description;
        if (isCompatibleApi) {
            QJsonObject obj;
            {
                QFile file(info.filePath());
                if (!file.open(QIODeviceBase::ReadOnly)) {
                    qWarning().noquote() << tr("cannot open \"%1\": %2").arg(file.fileName(), file.errorString());
                    continue;
                }
                QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                obj = doc.object();
            }
            {
                QString apiKey(obj["apiKey"].toString());
                QString baseUrl(obj["baseUrl"].toString());
                QString modelName(obj["modelName"].toString());
                apiKey = apiKey.length() < 10 ? "*****" : apiKey.left(5) + "*****";
                name = tr("%1 (%2)").arg(modelName, baseUrl);
                description = tr("<strong>OpenAI-Compatible API Model</strong><br>"
                                 "<ul><li>API Key: %1</li>"
                                 "<li>Base URL: %2</li>"
                                 "<li>Model Name: %3</li></ul>")
                                    .arg(apiKey, baseUrl, modelName);
            }
        }

        QVector<QString> modelsById;
        {
            QMutexLocker locker(&m_mutex);
            for (ModelInfo *info : m_models)
                if (info->filename() == filename)
                    modelsById.append(info->id());
        }

        if (modelsById.isEmpty()) {
            if (!contains(filename))
                addModel(filename);
            modelsById.append(filename);
        }

        for (const QString &id : modelsById) {
            QVector<QPair<int, QVariant>> data {
                { InstalledRole, true },
                { FilenameRole, filename },
                { OnlineRole, isOnline },
                { CompatibleApiRole, isCompatibleApi },
                { DirpathRole, info.dir().absolutePath() + "/" },
                { FilesizeRole, toFileSize(info.size()) },
            };
            if (isCompatibleApi) {
                // The data will be saved to "GPT4All.ini".
                data.append({ NameRole, name });
                // The description is hard-coded into "GPT4All.ini" due to performance issue.
                // If the description goes to be dynamic from its .rmodel file, it will get high I/O usage while using the ModelList.
                data.append({ DescriptionRole, description });
                data.append({ ChatTemplateRole, RMODEL_CHAT_TEMPLATE });
            }
            updateData(id, data);
        }
    }
}

void ModelList::updateModelsFromDirectory()
{
    const QString exePath = QCoreApplication::applicationDirPath() + QDir::separator();
    const QString localPath = MySettings::globalInstance()->modelPath();

    updateOldRemoteModels(exePath);
    processModelDirectory(exePath);
    if (localPath != exePath) {
        updateOldRemoteModels(localPath);
        processModelDirectory(localPath);
    }
}

static QString modelsJsonFilename()
{
    return QStringLiteral("models" MODELS_JSON_VERSION ".json");
}

static std::optional<QFile> modelsJsonCacheFile()
{
    constexpr auto loc = QStandardPaths::CacheLocation;
    QString modelsJsonFname = modelsJsonFilename();
    if (auto path = QStandardPaths::locate(loc, modelsJsonFname); !path.isEmpty())
        return std::make_optional<QFile>(path);
    if (auto path = QStandardPaths::writableLocation(loc); !path.isEmpty())
        return std::make_optional<QFile>(u"%1/%2"_s.arg(path, modelsJsonFname));
    return std::nullopt;
}

void ModelList::updateModelsFromJson()
{
    QString modelsJsonFname = modelsJsonFilename();

#if defined(USE_LOCAL_MODELSJSON)
    QUrl jsonUrl(u"file://%1/dev/large_language_models/gpt4all/gpt4all-chat/metadata/%2"_s.arg(QDir::homePath(), modelsJsonFname));
#else
    QUrl jsonUrl(u"http://gpt4all.io/models/%1"_s.arg(modelsJsonFname));
#endif

    QNetworkRequest request(jsonUrl);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);
    QNetworkReply *jsonReply = m_networkManager.get(request);
    connect(qGuiApp, &QCoreApplication::aboutToQuit, jsonReply, &QNetworkReply::abort);
    QEventLoop loop;
    connect(jsonReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(1500, &loop, &QEventLoop::quit);
    loop.exec();
    if (jsonReply->error() == QNetworkReply::NoError && jsonReply->isFinished()) {
        QByteArray jsonData = jsonReply->readAll();
        jsonReply->deleteLater();
        parseModelsJsonFile(jsonData, true);
    } else {
        qWarning() << "WARNING: Could not download models.json synchronously";
        updateModelsFromJsonAsync();

        auto cacheFile = modelsJsonCacheFile();
        if (!cacheFile) {
            // no known location
        } else if (cacheFile->open(QIODeviceBase::ReadOnly)) {
            QByteArray jsonData = cacheFile->readAll();
            cacheFile->close();
            parseModelsJsonFile(jsonData, false);
        } else if (cacheFile->exists())
            qWarning() << "ERROR: Couldn't read models.json cache file: " << cacheFile->fileName();
    }
    delete jsonReply;
}

void ModelList::updateModelsFromJsonAsync()
{
    m_asyncModelRequestOngoing = true;
    emit asyncModelRequestOngoingChanged();
    QString modelsJsonFname = modelsJsonFilename();

#if defined(USE_LOCAL_MODELSJSON)
    QUrl jsonUrl(u"file://%1/dev/large_language_models/gpt4all/gpt4all-chat/metadata/%2"_s.arg(QDir::homePath(), modelsJsonFname));
#else
    QUrl jsonUrl(u"http://gpt4all.io/models/%1"_s.arg(modelsJsonFname));
#endif

    QNetworkRequest request(jsonUrl);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);
    QNetworkReply *jsonReply = m_networkManager.get(request);
    connect(qGuiApp, &QCoreApplication::aboutToQuit, jsonReply, &QNetworkReply::abort);
    connect(jsonReply, &QNetworkReply::finished, this, &ModelList::handleModelsJsonDownloadFinished);
    connect(jsonReply, &QNetworkReply::errorOccurred, this, &ModelList::handleModelsJsonDownloadErrorOccurred);
}

void ModelList::handleModelsJsonDownloadFinished()
{
    QNetworkReply *jsonReply = qobject_cast<QNetworkReply *>(sender());
    if (!jsonReply) {
        m_asyncModelRequestOngoing = false;
        emit asyncModelRequestOngoingChanged();
        return;
    }

    QByteArray jsonData = jsonReply->readAll();
    jsonReply->deleteLater();
    parseModelsJsonFile(jsonData, true);
    m_asyncModelRequestOngoing = false;
    emit asyncModelRequestOngoingChanged();
}

void ModelList::handleModelsJsonDownloadErrorOccurred(QNetworkReply::NetworkError code)
{
    // TODO: Show what error occurred in the GUI
    m_asyncModelRequestOngoing = false;
    emit asyncModelRequestOngoingChanged();

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;

    qWarning() << u"ERROR: Modellist download failed with error code \"%1-%2\""_s
                      .arg(code).arg(reply->errorString());
}

void ModelList::handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    QUrl url = reply->request().url();
    for (const auto &e : errors)
        qWarning() << "ERROR: Received ssl error:" << e.errorString() << "for" << url;
}

void ModelList::maybeUpdateDataForSettings(const ModelInfo &info, bool fromInfo)
{
    // ignore updates that were *because* of a dataChanged - would cause a circular dependency
    int idx;
    if (!fromInfo && (idx = indexByModelId(info.id())) != -1) {
        emit dataChanged(index(idx, 0), index(idx, 0));
        emit selectableModelListChanged();
    }
}

void ModelList::updateDataForSettings()
{
    emit dataChanged(index(0, 0), index(m_models.size() - 1, 0));
    emit selectableModelListChanged();
}

void ModelList::parseModelsJsonFile(const QByteArray &jsonData, bool save)
{
    QJsonParseError err;
    QJsonDocument document = QJsonDocument::fromJson(jsonData, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "ERROR: Couldn't parse: " << jsonData << err.errorString();
        return;
    }

    if (save) {
        auto cacheFile = modelsJsonCacheFile();
        if (!cacheFile) {
            // no known location
        } else if (QFileInfo(*cacheFile).dir().mkpath(u"."_s) && cacheFile->open(QIODeviceBase::WriteOnly)) {
            cacheFile->write(jsonData);
            cacheFile->close();
        } else
            qWarning() << "ERROR: Couldn't write models config file: " << cacheFile->fileName();
    }

    QJsonArray jsonArray = document.array();
    const QString currentVersion = QCoreApplication::applicationVersion();

    for (const QJsonValue &value : jsonArray) {
        QJsonObject obj = value.toObject();

        QString modelName = obj["name"].toString();
        QString modelFilename = obj["filename"].toString();
        QString modelFilesize = obj["filesize"].toString();
        QString requiresVersion = obj["requires"].toString();
        QString versionRemoved = obj["removedIn"].toString();
        QString url = obj["url"].toString();
        QByteArray modelHash = obj["md5sum"].toString().toLatin1();
        bool isDefault = obj.contains("isDefault") && obj["isDefault"] == u"true"_s;
        bool disableGUI = obj.contains("disableGUI") && obj["disableGUI"] == u"true"_s;
        QString description = obj["description"].toString();
        QString order = obj["order"].toString();
        int ramrequired = obj["ramrequired"].toString().toInt();
        QString parameters = obj["parameters"].toString();
        QString quant = obj["quant"].toString();
        QString type = obj["type"].toString();
        bool isEmbeddingModel = obj["embeddingModel"].toBool();

        // Some models aren't supported in the GUI at all
        if (disableGUI)
            continue;

        // If the current version is strictly less than required version, then skip
        if (!requiresVersion.isEmpty() && Download::compareAppVersions(currentVersion, requiresVersion) < 0)
            continue;

        // If the version removed is less than or equal to the current version, then skip
        if (!versionRemoved.isEmpty() && Download::compareAppVersions(versionRemoved, currentVersion) <= 0)
            continue;

        modelFilesize = ModelList::toFileSize(modelFilesize.toULongLong());

        const QString id = modelName;
        Q_ASSERT(!id.isEmpty());

        if (contains(modelFilename))
            changeId(modelFilename, id);

        if (!contains(id))
            addModel(id);

        QVector<QPair<int, QVariant>> data {
            { ModelList::NameRole, modelName },
            { ModelList::FilenameRole, modelFilename },
            { ModelList::FilesizeRole, modelFilesize },
            { ModelList::HashRole, modelHash },
            { ModelList::HashAlgorithmRole, ModelInfo::Md5 },
            { ModelList::DefaultRole, isDefault },
            { ModelList::DescriptionRole, description },
            { ModelList::RequiresVersionRole, requiresVersion },
            { ModelList::VersionRemovedRole, versionRemoved },
            { ModelList::UrlRole, url },
            { ModelList::OrderRole, order },
            { ModelList::RamrequiredRole, ramrequired },
            { ModelList::ParametersRole, parameters },
            { ModelList::QuantRole, quant },
            { ModelList::TypeRole, type },
            { ModelList::IsEmbeddingModelRole, isEmbeddingModel },
        };
        if (obj.contains("temperature"))
            data.append({ ModelList::TemperatureRole, obj["temperature"].toDouble() });
        if (obj.contains("topP"))
            data.append({ ModelList::TopPRole, obj["topP"].toDouble() });
        if (obj.contains("minP"))
            data.append({ ModelList::MinPRole, obj["minP"].toDouble() });
        if (obj.contains("topK"))
            data.append({ ModelList::TopKRole, obj["topK"].toInt() });
        if (obj.contains("maxLength"))
            data.append({ ModelList::MaxLengthRole, obj["maxLength"].toInt() });
        if (obj.contains("promptBatchSize"))
            data.append({ ModelList::PromptBatchSizeRole, obj["promptBatchSize"].toInt() });
        if (obj.contains("contextLength"))
            data.append({ ModelList::ContextLengthRole, obj["contextLength"].toInt() });
        if (obj.contains("gpuLayers"))
            data.append({ ModelList::GpuLayersRole, obj["gpuLayers"].toInt() });
        if (obj.contains("repeatPenalty"))
            data.append({ ModelList::RepeatPenaltyRole, obj["repeatPenalty"].toDouble() });
        if (obj.contains("repeatPenaltyTokens"))
            data.append({ ModelList::RepeatPenaltyTokensRole, obj["repeatPenaltyTokens"].toInt() });
        if (auto it = obj.find("chatTemplate"_L1); it != obj.end())
            data.append({ ModelList::ChatTemplateRole, it->toString() });
        if (auto it = obj.find("systemMessage"_L1); it != obj.end())
            data.append({ ModelList::SystemMessageRole, it->toString() });
        updateData(id, data);
    }

    const QString chatGPTDesc = tr("<ul><li>Requires personal OpenAI API key.</li><li>WARNING: Will send"
        " your chats to OpenAI!</li><li>Your API key will be stored on disk</li><li>Will only be used"
        " to communicate with OpenAI</li><li>You can apply for an API key"
        " <a href=\"https://platform.openai.com/account/api-keys\">here.</a></li>");

    {
        const QString modelName = "ChatGPT-3.5 Turbo";
        const QString id = modelName;
        const QString modelFilename = "gpt4all-gpt-3.5-turbo.rmodel";
        if (contains(modelFilename))
            changeId(modelFilename, id);
        if (!contains(id))
            addModel(id);
        QVector<QPair<int, QVariant>> data {
            { ModelList::NameRole, modelName },
            { ModelList::FilenameRole, modelFilename },
            { ModelList::FilesizeRole, "minimal" },
            { ModelList::OnlineRole, true },
            { ModelList::DescriptionRole,
             tr("<strong>OpenAI's ChatGPT model GPT-3.5 Turbo</strong><br> %1").arg(chatGPTDesc) },
            { ModelList::RequiresVersionRole, "2.7.4" },
            { ModelList::OrderRole, "ca" },
            { ModelList::RamrequiredRole, 0 },
            { ModelList::ParametersRole, "?" },
            { ModelList::QuantRole, "NA" },
            { ModelList::TypeRole, "GPT" },
            { ModelList::UrlRole, "https://api.openai.com/v1/chat/completions" },
            { ModelList::ChatTemplateRole, RMODEL_CHAT_TEMPLATE },
        };
        updateData(id, data);
    }

    {
        const QString chatGPT4Warn = tr("<br><br><i>* Even if you pay OpenAI for ChatGPT-4 this does not guarantee API key access. Contact OpenAI for more info.");

        const QString modelName = "ChatGPT-4";
        const QString id = modelName;
        const QString modelFilename = "gpt4all-gpt-4.rmodel";
        if (contains(modelFilename))
            changeId(modelFilename, id);
        if (!contains(id))
            addModel(id);
        QVector<QPair<int, QVariant>> data {
            { ModelList::NameRole, modelName },
            { ModelList::FilenameRole, modelFilename },
            { ModelList::FilesizeRole, "minimal" },
            { ModelList::OnlineRole, true },
            { ModelList::DescriptionRole,
             tr("<strong>OpenAI's ChatGPT model GPT-4</strong><br> %1 %2").arg(chatGPTDesc).arg(chatGPT4Warn) },
            { ModelList::RequiresVersionRole, "2.7.4" },
            { ModelList::OrderRole, "cb" },
            { ModelList::RamrequiredRole, 0 },
            { ModelList::ParametersRole, "?" },
            { ModelList::QuantRole, "NA" },
            { ModelList::TypeRole, "GPT" },
            { ModelList::UrlRole, "https://api.openai.com/v1/chat/completions" },
            { ModelList::ChatTemplateRole, RMODEL_CHAT_TEMPLATE },
        };
        updateData(id, data);
    }

    const QString mistralDesc = tr("<ul><li>Requires personal Mistral API key.</li><li>WARNING: Will send"
                                   " your chats to Mistral!</li><li>Your API key will be stored on disk</li><li>Will only be used"
                                   " to communicate with Mistral</li><li>You can apply for an API key"
                                   " <a href=\"https://console.mistral.ai/user/api-keys\">here</a>.</li>");

    {
        const QString modelName = "Mistral Tiny API";
        const QString id = modelName;
        const QString modelFilename = "gpt4all-mistral-tiny.rmodel";
        if (contains(modelFilename))
            changeId(modelFilename, id);
        if (!contains(id))
            addModel(id);
        QVector<QPair<int, QVariant>> data {
            { ModelList::NameRole, modelName },
            { ModelList::FilenameRole, modelFilename },
            { ModelList::FilesizeRole, "minimal" },
            { ModelList::OnlineRole, true },
            { ModelList::DescriptionRole,
             tr("<strong>Mistral Tiny model</strong><br> %1").arg(mistralDesc) },
            { ModelList::RequiresVersionRole, "2.7.4" },
            { ModelList::OrderRole, "cc" },
            { ModelList::RamrequiredRole, 0 },
            { ModelList::ParametersRole, "?" },
            { ModelList::QuantRole, "NA" },
            { ModelList::TypeRole, "Mistral" },
            { ModelList::UrlRole, "https://api.mistral.ai/v1/chat/completions" },
            { ModelList::ChatTemplateRole, RMODEL_CHAT_TEMPLATE },
        };
        updateData(id, data);
    }
    {
        const QString modelName = "Mistral Small API";
        const QString id = modelName;
        const QString modelFilename = "gpt4all-mistral-small.rmodel";
        if (contains(modelFilename))
            changeId(modelFilename, id);
        if (!contains(id))
            addModel(id);
        QVector<QPair<int, QVariant>> data {
            { ModelList::NameRole, modelName },
            { ModelList::FilenameRole, modelFilename },
            { ModelList::FilesizeRole, "minimal" },
            { ModelList::OnlineRole, true },
            { ModelList::DescriptionRole,
             tr("<strong>Mistral Small model</strong><br> %1").arg(mistralDesc) },
            { ModelList::RequiresVersionRole, "2.7.4" },
            { ModelList::OrderRole, "cd" },
            { ModelList::RamrequiredRole, 0 },
            { ModelList::ParametersRole, "?" },
            { ModelList::QuantRole, "NA" },
            { ModelList::TypeRole, "Mistral" },
            { ModelList::UrlRole, "https://api.mistral.ai/v1/chat/completions" },
            { ModelList::ChatTemplateRole, RMODEL_CHAT_TEMPLATE },
        };
        updateData(id, data);
    }

    {
        const QString modelName = "Mistral Medium API";
        const QString id = modelName;
        const QString modelFilename = "gpt4all-mistral-medium.rmodel";
        if (contains(modelFilename))
            changeId(modelFilename, id);
        if (!contains(id))
            addModel(id);
        QVector<QPair<int, QVariant>> data {
            { ModelList::NameRole, modelName },
            { ModelList::FilenameRole, modelFilename },
            { ModelList::FilesizeRole, "minimal" },
            { ModelList::OnlineRole, true },
            { ModelList::DescriptionRole,
             tr("<strong>Mistral Medium model</strong><br> %1").arg(mistralDesc) },
            { ModelList::RequiresVersionRole, "2.7.4" },
            { ModelList::OrderRole, "ce" },
            { ModelList::RamrequiredRole, 0 },
            { ModelList::ParametersRole, "?" },
            { ModelList::QuantRole, "NA" },
            { ModelList::TypeRole, "Mistral" },
            { ModelList::UrlRole, "https://api.mistral.ai/v1/chat/completions" },
            { ModelList::ChatTemplateRole, RMODEL_CHAT_TEMPLATE },
        };
        updateData(id, data);
    }

    const QString compatibleDesc = tr("<ul><li>Requires personal API key and the API base URL.</li>"
                                      "<li>WARNING: Will send your chats to "
                                      "the OpenAI-compatible API Server you specified!</li>"
                                      "<li>Your API key will be stored on disk</li><li>Will only be used"
                                      " to communicate with the OpenAI-compatible API Server</li>");

    {
        const QString modelName = "OpenAI-compatible";
        const QString id = modelName;
        if (!contains(id))
            addModel(id);
        QVector<QPair<int, QVariant>> data {
            { ModelList::NameRole, modelName },
            { ModelList::FilesizeRole, "minimal" },
            { ModelList::OnlineRole, true },
            { ModelList::CompatibleApiRole, true },
            { ModelList::DescriptionRole,
             tr("<strong>Connect to OpenAI-compatible API server</strong><br> %1").arg(compatibleDesc) },
            { ModelList::RequiresVersionRole, "2.7.4" },
            { ModelList::OrderRole, "cf" },
            { ModelList::RamrequiredRole, 0 },
            { ModelList::ParametersRole, "?" },
            { ModelList::QuantRole, "NA" },
            { ModelList::TypeRole, "NA" },
            { ModelList::ChatTemplateRole, RMODEL_CHAT_TEMPLATE },
        };
        updateData(id, data);
    }
}

void ModelList::updateDiscoveredInstalled(const ModelInfo &info)
{
    QVector<QPair<int, QVariant>> data {
        { ModelList::InstalledRole, true },
        { ModelList::IsDiscoveredRole, true },
        { ModelList::NameRole, info.name() },
        { ModelList::FilenameRole, info.filename() },
        { ModelList::DescriptionRole, info.description() },
        { ModelList::UrlRole, info.url() },
        { ModelList::LikesRole, info.likes() },
        { ModelList::DownloadsRole, info.downloads() },
        { ModelList::RecencyRole, info.recency() },
        { ModelList::QuantRole, info.quant() },
        { ModelList::TypeRole, info.type() },
    };
    updateData(info.id(), data);
}

// FIXME(jared): This should only contain fields without reasonable defaults such as name, description, and URL.
//               For other settings, there is no authoritative value and we should load the setting lazily like we do
//               for any other override.
void ModelList::updateModelsFromSettings()
{
    QSettings settings;
    QStringList groups = settings.childGroups();
    for (const QString &g: groups) {
        if (!g.startsWith("model-"))
            continue;

        const QString id = g.sliced(6);
        if (contains(id))
            continue;

        // If we can't find the corresponding file, then ignore it as this reflects a stale model.
        // The file could have been deleted manually by the user for instance or temporarily renamed.
        QString filename;
        {
            auto value = settings.value(u"%1/filename"_s.arg(g));
            if (!value.isValid() || !modelExists(filename = value.toString()))
                continue;
        }

        QVector<QPair<int, QVariant>> data;

        // load data from base model
        // FIXME(jared): how does "Restore Defaults" work for other settings of clones which we don't do this for?
        if (auto base = modelInfoByFilename(filename, /*allowClone*/ false); !base.id().isNull()) {
            if (auto tmpl = base.m_chatTemplate)
                data.append({ ModelList::ChatTemplateRole,  *tmpl });
            if (auto msg  = base.m_systemMessage; !msg.isNull())
                data.append({ ModelList::SystemMessageRole,  msg  });
        }

        addModel(id);

        // load data from settings
        if (settings.contains(g + "/name")) {
            const QString name = settings.value(g + "/name").toString();
            data.append({ ModelList::NameRole, name });
        }
        if (settings.contains(g + "/filename")) {
            const QString filename = settings.value(g + "/filename").toString();
            data.append({ ModelList::FilenameRole, filename });
        }
        if (settings.contains(g + "/description")) {
            const QString d = settings.value(g + "/description").toString();
            data.append({ ModelList::DescriptionRole, d });
        }
        if (settings.contains(g + "/url")) {
            const QString u = settings.value(g + "/url").toString();
            data.append({ ModelList::UrlRole, u });
        }
        if (settings.contains(g + "/quant")) {
            const QString q = settings.value(g + "/quant").toString();
            data.append({ ModelList::QuantRole, q });
        }
        if (settings.contains(g + "/type")) {
            const QString t = settings.value(g + "/type").toString();
            data.append({ ModelList::TypeRole, t });
        }
        if (settings.contains(g + "/isClone")) {
            const bool b = settings.value(g + "/isClone").toBool();
            data.append({ ModelList::IsCloneRole, b });
        }
        if (settings.contains(g + "/isDiscovered")) {
            const bool b = settings.value(g + "/isDiscovered").toBool();
            data.append({ ModelList::IsDiscoveredRole, b });
        }
        if (settings.contains(g + "/likes")) {
            const int l = settings.value(g + "/likes").toInt();
            data.append({ ModelList::LikesRole, l });
        }
        if (settings.contains(g + "/downloads")) {
            const int d = settings.value(g + "/downloads").toInt();
            data.append({ ModelList::DownloadsRole, d });
        }
        if (settings.contains(g + "/recency")) {
            const QDateTime r = settings.value(g + "/recency").toDateTime();
            data.append({ ModelList::RecencyRole, r });
        }
        if (settings.contains(g + "/temperature")) {
            const double temperature = settings.value(g + "/temperature").toDouble();
            data.append({ ModelList::TemperatureRole, temperature });
        }
        if (settings.contains(g + "/topP")) {
            const double topP = settings.value(g + "/topP").toDouble();
            data.append({ ModelList::TopPRole, topP });
        }
        if (settings.contains(g + "/minP")) {
            const double minP = settings.value(g + "/minP").toDouble();
            data.append({ ModelList::MinPRole, minP });
        }
        if (settings.contains(g + "/topK")) {
            const int topK = settings.value(g + "/topK").toInt();
            data.append({ ModelList::TopKRole, topK });
        }
        if (settings.contains(g + "/maxLength")) {
            const int maxLength = settings.value(g + "/maxLength").toInt();
            data.append({ ModelList::MaxLengthRole, maxLength });
        }
        if (settings.contains(g + "/promptBatchSize")) {
            const int promptBatchSize = settings.value(g + "/promptBatchSize").toInt();
            data.append({ ModelList::PromptBatchSizeRole, promptBatchSize });
        }
        if (settings.contains(g + "/contextLength")) {
            const int contextLength = settings.value(g + "/contextLength").toInt();
            data.append({ ModelList::ContextLengthRole, contextLength });
        }
        if (settings.contains(g + "/gpuLayers")) {
            const int gpuLayers = settings.value(g + "/gpuLayers").toInt();
            data.append({ ModelList::GpuLayersRole, gpuLayers });
        }
        if (settings.contains(g + "/repeatPenalty")) {
            const double repeatPenalty = settings.value(g + "/repeatPenalty").toDouble();
            data.append({ ModelList::RepeatPenaltyRole, repeatPenalty });
        }
        if (settings.contains(g + "/repeatPenaltyTokens")) {
            const int repeatPenaltyTokens = settings.value(g + "/repeatPenaltyTokens").toInt();
            data.append({ ModelList::RepeatPenaltyTokensRole, repeatPenaltyTokens });
        }
        if (settings.contains(g + "/chatNamePrompt")) {
            const QString chatNamePrompt = settings.value(g + "/chatNamePrompt").toString();
            data.append({ ModelList::ChatNamePromptRole, chatNamePrompt });
        }
        if (settings.contains(g + "/suggestedFollowUpPrompt")) {
            const QString suggestedFollowUpPrompt = settings.value(g + "/suggestedFollowUpPrompt").toString();
            data.append({ ModelList::SuggestedFollowUpPromptRole, suggestedFollowUpPrompt });
        }
        updateData(id, data);
    }
}

int ModelList::discoverLimit() const
{
    return m_discoverLimit;
}

void ModelList::setDiscoverLimit(int limit)
{
    if (m_discoverLimit == limit)
        return;
    m_discoverLimit = limit;
    emit discoverLimitChanged();
}

int ModelList::discoverSortDirection() const
{
    return m_discoverSortDirection;
}

void ModelList::setDiscoverSortDirection(int direction)
{
    if (m_discoverSortDirection == direction || (direction != 1 && direction != -1))
        return;
    m_discoverSortDirection = direction;
    emit discoverSortDirectionChanged();
    resortModel();
}

ModelList::DiscoverSort ModelList::discoverSort() const
{
    return m_discoverSort;
}

void ModelList::setDiscoverSort(DiscoverSort sort)
{
    if (m_discoverSort == sort)
        return;
    m_discoverSort = sort;
    emit discoverSortChanged();
    resortModel();
}

void ModelList::clearDiscoveredModels()
{
    // NOTE: This could be made much more efficient
    QList<ModelInfo> infos;
    {
        QMutexLocker locker(&m_mutex);
        for (ModelInfo *info : m_models)
            if (info->isDiscovered() && !info->installed)
                infos.append(*info);
    }
    for (ModelInfo &info : infos)
        removeInternal(info);
}

float ModelList::discoverProgress() const
{
    if (!m_discoverNumberOfResults)
        return 0.0f;
    return m_discoverResultsCompleted / float(m_discoverNumberOfResults);
}

bool ModelList::discoverInProgress() const
{
    return m_discoverInProgress;
}

void ModelList::discoverSearch(const QString &search)
{
    Q_ASSERT(!m_discoverInProgress);

    clearDiscoveredModels();

    m_discoverNumberOfResults = 0;
    m_discoverResultsCompleted = 0;
    emit discoverProgressChanged();

    if (search.isEmpty()) {
        return;
    }

    m_discoverInProgress = true;
    emit discoverInProgressChanged();

    static const QRegularExpression wsRegex("\\s+");
    QStringList searchParams = search.split(wsRegex); // split by whitespace
    QString searchString = u"search=%1&"_s.arg(searchParams.join('+'));
    QString limitString = m_discoverLimit > 0 ? u"limit=%1&"_s.arg(m_discoverLimit) : QString();

    QString sortString;
    switch (m_discoverSort) {
    case Default: break;
    case Likes:
        sortString = "sort=likes&"; break;
    case Downloads:
        sortString = "sort=downloads&"; break;
    case Recent:
        sortString = "sort=lastModified&"; break;
    }

    QString directionString = !sortString.isEmpty() ? u"direction=%1&"_s.arg(m_discoverSortDirection) : QString();

    QUrl hfUrl(u"https://huggingface.co/api/models?filter=gguf&%1%2%3%4full=true&config=true"_s
               .arg(searchString, limitString, sortString, directionString));

    QNetworkRequest request(hfUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = m_networkManager.get(request);
    connect(qGuiApp, &QCoreApplication::aboutToQuit, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::finished, this, &ModelList::handleDiscoveryFinished);
    connect(reply, &QNetworkReply::errorOccurred, this, &ModelList::handleDiscoveryErrorOccurred);
}

void ModelList::handleDiscoveryFinished()
{
    QNetworkReply *jsonReply = qobject_cast<QNetworkReply *>(sender());
    if (!jsonReply)
        return;

    QByteArray jsonData = jsonReply->readAll();
    parseDiscoveryJsonFile(jsonData);
    jsonReply->deleteLater();
}

void ModelList::handleDiscoveryErrorOccurred(QNetworkReply::NetworkError code)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;
    qWarning() << u"ERROR: Discovery failed with error code \"%1-%2\""_s
                      .arg(code).arg(reply->errorString()).toStdString();
}

enum QuantType {
    Q4_0 = 0,
    Q4_1,
    F16,
    F32,
    Unknown
};

QuantType toQuantType(const QString& filename)
{
    QString lowerCaseFilename = filename.toLower();
    if (lowerCaseFilename.contains("q4_0")) return Q4_0;
    if (lowerCaseFilename.contains("q4_1")) return Q4_1;
    if (lowerCaseFilename.contains("f16")) return F16;
    if (lowerCaseFilename.contains("f32")) return F32;
    return Unknown;
}

QString toQuantString(const QString& filename)
{
    QString lowerCaseFilename = filename.toLower();
    if (lowerCaseFilename.contains("q4_0")) return "q4_0";
    if (lowerCaseFilename.contains("q4_1")) return "q4_1";
    if (lowerCaseFilename.contains("f16")) return "f16";
    if (lowerCaseFilename.contains("f32")) return "f32";
    return QString();
}

void ModelList::parseDiscoveryJsonFile(const QByteArray &jsonData)
{
    QJsonParseError err;
    QJsonDocument document = QJsonDocument::fromJson(jsonData, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "ERROR: Couldn't parse: " << jsonData << err.errorString();
        m_discoverNumberOfResults = 0;
        m_discoverResultsCompleted = 0;
        emit discoverProgressChanged();
        m_discoverInProgress = false;
        emit discoverInProgressChanged();
        return;
    }

    QJsonArray jsonArray = document.array();

    for (const QJsonValue &value : jsonArray) {
        QJsonObject obj = value.toObject();
        QJsonDocument jsonDocument(obj);
        QByteArray jsonData = jsonDocument.toJson();

        QString repo_id = obj["id"].toString();
        QJsonArray siblingsArray = obj["siblings"].toArray();
        QList<QPair<QuantType, QString>> filteredAndSortedFilenames;
        for (const QJsonValue &sibling : siblingsArray) {

            QJsonObject s = sibling.toObject();
            QString filename = s["rfilename"].toString();
            if (!filename.endsWith("gguf"))
                continue;

            QuantType quant = toQuantType(filename);
            if (quant != Unknown)
                filteredAndSortedFilenames.append({ quant, filename });
        }

        if (filteredAndSortedFilenames.isEmpty())
            continue;

        std::sort(filteredAndSortedFilenames.begin(), filteredAndSortedFilenames.end(),
            [](const QPair<QuantType, QString>& a, const QPair<QuantType, QString>& b) {
            return a.first < b.first;
        });

        QPair<QuantType, QString> file = filteredAndSortedFilenames.first();
        QString filename = file.second;
        ++m_discoverNumberOfResults;

        QUrl url(u"https://huggingface.co/%1/resolve/main/%2"_s.arg(repo_id, filename));
        QNetworkRequest request(url);
        request.setRawHeader("Accept-Encoding", "identity");
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
        request.setAttribute(QNetworkRequest::User, jsonData);
        request.setAttribute(QNetworkRequest::UserMax, filename);
        QNetworkReply *reply = m_networkManager.head(request);
        connect(qGuiApp, &QCoreApplication::aboutToQuit, reply, &QNetworkReply::abort);
        connect(reply, &QNetworkReply::finished, this, &ModelList::handleDiscoveryItemFinished);
        connect(reply, &QNetworkReply::errorOccurred, this, &ModelList::handleDiscoveryItemErrorOccurred);
    }

    emit discoverProgressChanged();
    if (!m_discoverNumberOfResults) {
        m_discoverInProgress = false;
        emit discoverInProgressChanged();
    }
}

void ModelList::handleDiscoveryItemFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;

    QVariant replyCustomData = reply->request().attribute(QNetworkRequest::User);
    QByteArray customDataByteArray = replyCustomData.toByteArray();
    QJsonDocument customJsonDocument = QJsonDocument::fromJson(customDataByteArray);
    QJsonObject obj = customJsonDocument.object();

    QString repo_id = obj["id"].toString();
    QString modelName = obj["modelId"].toString();
    QString author = obj["author"].toString();
    QDateTime lastModified = QDateTime::fromString(obj["lastModified"].toString(), Qt::ISODateWithMs);
    int likes = obj["likes"].toInt();
    int downloads = obj["downloads"].toInt();
    QJsonObject config = obj["config"].toObject();
    QString type = config["model_type"].toString();

    // QByteArray repoCommitHeader = reply->rawHeader("X-Repo-Commit");
    QByteArray linkedSizeHeader = reply->rawHeader("X-Linked-Size");
    QByteArray linkedEtagHeader = reply->rawHeader("X-Linked-Etag");
    // For some reason these seem to contain quotation marks ewww
    linkedEtagHeader.replace("\"", "");
    linkedEtagHeader.replace("\'", "");
    // QString locationHeader = reply->header(QNetworkRequest::LocationHeader).toString();

    QString modelFilename = reply->request().attribute(QNetworkRequest::UserMax).toString();
    QString modelFilesize = ModelList::toFileSize(QString(linkedSizeHeader).toULongLong());

    QString description = tr("<strong>Created by %1.</strong><br><ul>"
                             "<li>Published on %2."
                             "<li>This model has %3 likes."
                             "<li>This model has %4 downloads."
                             "<li>More info can be found <a href=\"https://huggingface.co/%5\">here.</a></ul>")
                              .arg(author)
                              .arg(lastModified.toString("ddd MMMM d, yyyy"))
                              .arg(likes)
                              .arg(downloads)
                              .arg(repo_id);

    const QString id = modelFilename;
    Q_ASSERT(!id.isEmpty());

    if (contains(modelFilename))
        changeId(modelFilename, id);

    if (!contains(id))
        addModel(id);

    QVector<QPair<int, QVariant>> data {
        { ModelList::NameRole, modelName },
        { ModelList::FilenameRole, modelFilename },
        { ModelList::FilesizeRole, modelFilesize },
        { ModelList::DescriptionRole, description },
        { ModelList::IsDiscoveredRole, true },
        { ModelList::UrlRole, reply->request().url() },
        { ModelList::LikesRole, likes },
        { ModelList::DownloadsRole, downloads },
        { ModelList::RecencyRole, lastModified },
        { ModelList::QuantRole, toQuantString(modelFilename) },
        { ModelList::TypeRole, type },
        { ModelList::HashRole, linkedEtagHeader },
        { ModelList::HashAlgorithmRole, ModelInfo::Sha256 },
    };
    updateData(id, data);

    ++m_discoverResultsCompleted;
    emit discoverProgressChanged();

    if (discoverProgress() >= 1.0) {
        m_discoverInProgress = false;
        emit discoverInProgressChanged();
    }

    reply->deleteLater();
}

void ModelList::handleDiscoveryItemErrorOccurred(QNetworkReply::NetworkError code)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;

    qWarning() << u"ERROR: Discovery item failed with error code \"%1-%2\""_s
                      .arg(code).arg(reply->errorString()).toStdString();
}
