#include "modellist.h"
#include "mysettings.h"
#include "network.h"
#include "../gpt4all-backend/llmodel.h"

#include <QFile>
#include <QStandardPaths>
#include <algorithm>

//#define USE_LOCAL_MODELSJSON

#define DEFAULT_EMBEDDING_MODEL "all-MiniLM-L6-v2-f16.gguf"
#define NOMIC_EMBEDDING_MODEL "nomic-embed-text-v1.txt"

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
    if (isClone) MySettings::globalInstance()->setModelName(*this, name, isClone /*force*/);
    m_name = name;
}

QString ModelInfo::filename() const
{
    return MySettings::globalInstance()->modelFilename(*this);
}

void ModelInfo::setFilename(const QString &filename)
{
    if (isClone) MySettings::globalInstance()->setModelFilename(*this, filename, isClone /*force*/);
    m_filename = filename;
}

double ModelInfo::temperature() const
{
    return MySettings::globalInstance()->modelTemperature(*this);
}

void ModelInfo::setTemperature(double t)
{
    if (isClone) MySettings::globalInstance()->setModelTemperature(*this, t, isClone /*force*/);
    m_temperature = t;
}

double ModelInfo::topP() const
{
    return MySettings::globalInstance()->modelTopP(*this);
}

void ModelInfo::setTopP(double p)
{
    if (isClone) MySettings::globalInstance()->setModelTopP(*this, p, isClone /*force*/);
    m_topP = p;
}

int ModelInfo::topK() const
{
    return MySettings::globalInstance()->modelTopK(*this);
}

void ModelInfo::setTopK(int k)
{
    if (isClone) MySettings::globalInstance()->setModelTopK(*this, k, isClone /*force*/);
    m_topK = k;
}

int ModelInfo::maxLength() const
{
    return MySettings::globalInstance()->modelMaxLength(*this);
}

void ModelInfo::setMaxLength(int l)
{
    if (isClone) MySettings::globalInstance()->setModelMaxLength(*this, l, isClone /*force*/);
    m_maxLength = l;
}

int ModelInfo::promptBatchSize() const
{
    return MySettings::globalInstance()->modelPromptBatchSize(*this);
}

void ModelInfo::setPromptBatchSize(int s)
{
    if (isClone) MySettings::globalInstance()->setModelPromptBatchSize(*this, s, isClone /*force*/);
    m_promptBatchSize = s;
}

int ModelInfo::contextLength() const
{
    return MySettings::globalInstance()->modelContextLength(*this);
}

void ModelInfo::setContextLength(int l)
{
    if (isClone) MySettings::globalInstance()->setModelContextLength(*this, l, isClone /*force*/);
    m_contextLength = l;
}

int ModelInfo::maxContextLength() const
{
    if (m_maxContextLength != -1) return m_maxContextLength;
    auto path = (dirpath + filename()).toStdString();
    int layers = LLModel::Implementation::maxContextLength(path);
    if (layers < 0) {
        layers = 4096; // fallback value
    }
    m_maxContextLength = layers;
    return m_maxContextLength;
}

int ModelInfo::gpuLayers() const
{
    return MySettings::globalInstance()->modelGpuLayers(*this);
}

void ModelInfo::setGpuLayers(int l)
{
    if (isClone) MySettings::globalInstance()->setModelGpuLayers(*this, l, isClone /*force*/);
    m_gpuLayers = l;
}

int ModelInfo::maxGpuLayers() const
{
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
    if (isClone) MySettings::globalInstance()->setModelRepeatPenalty(*this, p, isClone /*force*/);
    m_repeatPenalty = p;
}

int ModelInfo::repeatPenaltyTokens() const
{
    return MySettings::globalInstance()->modelRepeatPenaltyTokens(*this);
}

void ModelInfo::setRepeatPenaltyTokens(int t)
{
    if (isClone) MySettings::globalInstance()->setModelRepeatPenaltyTokens(*this, t, isClone /*force*/);
    m_repeatPenaltyTokens = t;
}

QString ModelInfo::promptTemplate() const
{
    return MySettings::globalInstance()->modelPromptTemplate(*this);
}

void ModelInfo::setPromptTemplate(const QString &t)
{
    if (isClone) MySettings::globalInstance()->setModelPromptTemplate(*this, t, isClone /*force*/);
    m_promptTemplate = t;
}

QString ModelInfo::systemPrompt() const
{
    return MySettings::globalInstance()->modelSystemPrompt(*this);
}

void ModelInfo::setSystemPrompt(const QString &p)
{
    if (isClone) MySettings::globalInstance()->setModelSystemPrompt(*this, p, isClone /*force*/);
    m_systemPrompt = p;
}

