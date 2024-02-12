#ifndef MODELLIST_H
#define MODELLIST_H

#include <QAbstractListModel>
#include <QtQml>

struct ModelInfo {
    Q_GADGET
    Q_PROPERTY(QString id READ id WRITE setId)
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(QString filename READ filename WRITE setFilename)
    Q_PROPERTY(QString dirpath MEMBER dirpath)
    Q_PROPERTY(QString filesize MEMBER filesize)
    Q_PROPERTY(QByteArray md5sum MEMBER md5sum)
    Q_PROPERTY(bool calcHash MEMBER calcHash)
    Q_PROPERTY(bool installed MEMBER installed)
    Q_PROPERTY(bool isDefault MEMBER isDefault)
    Q_PROPERTY(bool disableGUI MEMBER disableGUI)
    Q_PROPERTY(bool isOnline MEMBER isOnline)
    Q_PROPERTY(QString description MEMBER description)
    Q_PROPERTY(QString requiresVersion MEMBER requiresVersion)
    Q_PROPERTY(QString deprecatedVersion MEMBER deprecatedVersion)
    Q_PROPERTY(QString url MEMBER url)
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
    Q_PROPERTY(QString quant MEMBER quant)
    Q_PROPERTY(QString type MEMBER type)
    Q_PROPERTY(bool isClone MEMBER isClone)
    Q_PROPERTY(double temperature READ temperature WRITE setTemperature)
    Q_PROPERTY(double topP READ topP WRITE setTopP)
    Q_PROPERTY(int topK READ topK WRITE setTopK)
    Q_PROPERTY(int maxLength READ maxLength WRITE setMaxLength)
    Q_PROPERTY(int promptBatchSize READ promptBatchSize WRITE setPromptBatchSize)
    Q_PROPERTY(int contextLength READ contextLength WRITE setContextLength)
    Q_PROPERTY(int maxContextLength READ maxContextLength)
    Q_PROPERTY(int gpuLayers READ gpuLayers WRITE setGpuLayers)
    Q_PROPERTY(int maxGpuLayers READ maxGpuLayers)
    Q_PROPERTY(double repeatPenalty READ repeatPenalty WRITE setRepeatPenalty)
    Q_PROPERTY(int repeatPenaltyTokens READ repeatPenaltyTokens WRITE setRepeatPenaltyTokens)
    Q_PROPERTY(QString promptTemplate READ promptTemplate WRITE setPromptTemplate)
    Q_PROPERTY(QString systemPrompt READ systemPrompt WRITE setSystemPrompt)

public:
    QString id() const;
    void setId(const QString &id);

    QString name() const;
    void setName(const QString &name);

    QString filename() const;
    void setFilename(const QString &name);

    QString dirpath;
    QString filesize;
    QByteArray md5sum;
    bool calcHash = false;
    bool installed = false;
    bool isDefault = false;
    bool isOnline = false;
    bool disableGUI = false;
    QString description;
    QString requiresVersion;
    QString deprecatedVersion;
    QString url;
    qint64 bytesReceived = 0;
    qint64 bytesTotal = 0;
    qint64 timestamp = 0;
    QString speed;
    bool isDownloading = false;
    bool isIncomplete = false;
    QString downloadError;
    QString order;
    int ramrequired = 0;
    QString parameters;
    QString quant;
    QString type;
    bool isClone = false;

    bool operator==(const ModelInfo &other) const {
        return  m_id == other.m_id;
    }

