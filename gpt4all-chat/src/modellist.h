#ifndef MODELLIST_H
#define MODELLIST_H

#include <QAbstractListModel>
#include <QByteArray>
#include <QDateTime>
#include <QHash>
#include <QLatin1StringView>
#include <QList>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QPair>
#include <QQmlEngine>
#include <QSortFilterProxyModel>
#include <QSslError>
#include <QString>
#include <QVariant>
#include <QVector>
#include <Qt>
#include <QtGlobal>

#include <optional>
#include <utility>

using namespace Qt::Literals::StringLiterals;


class UpgradeableSetting {
    Q_GADGET
    QML_ANONYMOUS

    // NOTE: Unset implies there is neither a value nor a default
    enum class State { Unset, Legacy, Modern };

    Q_PROPERTY(bool     isSet     READ isSet   )
    Q_PROPERTY(bool     isLegacy  READ isLegacy)
    Q_PROPERTY(bool     isModern  READ isModern)
    Q_PROPERTY(QVariant value READ value) // string or null

public:
    struct legacy_tag_t { explicit legacy_tag_t() = default; };
    static inline constexpr legacy_tag_t legacy_tag = legacy_tag_t();

    UpgradeableSetting()                           : m_state(State::Unset ) {}
    UpgradeableSetting(legacy_tag_t, QString value): m_state(State::Legacy), m_value(std::move(value)) {}
    UpgradeableSetting(              QString value): m_state(State::Modern), m_value(std::move(value)) {}

    bool     isSet   () const { return m_state != State::Unset;  }
    bool     isLegacy() const { return m_state == State::Legacy; }
    bool     isModern() const { return m_state == State::Modern; }
    QVariant value   () const { return m_state == State::Unset ? QVariant::fromValue(nullptr) : m_value; }

    friend bool operator==(const UpgradeableSetting &a, const UpgradeableSetting &b)
    { return a.m_state == b.m_state && (a.m_state == State::Unset || a.m_value == b.m_value); }

    // returns std::nullopt if there is a legacy template or it is not set
    std::optional<QString> asModern() const
    {
        if (m_state == State::Modern)
            return m_value;
        return std::nullopt;
    }

private:
    State   m_state;
    QString m_value;
};