EmbeddingModels::EmbeddingModels(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    connect(this, &EmbeddingModels::rowsInserted, this, &EmbeddingModels::countChanged);
    connect(this, &EmbeddingModels::rowsRemoved, this, &EmbeddingModels::countChanged);
    connect(this, &EmbeddingModels::modelReset, this, &EmbeddingModels::countChanged);
    connect(this, &EmbeddingModels::layoutChanged, this, &EmbeddingModels::countChanged);
}

bool EmbeddingModels::filterAcceptsRow(int sourceRow,
                                       const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    bool isInstalled = sourceModel()->data(index, ModelList::InstalledRole).toBool();
    bool isEmbedding = sourceModel()->data(index, ModelList::FilenameRole).toString() == DEFAULT_EMBEDDING_MODEL ||
        sourceModel()->data(index, ModelList::FilenameRole).toString() == NOMIC_EMBEDDING_MODEL;
    return isInstalled && isEmbedding;
}

int EmbeddingModels::count() const
{
    return rowCount();
}

ModelInfo EmbeddingModels::defaultModelInfo() const
{
    if (!sourceModel())
        return ModelInfo();

    const ModelList *sourceListModel = qobject_cast<const ModelList*>(sourceModel());
    if (!sourceListModel)
        return ModelInfo();

    const int rows = sourceListModel->rowCount();
    for (int i = 0; i < rows; ++i) {
        QModelIndex sourceIndex = sourceListModel->index(i, 0);
        if (filterAcceptsRow(i, sourceIndex.parent())) {
            const QString id = sourceListModel->data(sourceIndex, ModelList::IdRole).toString();
            return sourceListModel->modelInfo(id);
        }
    }

    return ModelInfo();
}

InstalledModels::InstalledModels(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    connect(this, &InstalledModels::rowsInserted, this, &InstalledModels::countChanged);
    connect(this, &InstalledModels::rowsRemoved, this, &InstalledModels::countChanged);
    connect(this, &InstalledModels::modelReset, this, &InstalledModels::countChanged);
    connect(this, &InstalledModels::layoutChanged, this, &InstalledModels::countChanged);
}

bool InstalledModels::filterAcceptsRow(int sourceRow,
                                       const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    bool isInstalled = sourceModel()->data(index, ModelList::InstalledRole).toBool();
    bool showInGUI = !sourceModel()->data(index, ModelList::DisableGUIRole).toBool();
    return isInstalled && showInGUI;
}

int InstalledModels::count() const
{
    return rowCount();
}