    double temperature() const;
    void setTemperature(double t);
    double topP() const;
    void setTopP(double p);
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
    QString promptTemplate() const;
    void setPromptTemplate(const QString &t);
    QString systemPrompt() const;
    void setSystemPrompt(const QString &p);

private:
    QString m_id;
    QString m_name;
    QString m_filename;
    double  m_temperature          = 0.7;
    double  m_topP                 = 0.4;
    int     m_topK                 = 40;
    int     m_maxLength            = 4096;
    int     m_promptBatchSize      = 128;
    int     m_contextLength        = 2048;
    mutable int m_maxContextLength = -1;
    int     m_gpuLayers            = 100;
    mutable int m_maxGpuLayers     = -1;
    double  m_repeatPenalty        = 1.18;
    int     m_repeatPenaltyTokens  = 64;
    QString m_promptTemplate       = "### Human:\n%1\n### Assistant:\n";
    QString m_systemPrompt         = "### System:\nYou are an AI assistant who gives a quality response to whatever humans ask of you.\n";
    friend class MySettings;
};
Q_DECLARE_METATYPE(ModelInfo)

class EmbeddingModels : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    explicit EmbeddingModels(QObject *parent);
    int count() const;

    ModelInfo defaultModelInfo() const;

Q_SIGNALS:
    void countChanged();
    void defaultModelIndexChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
};

class InstalledModels : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    explicit InstalledModels(QObject *parent);
    int count() const;

Q_SIGNALS:
    void countChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
};

class DownloadableModels : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool expanded READ isExpanded WRITE setExpanded NOTIFY expandedChanged)
public:
    explicit DownloadableModels(QObject *parent);
    int count() const;

    bool isExpanded() const;
    void setExpanded(bool expanded);

Q_SIGNALS:
    void countChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

Q_SIGNALS:
    void expandedChanged(bool expanded);

private:
    bool m_expanded;
    int m_limit;
};

class ModelList : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int defaultEmbeddingModelIndex READ defaultEmbeddingModelIndex NOTIFY defaultEmbeddingModelIndexChanged)
    Q_PROPERTY(EmbeddingModels* embeddingModels READ embeddingModels NOTIFY embeddingModelsChanged)
    Q_PROPERTY(InstalledModels* installedModels READ installedModels NOTIFY installedModelsChanged)
    Q_PROPERTY(DownloadableModels* downloadableModels READ downloadableModels NOTIFY downloadableModelsChanged)
    Q_PROPERTY(QList<QString> userDefaultModelList READ userDefaultModelList NOTIFY userDefaultModelListChanged)
    Q_PROPERTY(bool asyncModelRequestOngoing READ asyncModelRequestOngoing NOTIFY asyncModelRequestOngoingChanged)