struct ModelInfo {
    Q_GADGET
    Q_PROPERTY(QString id READ id WRITE setId)
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(QString filename READ filename WRITE setFilename)
    Q_PROPERTY(QString dirpath MEMBER dirpath)
    Q_PROPERTY(QString filesize MEMBER filesize)
    Q_PROPERTY(QByteArray hash MEMBER hash)
    Q_PROPERTY(HashAlgorithm hashAlgorithm MEMBER hashAlgorithm)
    Q_PROPERTY(bool calcHash MEMBER calcHash)
    Q_PROPERTY(bool installed MEMBER installed)
    Q_PROPERTY(bool isDefault MEMBER isDefault)
    Q_PROPERTY(bool isOnline MEMBER isOnline)
    Q_PROPERTY(bool isCompatibleApi MEMBER isCompatibleApi)
    Q_PROPERTY(QString description READ description WRITE setDescription)
    Q_PROPERTY(QString requiresVersion MEMBER requiresVersion)
    Q_PROPERTY(QString versionRemoved MEMBER versionRemoved)
    Q_PROPERTY(QString url READ url WRITE setUrl)
    Q_PROPERTY(qint64 bytesReceived MEMBER bytesReceived)
    Q_PROPERTY(qint64 bytesTotal MEMBER bytesTotal)
    Q_PROPERTY(qint64 timestamp MEMBER timestamp)
    Q_PROPERTY(QString speed MEMBER speed)
    Q_PROPERTY(bool isDownloading MEMBER isDownloading)
    Q_PROPERTY(bool isIncomplete MEMBER isIncomplete)
    Q_PROPERTY(QString downloadError MEMBER downloadError)
    Q_PROPERTY(QString order MEMBER order)
    Q_PROPERTY(int ramrequired MEMBER ramrequired)
    Q_PROPERTY(QString parameters MEMBER parameters)
    Q_PROPERTY(QString quant READ quant WRITE setQuant)
    Q_PROPERTY(QString type READ type WRITE setType)
    Q_PROPERTY(bool isClone READ isClone WRITE setIsClone)
    Q_PROPERTY(bool isDiscovered READ isDiscovered WRITE setIsDiscovered)
    Q_PROPERTY(bool isEmbeddingModel MEMBER isEmbeddingModel)
    Q_PROPERTY(double temperature READ temperature WRITE setTemperature)
    Q_PROPERTY(double topP READ topP WRITE setTopP)
    Q_PROPERTY(double minP READ minP WRITE setMinP)
    Q_PROPERTY(int topK READ topK WRITE setTopK)
    Q_PROPERTY(int maxLength READ maxLength WRITE setMaxLength)
    Q_PROPERTY(int promptBatchSize READ promptBatchSize WRITE setPromptBatchSize)
    Q_PROPERTY(int contextLength READ contextLength WRITE setContextLength)
    Q_PROPERTY(int maxContextLength READ maxContextLength)
    Q_PROPERTY(int gpuLayers READ gpuLayers WRITE setGpuLayers)
    Q_PROPERTY(int maxGpuLayers READ maxGpuLayers)
    Q_PROPERTY(double repeatPenalty READ repeatPenalty WRITE setRepeatPenalty)
    Q_PROPERTY(int repeatPenaltyTokens READ repeatPenaltyTokens WRITE setRepeatPenaltyTokens)
    // user-defined chat template and system message must be written through settings because of their legacy compat
    Q_PROPERTY(QVariant           defaultChatTemplate  READ defaultChatTemplate )
    Q_PROPERTY(UpgradeableSetting chatTemplate         READ chatTemplate        )
    Q_PROPERTY(QString            defaultSystemMessage READ defaultSystemMessage)
    Q_PROPERTY(UpgradeableSetting systemMessage        READ systemMessage       )
    Q_PROPERTY(QString chatNamePrompt READ chatNamePrompt WRITE setChatNamePrompt)
    Q_PROPERTY(QString suggestedFollowUpPrompt READ suggestedFollowUpPrompt WRITE setSuggestedFollowUpPrompt)
    Q_PROPERTY(int likes READ likes WRITE setLikes)
    Q_PROPERTY(int downloads READ downloads WRITE setDownloads)
    Q_PROPERTY(QDateTime recency READ recency WRITE setRecency)

public:
    enum HashAlgorithm {
        Md5,
        Sha256
    };

    QString id() const;
    void setId(const QString &id);

    QString name() const;
    void setName(const QString &name);

    QString filename() const;
    void setFilename(const QString &name);

    QString description() const;
    void setDescription(const QString &d);

    QString url() const;
    void setUrl(const QString &u);

    QString quant() const;
    void setQuant(const QString &q);

    QString type() const;
    void setType(const QString &t);

    bool isClone() const;
    void setIsClone(bool b);

    bool isDiscovered() const;
    void setIsDiscovered(bool b);

    int likes() const;
    void setLikes(int l);

    int downloads() const;
    void setDownloads(int d);

    QDateTime recency() const;
    void setRecency(const QDateTime &r);