DownloadableModels::DownloadableModels(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_expanded(false)
    , m_limit(5)
{
    connect(this, &DownloadableModels::rowsInserted, this, &DownloadableModels::countChanged);
    connect(this, &DownloadableModels::rowsRemoved, this, &DownloadableModels::countChanged);
    connect(this, &DownloadableModels::modelReset, this, &DownloadableModels::countChanged);
    connect(this, &DownloadableModels::layoutChanged, this, &DownloadableModels::countChanged);
}

bool DownloadableModels::filterAcceptsRow(int sourceRow,
                                          const QModelIndex &sourceParent) const
{
    bool withinLimit = sourceRow < (m_expanded ? sourceModel()->rowCount() : m_limit);
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    bool isDownloadable = !sourceModel()->data(index, ModelList::DescriptionRole).toString().isEmpty();
    return withinLimit && isDownloadable;
}

int DownloadableModels::count() const
{
    return rowCount();
}

bool DownloadableModels::isExpanded() const
{
    return m_expanded;
}

void DownloadableModels::setExpanded(bool expanded)
{
    if (m_expanded != expanded) {
        m_expanded = expanded;
        invalidateFilter();
        emit expandedChanged(m_expanded);
    }
}

class MyModelList: public ModelList { };
Q_GLOBAL_STATIC(MyModelList, modelListInstance)
ModelList *ModelList::globalInstance()
{
    return modelListInstance();
}

ModelList::ModelList()
    : QAbstractListModel(nullptr)
    , m_embeddingModels(new EmbeddingModels(this))
    , m_installedModels(new InstalledModels(this))
    , m_downloadableModels(new DownloadableModels(this))
    , m_asyncModelRequestOngoing(false)
{
    m_embeddingModels->setSourceModel(this);
    m_installedModels->setSourceModel(this);
    m_downloadableModels->setSourceModel(this);

    connect(MySettings::globalInstance(), &MySettings::modelPathChanged, this, &ModelList::updateModelsFromDirectory);
    connect(MySettings::globalInstance(), &MySettings::modelPathChanged, this, &ModelList::updateModelsFromJson);
    connect(MySettings::globalInstance(), &MySettings::modelPathChanged, this, &ModelList::updateModelsFromSettings);
    connect(MySettings::globalInstance(), &MySettings::nameChanged, this, &ModelList::updateDataForSettings);
    connect(MySettings::globalInstance(), &MySettings::temperatureChanged, this, &ModelList::updateDataForSettings);
    connect(MySettings::globalInstance(), &MySettings::topPChanged, this, &ModelList::updateDataForSettings);
    connect(MySettings::globalInstance(), &MySettings::topKChanged, this, &ModelList::updateDataForSettings);
    connect(MySettings::globalInstance(), &MySettings::maxLengthChanged, this, &ModelList::updateDataForSettings);
    connect(MySettings::globalInstance(), &MySettings::promptBatchSizeChanged, this, &ModelList::updateDataForSettings);
    connect(MySettings::globalInstance(), &MySettings::contextLengthChanged, this, &ModelList::updateDataForSettings);
    connect(MySettings::globalInstance(), &MySettings::gpuLayersChanged, this, &ModelList::updateDataForSettings);
    connect(MySettings::globalInstance(), &MySettings::repeatPenaltyChanged, this, &ModelList::updateDataForSettings);
    connect(MySettings::globalInstance(), &MySettings::repeatPenaltyTokensChanged, this, &ModelList::updateDataForSettings);;
    connect(MySettings::globalInstance(), &MySettings::promptTemplateChanged, this, &ModelList::updateDataForSettings);
    connect(MySettings::globalInstance(), &MySettings::systemPromptChanged, this, &ModelList::updateDataForSettings);
    connect(&m_networkManager, &QNetworkAccessManager::sslErrors, this, &ModelList::handleSslErrors);

    updateModelsFromJson();
    updateModelsFromSettings();
    updateModelsFromDirectory();
}

QString ModelList::incompleteDownloadPath(const QString &modelFile)
{
    return MySettings::globalInstance()->modelPath() + "incomplete-" + modelFile;
}

const QList<ModelInfo> ModelList::exportModelList() const
{
    QMutexLocker locker(&m_mutex);
    QList<ModelInfo> infos;
    for (ModelInfo *info : m_models)
        if (info->installed)
            infos.append(*info);
    return infos;
}

const QList<QString> ModelList::userDefaultModelList() const
{
    QMutexLocker locker(&m_mutex);

    const QString userDefaultModelName = MySettings::globalInstance()->userDefaultModel();
    QList<QString> models;
    bool foundUserDefault = false;
    for (ModelInfo *info : m_models) {
        if (info->installed && info->id() == userDefaultModelName) {
            foundUserDefault = true;
            models.prepend(info->name());
        } else if (info->installed) {
            models.append(info->name());
        }
    }

    const QString defaultId = "Application default";
    if (foundUserDefault)
        models.append(defaultId);
    else
        models.prepend(defaultId);
    return models;
}

int ModelList::defaultEmbeddingModelIndex() const
{
    QMutexLocker locker(&m_mutex);
    for (int i = 0; i < m_models.size(); ++i) {
        const ModelInfo *info = m_models.at(i);
        const bool isEmbedding = info->filename() == DEFAULT_EMBEDDING_MODEL;
        if (isEmbedding) return i;
    }
    return -1;
}

ModelInfo ModelList::defaultModelInfo() const
{
    QMutexLocker locker(&m_mutex);

    QSettings settings;
    settings.sync();

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

bool ModelList::lessThan(const ModelInfo* a, const ModelInfo* b)
{
    // Rule 0: Non-clone before clone
    if (a->isClone != b->isClone) {
        return !a->isClone;
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

    int modelSizeBefore = 0;
    int modelSizeAfter = 0;
    {
        QMutexLocker locker(&m_mutex);
        modelSizeBefore = m_models.size();
    }
    beginInsertRows(QModelIndex(), modelSizeBefore, modelSizeBefore);
    {
        QMutexLocker locker(&m_mutex);
        ModelInfo *info = new ModelInfo;
        info->setId(id);
        m_models.append(info);
        m_modelMap.insert(id, info);
        std::stable_sort(m_models.begin(), m_models.end(), ModelList::lessThan);
        modelSizeAfter = m_models.size();
    }
    endInsertRows();
    emit dataChanged(index(0, 0), index(modelSizeAfter - 1, 0));
    emit userDefaultModelListChanged();
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
        case Md5sumRole:
            return info->md5sum;
        case CalcHashRole:
            return info->calcHash;
        case InstalledRole:
            return info->installed;
        case DefaultRole:
            return info->isDefault;
        case OnlineRole:
            return info->isOnline;
        case DisableGUIRole:
            return info->disableGUI;
        case DescriptionRole:
            return info->description;
        case RequiresVersionRole:
            return info->requiresVersion;
        case DeprecatedVersionRole:
            return info->deprecatedVersion;
        case UrlRole:
            return info->url;
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
            return info->quant;
        case TypeRole:
            return info->type;
        case IsCloneRole:
            return info->isClone;
        case TemperatureRole:
            return info->temperature();
        case TopPRole:
            return info->topP();
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
        case PromptTemplateRole:
            return info->promptTemplate();
        case SystemPromptRole:
            return info->systemPrompt();
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

void ModelList::updateData(const QString &id, int role, const QVariant &value)
{
    int modelSize;
    bool updateInstalled;
    bool updateIncomplete;
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

        switch (role) {
        case IdRole:
            info->setId(value.toString()); break;
        case NameRole:
            info->setName(value.toString()); break;
        case FilenameRole:
            info->setFilename(value.toString()); break;
        case DirpathRole:
            info->dirpath = value.toString(); break;
        case FilesizeRole:
            info->filesize = value.toString(); break;
        case Md5sumRole:
            info->md5sum = value.toByteArray(); break;
        case CalcHashRole:
            info->calcHash = value.toBool(); break;
        case InstalledRole:
            info->installed = value.toBool(); break;
        case DefaultRole:
            info->isDefault = value.toBool(); break;
        case OnlineRole:
            info->isOnline = value.toBool(); break;
        case DisableGUIRole:
            info->disableGUI = value.toBool(); break;
        case DescriptionRole:
            info->description = value.toString(); break;
        case RequiresVersionRole:
            info->requiresVersion = value.toString(); break;
        case DeprecatedVersionRole:
            info->deprecatedVersion = value.toString(); break;
        case UrlRole:
            info->url = value.toString(); break;
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
            info->order = value.toString(); break;
        case RamrequiredRole:
            info->ramrequired = value.toInt(); break;
        case ParametersRole:
            info->parameters = value.toString(); break;
        case QuantRole:
            info->quant = value.toString(); break;
        case TypeRole:
            info->type = value.toString(); break;
        case IsCloneRole:
            info->isClone = value.toBool(); break;
        case TemperatureRole:
            info->setTemperature(value.toDouble()); break;
        case TopPRole:
            info->setTopP(value.toDouble()); break;
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
        case PromptTemplateRole:
            info->setPromptTemplate(value.toString()); break;
        case SystemPromptRole:
            info->setSystemPrompt(value.toString()); break;
        }

        // Extra guarantee that these always remains in sync with filesystem
        QFileInfo fileInfo(info->dirpath + info->filename());
        if (info->installed != fileInfo.exists()) {
            info->installed = fileInfo.exists();
            updateInstalled = true;
        }
        QFileInfo incompleteInfo(incompleteDownloadPath(info->filename()));
        if (info->isIncomplete != incompleteInfo.exists()) {
            info->isIncomplete = incompleteInfo.exists();
            updateIncomplete = true;
        }

        std::stable_sort(m_models.begin(), m_models.end(), ModelList::lessThan);
        modelSize = m_models.size();
    }
    emit dataChanged(createIndex(0, 0), createIndex(modelSize - 1, 0));
    emit userDefaultModelListChanged();
}

void ModelList::updateDataByFilename(const QString &filename, int role, const QVariant &value)
{
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
        updateData(id, role, value);;
}

ModelInfo ModelList::modelInfo(const QString &id) const
{
    QMutexLocker locker(&m_mutex);
    if (!m_modelMap.contains(id))
        return ModelInfo();
    return *m_modelMap.value(id);
}

ModelInfo ModelList::modelInfoByFilename(const QString &filename) const
{
    QMutexLocker locker(&m_mutex);
    for (ModelInfo *info : m_models)
        if (info->filename() == filename)
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
    const QString id = Network::globalInstance()->generateUniqueId();
    addModel(id);
    updateData(id, ModelList::IsCloneRole, true);
    updateData(id, ModelList::NameRole, uniqueModelName(model));
    updateData(id, ModelList::FilenameRole, model.filename());
    updateData(id, ModelList::DirpathRole, model.dirpath);
    updateData(id, ModelList::InstalledRole, model.installed);
    updateData(id, ModelList::OnlineRole, model.isOnline);
    updateData(id, ModelList::TemperatureRole, model.temperature());
    updateData(id, ModelList::TopPRole, model.topP());
    updateData(id, ModelList::TopKRole, model.topK());
    updateData(id, ModelList::MaxLengthRole, model.maxLength());
    updateData(id, ModelList::PromptBatchSizeRole, model.promptBatchSize());
    updateData(id, ModelList::ContextLengthRole, model.contextLength());
    updateData(id, ModelList::GpuLayersRole, model.contextLength());
    updateData(id, ModelList::RepeatPenaltyRole, model.repeatPenalty());
    updateData(id, ModelList::RepeatPenaltyTokensRole, model.repeatPenaltyTokens());
    updateData(id, ModelList::PromptTemplateRole, model.promptTemplate());
    updateData(id, ModelList::SystemPromptRole, model.systemPrompt());
    return id;
}

void ModelList::remove(const ModelInfo &model)
{
    Q_ASSERT(model.isClone);
    if (!model.isClone)
        return;

    const bool hasModel = contains(model.id());
    Q_ASSERT(hasModel);
    if (!hasModel) {
        qWarning() << "ERROR: model list does not contain" << model.id();
        return;
    }

    int indexOfModel = 0;
    int modelSizeAfter = 0;
    {
        QMutexLocker locker(&m_mutex);
        ModelInfo *info = m_modelMap.value(model.id());
        indexOfModel = m_models.indexOf(info);
    }
    beginRemoveRows(QModelIndex(), indexOfModel, indexOfModel);
    {
        QMutexLocker locker(&m_mutex);
        ModelInfo *info = m_models.takeAt(indexOfModel);
        m_modelMap.remove(info->id());
        delete info;
        modelSizeAfter = m_models.size();
    }
    endRemoveRows();
    emit dataChanged(index(0, 0), index(modelSizeAfter - 1, 0));
    emit userDefaultModelListChanged();
    MySettings::globalInstance()->eraseModel(model);
}

QString ModelList::uniqueModelName(const ModelInfo &model) const
{
    QMutexLocker locker(&m_mutex);
    QRegularExpression re("^(.*)~(\\d+)$");
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

QString ModelList::modelDirPath(const QString &modelName, bool isOnline)
{
    QVector<QString> possibleFilePaths;
    if (isOnline)
        possibleFilePaths << "/" + modelName + ".txt";
    else {
        possibleFilePaths << "/ggml-" + modelName + ".bin";
        possibleFilePaths << "/" + modelName + ".bin";
    }
    for (const QString &modelFilename : possibleFilePaths) {
        QString appPath = QCoreApplication::applicationDirPath() + modelFilename;
        QFileInfo infoAppPath(appPath);
        if (infoAppPath.exists())
            return QCoreApplication::applicationDirPath();

        QString downloadPath = MySettings::globalInstance()->modelPath() + modelFilename;
        QFileInfo infoLocalPath(downloadPath);
        if (infoLocalPath.exists())
            return MySettings::globalInstance()->modelPath();
    }
    return QString();
}

void ModelList::updateModelsFromDirectory()
{
    const QString exePath = QCoreApplication::applicationDirPath() + QDir::separator();
    const QString localPath = MySettings::globalInstance()->modelPath();

    auto processDirectory = [&](const QString& path) {
        QDirIterator it(path, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();

            if (!it.fileInfo().isDir()) {
                QString filename = it.fileName();

                // All files that end with .bin and have 'ggml' somewhere in the name
                if (((filename.endsWith(".bin") || filename.endsWith(".gguf")) && (/*filename.contains("ggml") ||*/ filename.contains("gguf")) && !filename.startsWith("incomplete"))
                    || (filename.endsWith(".txt") && (filename.startsWith("chatgpt-") || filename.startsWith("nomic-")))) {

                    QString filePath = it.filePath();
                    QFileInfo info(filePath);

                    if (!info.exists())
                        continue;

                    QVector<QString> modelsById;
                    {
                        QMutexLocker locker(&m_mutex);
                        for (ModelInfo *info : m_models)
                            if (info->filename() == filename)
                                modelsById.append(info->id());
                    }

                    if (modelsById.isEmpty()) {
                        addModel(filename);
                        modelsById.append(filename);
                    }

                    for (const QString &id : modelsById) {
                        updateData(id, FilenameRole, filename);
                        // FIXME: WE should change this to use a consistent filename for online models
                        updateData(id, OnlineRole, filename.startsWith("chatgpt-") || filename.startsWith("nomic-"));
                        updateData(id, DirpathRole, info.dir().absolutePath() + "/");
                        updateData(id, FilesizeRole, toFileSize(info.size()));
                    }
                }
            }
        }
    };

    processDirectory(exePath);
    if (localPath != exePath)
        processDirectory(localPath);
}

#define MODELS_VERSION 2

void ModelList::updateModelsFromJson()
{
#if defined(USE_LOCAL_MODELSJSON)
    QUrl jsonUrl("file://" + QDir::homePath() + QString("/dev/large_language_models/gpt4all/gpt4all-chat/metadata/models%1.json").arg(MODELS_VERSION));
#else
    QUrl jsonUrl(QString("http://gpt4all.io/models/models%1.json").arg(MODELS_VERSION));
#endif
    QNetworkRequest request(jsonUrl);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);
    QNetworkReply *jsonReply = m_networkManager.get(request);
    connect(qApp, &QCoreApplication::aboutToQuit, jsonReply, &QNetworkReply::abort);
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

        QSettings settings;
        QFileInfo info(settings.fileName());
        QString dirPath = info.canonicalPath();
        const QString modelsConfig = dirPath + "/models.json";
        QFile file(modelsConfig);
        if (!file.open(QIODeviceBase::ReadOnly)) {
            qWarning() << "ERROR: Couldn't read models config file: " << modelsConfig;
        } else {
            QByteArray jsonData = file.readAll();
            file.close();
            parseModelsJsonFile(jsonData, false);
        }
    }
    delete jsonReply;
}

void ModelList::updateModelsFromJsonAsync()
{
    m_asyncModelRequestOngoing = true;
    emit asyncModelRequestOngoingChanged();

#if defined(USE_LOCAL_MODELSJSON)
    QUrl jsonUrl("file://" + QDir::homePath() + QString("/dev/large_language_models/gpt4all/gpt4all-chat/metadata/models%1.json").arg(MODELS_VERSION));
#else
    QUrl jsonUrl(QString("http://gpt4all.io/models/models%1.json").arg(MODELS_VERSION));
#endif
    QNetworkRequest request(jsonUrl);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);
    QNetworkReply *jsonReply = m_networkManager.get(request);
    connect(qApp, &QCoreApplication::aboutToQuit, jsonReply, &QNetworkReply::abort);
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

    qWarning() << QString("ERROR: Modellist download failed with error code \"%1-%2\"")
                      .arg(code).arg(reply->errorString()).toStdString();
}

void ModelList::handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    QUrl url = reply->request().url();
    for (const auto &e : errors)
        qWarning() << "ERROR: Received ssl error:" << e.errorString() << "for" << url;
}