public:
    static ModelList *globalInstance();

    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        FilenameRole,
        DirpathRole,
        FilesizeRole,
        Md5sumRole,
        CalcHashRole,
        InstalledRole,
        DefaultRole,
        OnlineRole,
        DisableGUIRole,
        DescriptionRole,
        RequiresVersionRole,
        DeprecatedVersionRole,
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
        TemperatureRole,
        TopPRole,
        TopKRole,
        MaxLengthRole,
        PromptBatchSizeRole,
        ContextLengthRole,
        GpuLayersRole,
        RepeatPenaltyRole,
        RepeatPenaltyTokensRole,
        PromptTemplateRole,
        SystemPromptRole,
    };

    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> roles;
        roles[IdRole] = "id";
        roles[NameRole] = "name";
        roles[FilenameRole] = "filename";
        roles[DirpathRole] = "dirpath";
        roles[FilesizeRole] = "filesize";
        roles[Md5sumRole] = "md5sum";
        roles[CalcHashRole] = "calcHash";
        roles[InstalledRole] = "installed";
        roles[DefaultRole] = "isDefault";
        roles[OnlineRole] = "isOnline";
        roles[DisableGUIRole] = "disableGUI";
        roles[DescriptionRole] = "description";
        roles[RequiresVersionRole] = "requiresVersion";
        roles[DeprecatedVersionRole] = "deprecatedVersion";
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
        roles[TemperatureRole] = "temperature";
        roles[TopPRole] = "topP";
        roles[TopKRole] = "topK";
        roles[MaxLengthRole] = "maxLength";
        roles[PromptBatchSizeRole] = "promptBatchSize";
        roles[ContextLengthRole] = "contextLength";
        roles[GpuLayersRole] = "gpuLayers";
        roles[RepeatPenaltyRole] = "repeatPenalty";
        roles[RepeatPenaltyTokensRole] = "repeatPenaltyTokens";
        roles[PromptTemplateRole] = "promptTemplate";
        roles[SystemPromptRole] = "systemPrompt";
        return roles;
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant data(const QString &id, int role) const;
    QVariant dataByFilename(const QString &filename, int role) const;
    void updateData(const QString &id, int role, const QVariant &value);
    void updateDataByFilename(const QString &filename, int role, const QVariant &value);

    int count() const { return m_models.size(); }

    bool contains(const QString &id) const;
    bool containsByFilename(const QString &filename) const;
    Q_INVOKABLE ModelInfo modelInfo(const QString &id) const;
    Q_INVOKABLE ModelInfo modelInfoByFilename(const QString &filename) const;
    Q_INVOKABLE bool isUniqueName(const QString &name) const;
    Q_INVOKABLE QString clone(const ModelInfo &model);
    Q_INVOKABLE void remove(const ModelInfo &model);
    ModelInfo defaultModelInfo() const;
    int defaultEmbeddingModelIndex() const;

    void addModel(const QString &id);
    void changeId(const QString &oldId, const QString &newId);

    const QList<ModelInfo> exportModelList() const;
    const QList<QString> userDefaultModelList() const;

    EmbeddingModels *embeddingModels() const { return m_embeddingModels; }
    InstalledModels *installedModels() const { return m_installedModels; }
    DownloadableModels *downloadableModels() const { return m_downloadableModels; }

    static inline QString toFileSize(quint64 sz) {
        if (sz < 1024) {
            return QString("%1 bytes").arg(sz);
        } else if (sz < 1024 * 1024) {
            return QString("%1 KB").arg(qreal(sz) / 1024, 0, 'g', 3);
        } else if (sz < 1024 * 1024 * 1024) {
            return  QString("%1 MB").arg(qreal(sz) / (1024 * 1024), 0, 'g', 3);
        } else {
            return QString("%1 GB").arg(qreal(sz) / (1024 * 1024 * 1024), 0, 'g', 3);
        }
    }

    QString incompleteDownloadPath(const QString &modelFile);
    bool asyncModelRequestOngoing() const { return m_asyncModelRequestOngoing; }

    void updateModelsFromDirectory();

Q_SIGNALS:
    void countChanged();
    void embeddingModelsChanged();
    void installedModelsChanged();
    void downloadableModelsChanged();
    void userDefaultModelListChanged();
    void asyncModelRequestOngoingChanged();
    void defaultEmbeddingModelIndexChanged();

private Q_SLOTS:
    void updateModelsFromJson();
    void updateModelsFromJsonAsync();
    void updateModelsFromSettings();
    void updateDataForSettings();
    void handleModelsJsonDownloadFinished();
    void handleModelsJsonDownloadErrorOccurred(QNetworkReply::NetworkError code);
    void handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);

private:
    QString modelDirPath(const QString &modelName, bool isOnline);
    int indexForModel(ModelInfo *model);
    QVariant dataInternal(const ModelInfo *info, int role) const;
    static bool lessThan(const ModelInfo* a, const ModelInfo* b);
    void parseModelsJsonFile(const QByteArray &jsonData, bool save);
    QString uniqueModelName(const ModelInfo &model) const;

private:
    mutable QMutex m_mutex;
    QNetworkAccessManager m_networkManager;
    EmbeddingModels *m_embeddingModels;
    InstalledModels *m_installedModels;
    DownloadableModels *m_downloadableModels;
    QList<ModelInfo*> m_models;
    QHash<QString, ModelInfo*> m_modelMap;
    bool m_asyncModelRequestOngoing;

private:
    explicit ModelList();
    ~ModelList() {}
    friend class MyModelList;
};

#endif // MODELLIST_H