    QString dirpath;
    QString filesize;
    QByteArray hash;
    HashAlgorithm hashAlgorithm;
    bool calcHash = false;
    bool installed = false;
    bool isDefault = false;
    // Differences between 'isOnline' and 'isCompatibleApi' in ModelInfo:
    // 'isOnline':
    // - Indicates whether this is a online model.
    // - Linked with the ModelList, fetching info from it.
    bool isOnline = false;
    // 'isCompatibleApi':
    // - Indicates whether the model is using the OpenAI-compatible API which user custom.
    // - When the property is true, 'isOnline' should also be true.
    // - Does not link to the ModelList directly; instead, fetches info from the *-capi.rmodel file and works standalone.
    // - Still needs to copy data from gpt4all.ini and *-capi.rmodel to the ModelList in memory while application getting started(as custom .gguf models do).
    bool isCompatibleApi = false;
    QString requiresVersion;
    QString versionRemoved;
    qint64 bytesReceived = 0;
    qint64 bytesTotal = 0;
    qint64 timestamp = 0;
    QString speed;
    bool isDownloading = false;
    bool isIncomplete = false;
    QString downloadError;
    QString order;
    int ramrequired = -1;
    QString parameters;
    bool isEmbeddingModel = false;
    bool checkedEmbeddingModel = false;

    bool operator==(const ModelInfo &other) const {
        return  m_id == other.m_id;
    }

    double temperature() const;
    void setTemperature(double t);
    double topP() const;
    void setTopP(double p);
    double minP() const;
    void setMinP(double p);
    int topK() const;
    void setTopK(int k);
    int maxLength() const;
    void setMaxLength(int l);
    int promptBatchSize() const;
    void setPromptBatchSize(int s);
    int contextLength() const;
    void setContextLength(int l);
    int maxContextLength() const;
    int gpuLayers() const;
    void setGpuLayers(int l);
    int maxGpuLayers() const;
    double repeatPenalty() const;
    void setRepeatPenalty(double p);
    int repeatPenaltyTokens() const;
    void setRepeatPenaltyTokens(int t);
    QVariant defaultChatTemplate() const;
    UpgradeableSetting chatTemplate() const;
    QString defaultSystemMessage() const;
    UpgradeableSetting systemMessage() const;
    QString chatNamePrompt() const;
    void setChatNamePrompt(const QString &p);
    QString suggestedFollowUpPrompt() const;
    void setSuggestedFollowUpPrompt(const QString &p);

    // Some metadata must be saved to settings because it does not have a meaningful default from some other source.
    // This is useful for fields such as name, description, and URL.
    // It is true for any models that have not been installed from models.json.
    bool shouldSaveMetadata() const;

private:
    QVariant getField(QLatin1StringView name) const;

    QString m_id;
    QString m_name;
    QString m_filename;
    QString m_description;
    QString m_url;
    QString m_quant;
    QString m_type;
    bool    m_isClone                 = false;
    bool    m_isDiscovered            = false;
    int     m_likes                   = -1;
    int     m_downloads               = -1;
    QDateTime m_recency;
    double  m_temperature             = 0.7;
    double  m_topP                    = 0.4;
    double  m_minP                    = 0.0;
    int     m_topK                    = 40;
    int     m_maxLength               = 4096;
    int     m_promptBatchSize         = 128;
    int     m_contextLength           = 2048;
    mutable int m_maxContextLength    = -1;
    int     m_gpuLayers               = 100;
    mutable int m_maxGpuLayers        = -1;
    double  m_repeatPenalty           = 1.18;
    int     m_repeatPenaltyTokens     = 64;
            std::optional<QString> m_chatTemplate;
    mutable std::optional<QString> m_modelChatTemplate;
    QString m_systemMessage;
    QString m_chatNamePrompt          = "Describe the above conversation in seven words or less.";
    QString m_suggestedFollowUpPrompt = "Suggest three very short factual follow-up questions that have not been answered yet or cannot be found inspired by the previous conversation and excerpts.";
    friend class MySettings;
    friend class ModelList;
};
Q_DECLARE_METATYPE(ModelInfo)

class InstalledModels : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    explicit InstalledModels(QObject *parent, bool selectable = false);
    int count() const { return rowCount(); }

Q_SIGNALS:
    void countChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    bool m_selectable;
};