void ModelList::updateDataForSettings()
{
    emit dataChanged(index(0, 0), index(m_models.size() - 1, 0));
}

static bool compareVersions(const QString &a, const QString &b) {
    QStringList aParts = a.split('.');
    QStringList bParts = b.split('.');

    for (int i = 0; i < std::min(aParts.size(), bParts.size()); ++i) {
        int aInt = aParts[i].toInt();
        int bInt = bParts[i].toInt();

        if (aInt > bInt) {
            return true;
        } else if (aInt < bInt) {
            return false;
        }
    }

    return aParts.size() > bParts.size();
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
        QSettings settings;
        QFileInfo info(settings.fileName());
        QString dirPath = info.canonicalPath();
        const QString modelsConfig = dirPath + "/models.json";
        QFile file(modelsConfig);
        if (!file.open(QIODeviceBase::WriteOnly)) {
            qWarning() << "ERROR: Couldn't write models config file: " << modelsConfig;
        } else {
            file.write(jsonData.constData());
            file.close();
        }
    }

    QJsonArray jsonArray = document.array();
    const QString currentVersion = QCoreApplication::applicationVersion();

    for (const QJsonValue &value : jsonArray) {
        QJsonObject obj = value.toObject();

        QString modelName = obj["name"].toString();
        QString modelFilename = obj["filename"].toString();
        QString modelFilesize = obj["filesize"].toString();
        QString requiresVersion = obj["requires"].toString();
        QString deprecatedVersion = obj["deprecated"].toString();
        QString url = obj["url"].toString();
        QByteArray modelMd5sum = obj["md5sum"].toString().toLatin1().constData();
        bool isDefault = obj.contains("isDefault") && obj["isDefault"] == QString("true");
        bool disableGUI = obj.contains("disableGUI") && obj["disableGUI"] == QString("true");
        QString description = obj["description"].toString();
        QString order = obj["order"].toString();
        int ramrequired = obj["ramrequired"].toString().toInt();
        QString parameters = obj["parameters"].toString();
        QString quant = obj["quant"].toString();
        QString type = obj["type"].toString();

        // If the currentVersion version is strictly less than required version, then continue
        if (!requiresVersion.isEmpty()
            && requiresVersion != currentVersion
            && compareVersions(requiresVersion, currentVersion)) {
            continue;
        }

        // If the current version is strictly greater than the deprecated version, then continue
        if (!deprecatedVersion.isEmpty()
            && compareVersions(currentVersion, deprecatedVersion)) {
            continue;
        }

        modelFilesize = ModelList::toFileSize(modelFilesize.toULongLong());

        const QString id = modelName;
        Q_ASSERT(!id.isEmpty());

        if (contains(modelFilename))
            changeId(modelFilename, id);

        if (!contains(id))
            addModel(id);

        updateData(id, ModelList::NameRole, modelName);
        updateData(id, ModelList::FilenameRole, modelFilename);
        updateData(id, ModelList::FilesizeRole, modelFilesize);
        updateData(id, ModelList::Md5sumRole, modelMd5sum);
        updateData(id, ModelList::DefaultRole, isDefault);
        updateData(id, ModelList::DescriptionRole, description);
        updateData(id, ModelList::RequiresVersionRole, requiresVersion);
        updateData(id, ModelList::DeprecatedVersionRole, deprecatedVersion);
        updateData(id, ModelList::UrlRole, url);
        updateData(id, ModelList::DisableGUIRole, disableGUI);
        updateData(id, ModelList::OrderRole, order);
        updateData(id, ModelList::RamrequiredRole, ramrequired);
        updateData(id, ModelList::ParametersRole, parameters);
        updateData(id, ModelList::QuantRole, quant);
        updateData(id, ModelList::TypeRole, type);
        if (obj.contains("temperature"))
            updateData(id, ModelList::TemperatureRole, obj["temperature"].toDouble());
        if (obj.contains("topP"))
            updateData(id, ModelList::TopPRole, obj["topP"].toDouble());
        if (obj.contains("topK"))
            updateData(id, ModelList::TopKRole, obj["topK"].toInt());
        if (obj.contains("maxLength"))
            updateData(id, ModelList::MaxLengthRole, obj["maxLength"].toInt());
        if (obj.contains("promptBatchSize"))
            updateData(id, ModelList::PromptBatchSizeRole, obj["promptBatchSize"].toInt());
        if (obj.contains("contextLength"))
            updateData(id, ModelList::ContextLengthRole, obj["contextLength"].toInt());
        if (obj.contains("gpuLayers"))
            updateData(id, ModelList::GpuLayersRole, obj["gpuLayers"].toInt());
        if (obj.contains("repeatPenalty"))
            updateData(id, ModelList::RepeatPenaltyRole, obj["repeatPenalty"].toDouble());
        if (obj.contains("repeatPenaltyTokens"))
            updateData(id, ModelList::RepeatPenaltyTokensRole, obj["repeatPenaltyTokens"].toInt());
        if (obj.contains("promptTemplate"))
            updateData(id, ModelList::PromptTemplateRole, obj["promptTemplate"].toString());
        if (obj.contains("systemPrompt"))
            updateData(id, ModelList::SystemPromptRole, obj["systemPrompt"].toString());
    }

    const QString chatGPTDesc = tr("<ul><li>Requires personal OpenAI API key.</li><li>WARNING: Will send"
        " your chats to OpenAI!</li><li>Your API key will be stored on disk</li><li>Will only be used"
        " to communicate with OpenAI</li><li>You can apply for an API key"
        " <a href=\"https://platform.openai.com/account/api-keys\">here.</a></li>");

    {
        const QString modelName = "ChatGPT-3.5 Turbo";
        const QString id = modelName;
        const QString modelFilename = "chatgpt-gpt-3.5-turbo.txt";
        if (contains(modelFilename))
            changeId(modelFilename, id);
        if (!contains(id))
            addModel(id);
        updateData(id, ModelList::NameRole, modelName);
        updateData(id, ModelList::FilenameRole, modelFilename);
        updateData(id, ModelList::FilesizeRole, "minimal");
        updateData(id, ModelList::OnlineRole, true);
        updateData(id, ModelList::DescriptionRole,
            tr("<strong>OpenAI's ChatGPT model GPT-3.5 Turbo</strong><br>") + chatGPTDesc);
        updateData(id, ModelList::RequiresVersionRole, "2.4.2");
        updateData(id, ModelList::OrderRole, "ca");
        updateData(id, ModelList::RamrequiredRole, 0);
        updateData(id, ModelList::ParametersRole, "?");
        updateData(id, ModelList::QuantRole, "NA");
        updateData(id, ModelList::TypeRole, "GPT");
    }

    {
        const QString chatGPT4Warn = tr("<br><br><i>* Even if you pay OpenAI for ChatGPT-4 this does not guarantee API key access. Contact OpenAI for more info.");

        const QString modelName = "ChatGPT-4";
        const QString id = modelName;
        const QString modelFilename = "chatgpt-gpt-4.txt";
        if (contains(modelFilename))
            changeId(modelFilename, id);
        if (!contains(id))
            addModel(id);
        updateData(id, ModelList::NameRole, modelName);
        updateData(id, ModelList::FilenameRole, modelFilename);
        updateData(id, ModelList::FilesizeRole, "minimal");
        updateData(id, ModelList::OnlineRole, true);
        updateData(id, ModelList::DescriptionRole,
            tr("<strong>OpenAI's ChatGPT model GPT-4</strong><br>") + chatGPTDesc + chatGPT4Warn);
        updateData(id, ModelList::RequiresVersionRole, "2.4.2");
        updateData(id, ModelList::OrderRole, "cb");
        updateData(id, ModelList::RamrequiredRole, 0);
        updateData(id, ModelList::ParametersRole, "?");
        updateData(id, ModelList::QuantRole, "NA");
        updateData(id, ModelList::TypeRole, "GPT");
    }

    {
        const QString nomicEmbedDesc = tr("<ul><li>For use with LocalDocs feature</li>"
            "<li>Used for retrieval augmented generation (RAG)</li>"
            "<li>Requires personal Nomic API key.</li>"
            "<li>WARNING: Will send your localdocs to Nomic Atlas!</li>"
            "<li>You can apply for an API key <a href=\"https://atlas.nomic.ai/\">with Nomic Atlas.</a></li>");
        const QString modelName = "Nomic Embed";
        const QString id = modelName;
        const QString modelFilename = "nomic-embed-text-v1.txt";
        if (contains(modelFilename))
            changeId(modelFilename, id);
        if (!contains(id))
            addModel(id);
        updateData(id, ModelList::NameRole, modelName);
        updateData(id, ModelList::FilenameRole, modelFilename);
        updateData(id, ModelList::FilesizeRole, "minimal");
        updateData(id, ModelList::OnlineRole, true);
        updateData(id, ModelList::DisableGUIRole, true);
        updateData(id, ModelList::DescriptionRole,
            tr("<strong>LocalDocs Nomic Atlas Embed</strong><br>") + nomicEmbedDesc);
        updateData(id, ModelList::RequiresVersionRole, "2.6.3");
        updateData(id, ModelList::OrderRole, "na");
        updateData(id, ModelList::RamrequiredRole, 0);
        updateData(id, ModelList::ParametersRole, "?");
        updateData(id, ModelList::QuantRole, "NA");
        updateData(id, ModelList::TypeRole, "Bert");
    }
}