class GPT4AllDownloadableModels : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    explicit GPT4AllDownloadableModels(QObject *parent);
    int count() const;

    Q_INVOKABLE void filter(const QVector<QString> &keywords);

Q_SIGNALS:
    void countChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QVector<QString> m_keywords;
};

class HuggingFaceDownloadableModels : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    explicit HuggingFaceDownloadableModels(QObject *parent);
    int count() const;

    Q_INVOKABLE void discoverAndFilter(const QString &discover);

Q_SIGNALS:
    void countChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    int m_limit;
    QString m_discoverFilter;
};

class ModelList : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(InstalledModels* installedModels READ installedModels NOTIFY installedModelsChanged)
    Q_PROPERTY(InstalledModels* selectableModels READ selectableModels NOTIFY selectableModelsChanged)
    Q_PROPERTY(GPT4AllDownloadableModels* gpt4AllDownloadableModels READ gpt4AllDownloadableModels CONSTANT)
    Q_PROPERTY(HuggingFaceDownloadableModels* huggingFaceDownloadableModels READ huggingFaceDownloadableModels CONSTANT)
    Q_PROPERTY(QList<ModelInfo> selectableModelList READ selectableModelList NOTIFY  selectableModelListChanged)
    Q_PROPERTY(bool asyncModelRequestOngoing READ asyncModelRequestOngoing NOTIFY asyncModelRequestOngoingChanged)
    Q_PROPERTY(int discoverLimit READ discoverLimit WRITE setDiscoverLimit NOTIFY discoverLimitChanged)
    Q_PROPERTY(int discoverSortDirection READ discoverSortDirection WRITE setDiscoverSortDirection NOTIFY discoverSortDirectionChanged)
    Q_PROPERTY(DiscoverSort discoverSort READ discoverSort WRITE setDiscoverSort NOTIFY discoverSortChanged)
    Q_PROPERTY(float discoverProgress READ discoverProgress NOTIFY discoverProgressChanged)
    Q_PROPERTY(bool discoverInProgress READ discoverInProgress NOTIFY discoverInProgressChanged)

public:
    static ModelList *globalInstance();

    static QString compatibleModelNameHash(QUrl baseUrl, QString modelName);
    static QString compatibleModelFilename(QUrl baseUrl, QString modelName);

    enum DiscoverSort {
        Default,
        Likes,
        Downloads,
        Recent
    };

    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        FilenameRole,
        DirpathRole,
        FilesizeRole,
        HashRole,
        HashAlgorithmRole,
        CalcHashRole,
        InstalledRole,
        DefaultRole,
        OnlineRole,
        CompatibleApiRole,
        DescriptionRole,
        RequiresVersionRole,
        VersionRemovedRole,
        UrlRole,
        BytesReceivedRole,
        BytesTotalRole,
        TimestampRole,
        SpeedRole,
        DownloadingRole,
        IncompleteRole,
        DownloadErrorRole,
        OrderRole,
        RamrequiredRole,
        ParametersRole,
        QuantRole,
        TypeRole,
        IsCloneRole,
        IsDiscoveredRole,
        IsEmbeddingModelRole,
        TemperatureRole,
        TopPRole,
        TopKRole,
        MaxLengthRole,
        PromptBatchSizeRole,
        ContextLengthRole,
        GpuLayersRole,
        RepeatPenaltyRole,
        RepeatPenaltyTokensRole,
        ChatTemplateRole,
        SystemMessageRole,
        ChatNamePromptRole,
        SuggestedFollowUpPromptRole,
        MinPRole,
        LikesRole,
        DownloadsRole,
        RecencyRole
    };

    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> roles;
        roles[IdRole] = "id";
        roles[NameRole] = "name";
        roles[FilenameRole] = "filename";
        roles[DirpathRole] = "dirpath";
        roles[FilesizeRole] = "filesize";
        roles[HashRole] = "hash";
        roles[HashAlgorithmRole] = "hashAlgorithm";
        roles[CalcHashRole] = "calcHash";
        roles[InstalledRole] = "installed";
        roles[DefaultRole] = "isDefault";
        roles[OnlineRole] = "isOnline";
        roles[CompatibleApiRole] = "isCompatibleApi";
        roles[DescriptionRole] = "description";
        roles[RequiresVersionRole] = "requiresVersion";
        roles[VersionRemovedRole] = "versionRemoved";
        roles[UrlRole] = "url";
        roles[BytesReceivedRole] = "bytesReceived";
        roles[BytesTotalRole] = "bytesTotal";
        roles[TimestampRole] = "timestamp";
        roles[SpeedRole] = "speed";
        roles[DownloadingRole] = "isDownloading";
        roles[IncompleteRole] = "isIncomplete";
        roles[DownloadErrorRole] = "downloadError";
        roles[OrderRole] = "order";
        roles[RamrequiredRole] = "ramrequired";
        roles[ParametersRole] = "parameters";
        roles[QuantRole] = "quant";
        roles[TypeRole] = "type";
        roles[IsCloneRole] = "isClone";
        roles[IsDiscoveredRole] = "isDiscovered";
        roles[IsEmbeddingModelRole] = "isEmbeddingModel";
        roles[TemperatureRole] = "temperature";
        roles[TopPRole] = "topP";
        roles[MinPRole] = "minP";
        roles[TopKRole] = "topK";
        roles[MaxLengthRole] = "maxLength";
        roles[PromptBatchSizeRole] = "promptBatchSize";
        roles[ContextLengthRole] = "contextLength";
        roles[GpuLayersRole] = "gpuLayers";
        roles[RepeatPenaltyRole] = "repeatPenalty";
        roles[RepeatPenaltyTokensRole] = "repeatPenaltyTokens";
        roles[ChatTemplateRole] = "chatTemplate";
        roles[SystemMessageRole] = "systemMessage";
        roles[ChatNamePromptRole] = "chatNamePrompt";
        roles[SuggestedFollowUpPromptRole] = "suggestedFollowUpPrompt";
        roles[LikesRole] = "likes";
        roles[DownloadsRole] = "downloads";
        roles[RecencyRole] = "recency";
        return roles;
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant data(const QString &id, int role) const;
    QVariant dataByFilename(const QString &filename, int role) const;
    void updateDataByFilename(const QString &filename, QVector<QPair<int, QVariant>> data);
    void updateData(const QString &id, const QVector<QPair<int, QVariant>> &data);

    int count() const { return m_models.size(); }

    bool contains(const QString &id) const;
    bool containsByFilename(const QString &filename) const;
    Q_INVOKABLE ModelInfo modelInfo(const QString &id) const;
    Q_INVOKABLE ModelInfo modelInfoByFilename(const QString &filename, bool allowClone = true) const;
    Q_INVOKABLE bool isUniqueName(const QString &name) const;
    Q_INVOKABLE QString clone(const ModelInfo &model);
    Q_INVOKABLE void removeClone(const ModelInfo &model);
    Q_INVOKABLE void removeInstalled(const ModelInfo &model);
    ModelInfo defaultModelInfo() const;

    void addModel(const QString &id);
    void changeId(const QString &oldId, const QString &newId);

    const QList<ModelInfo> selectableModelList() const;

    InstalledModels *installedModels() const { return m_installedModels; }
    InstalledModels *selectableModels() const { return m_selectableModels; }
    GPT4AllDownloadableModels *gpt4AllDownloadableModels() const { return m_gpt4AllDownloadableModels; }
    HuggingFaceDownloadableModels *huggingFaceDownloadableModels() const { return m_huggingFaceDownloadableModels; }

    static inline QString toFileSize(quint64 sz) {
        if (sz < 1024) {
            return u"%1 bytes"_s.arg(sz);
        } else if (sz < 1024 * 1024) {
            return u"%1 KB"_s.arg(qreal(sz) / 1024, 0, 'g', 3);
        } else if (sz < 1024 * 1024 * 1024) {
            return u"%1 MB"_s.arg(qreal(sz) / (1024 * 1024), 0, 'g', 3);
        } else {
            return u"%1 GB"_s.arg(qreal(sz) / (1024 * 1024 * 1024), 0, 'g', 3);
        }
    }

    QString incompleteDownloadPath(const QString &modelFile);
    bool asyncModelRequestOngoing() const { return m_asyncModelRequestOngoing; }

    void updateModelsFromDirectory();
    void updateDiscoveredInstalled(const ModelInfo &info);

    int discoverLimit() const;
    void setDiscoverLimit(int limit);

    int discoverSortDirection() const;
    void setDiscoverSortDirection(int direction); // -1 or 1

    DiscoverSort discoverSort() const;
    void setDiscoverSort(DiscoverSort sort);

    float discoverProgress() const;
    bool discoverInProgress() const;

    Q_INVOKABLE void discoverSearch(const QString &discover);