void ModelList::updateModelsFromSettings()
{
    QSettings settings;
    settings.sync();
    QStringList groups = settings.childGroups();
    for (const QString g : groups) {
        if (!g.startsWith("model-"))
            continue;

        const QString id = g.sliced(6);
        if (contains(id))
            continue;

        if (!settings.contains(g+ "/isClone"))
            continue;

        Q_ASSERT(settings.contains(g + "/name"));
        const QString name = settings.value(g + "/name").toString();
        Q_ASSERT(settings.contains(g + "/filename"));
        const QString filename = settings.value(g + "/filename").toString();
        Q_ASSERT(settings.contains(g + "/temperature"));
        const double temperature = settings.value(g + "/temperature").toDouble();
        Q_ASSERT(settings.contains(g + "/topP"));
        const double topP = settings.value(g + "/topP").toDouble();
        Q_ASSERT(settings.contains(g + "/topK"));
        const int topK = settings.value(g + "/topK").toInt();
        Q_ASSERT(settings.contains(g + "/maxLength"));
        const int maxLength = settings.value(g + "/maxLength").toInt();
        Q_ASSERT(settings.contains(g + "/promptBatchSize"));
        const int promptBatchSize = settings.value(g + "/promptBatchSize").toInt();
        Q_ASSERT(settings.contains(g + "/contextLength"));
        const int contextLength = settings.value(g + "/contextLength").toInt();
        Q_ASSERT(settings.contains(g + "/gpuLayers"));
        const int gpuLayers = settings.value(g + "/gpuLayers").toInt();
        Q_ASSERT(settings.contains(g + "/repeatPenalty"));
        const double repeatPenalty = settings.value(g + "/repeatPenalty").toDouble();
        Q_ASSERT(settings.contains(g + "/repeatPenaltyTokens"));
        const int repeatPenaltyTokens = settings.value(g + "/repeatPenaltyTokens").toInt();
        Q_ASSERT(settings.contains(g + "/promptTemplate"));
        const QString promptTemplate = settings.value(g + "/promptTemplate").toString();
        Q_ASSERT(settings.contains(g + "/systemPrompt"));
        const QString systemPrompt = settings.value(g + "/systemPrompt").toString();

        addModel(id);
        updateData(id, ModelList::IsCloneRole, true);
        updateData(id, ModelList::NameRole, name);
        updateData(id, ModelList::FilenameRole, filename);
        updateData(id, ModelList::TemperatureRole, temperature);
        updateData(id, ModelList::TopPRole, topP);
        updateData(id, ModelList::TopKRole, topK);
        updateData(id, ModelList::MaxLengthRole, maxLength);
        updateData(id, ModelList::PromptBatchSizeRole, promptBatchSize);
        updateData(id, ModelList::ContextLengthRole, contextLength);
        updateData(id, ModelList::GpuLayersRole, gpuLayers);
        updateData(id, ModelList::RepeatPenaltyRole, repeatPenalty);
        updateData(id, ModelList::RepeatPenaltyTokensRole, repeatPenaltyTokens);
        updateData(id, ModelList::PromptTemplateRole, promptTemplate);
        updateData(id, ModelList::SystemPromptRole, systemPrompt);
    }
}