Q_SIGNALS:
    void countChanged();
    void installedModelsChanged();
    void selectableModelsChanged();
    void selectableModelListChanged();
    void asyncModelRequestOngoingChanged();
    void discoverLimitChanged();
    void discoverSortDirectionChanged();
    void discoverSortChanged();
    void discoverProgressChanged();
    void discoverInProgressChanged();
    void modelInfoChanged(const ModelInfo &info);

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private Q_SLOTS:
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles);
    void resortModel();
    void updateModelsFromJson();
    void updateModelsFromJsonAsync();
    void updateModelsFromSettings();
    void maybeUpdateDataForSettings(const ModelInfo &info, bool fromInfo);
    void updateDataForSettings();
    void handleModelsJsonDownloadFinished();
    void handleModelsJsonDownloadErrorOccurred(QNetworkReply::NetworkError code);
    void handleDiscoveryFinished();
    void handleDiscoveryErrorOccurred(QNetworkReply::NetworkError code);
    void handleDiscoveryItemFinished();
    void handleDiscoveryItemErrorOccurred(QNetworkReply::NetworkError code);
    void handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);

private:
    // Return the index of the model with the given id, or -1 if not found.
    int indexByModelId(const QString &id) const;

    void removeInternal(const ModelInfo &model);
    void clearDiscoveredModels();
    bool modelExists(const QString &fileName) const;
    int indexForModel(ModelInfo *model);
    QVariant dataInternal(const ModelInfo *info, int role) const;
    static bool lessThan(const ModelInfo* a, const ModelInfo* b, DiscoverSort s, int d);
    void parseModelsJsonFile(const QByteArray &jsonData, bool save);
    void parseDiscoveryJsonFile(const QByteArray &jsonData);
    QString uniqueModelName(const ModelInfo &model) const;
    void updateOldRemoteModels(const QString &path);
    void processModelDirectory(const QString &path);

private:
    mutable QMutex m_mutex;
    QNetworkAccessManager m_networkManager;
    InstalledModels *m_installedModels;
    InstalledModels *m_selectableModels;
    GPT4AllDownloadableModels *m_gpt4AllDownloadableModels;
    HuggingFaceDownloadableModels *m_huggingFaceDownloadableModels;
    QList<ModelInfo*> m_models;
    QHash<QString, ModelInfo*> m_modelMap;
    bool m_asyncModelRequestOngoing;
    int m_discoverLimit;
    int m_discoverSortDirection;
    DiscoverSort m_discoverSort;
    int m_discoverNumberOfResults;
    int m_discoverResultsCompleted;
    bool m_discoverInProgress;

protected:
    explicit ModelList();
    ~ModelList() override { for (auto *model: std::as_const(m_models)) { delete model; } }
    friend class MyModelList;
};

#endif // MODELLIST_H
