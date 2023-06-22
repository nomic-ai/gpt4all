#ifndef MODELLIST_H
#define MODELLIST_H

#include <QAbstractListModel>
#include <QtQml>

struct ModelInfo {
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString filename MEMBER filename)
    Q_PROPERTY(QString dirpath MEMBER dirpath)
    Q_PROPERTY(QString filesize MEMBER filesize)
    Q_PROPERTY(QByteArray md5sum MEMBER md5sum)
    Q_PROPERTY(bool calcHash MEMBER calcHash)
    Q_PROPERTY(bool installed MEMBER installed)
    Q_PROPERTY(bool isDefault MEMBER isDefault)
    Q_PROPERTY(bool disableGUI MEMBER disableGUI)
    Q_PROPERTY(bool isChatGPT MEMBER isChatGPT)
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

public:
    QString name;
    QString filename;
    QString dirpath;
    QString filesize;
    QByteArray md5sum;
    bool calcHash = false;
    bool installed = false;
    bool isDefault = false;
    bool isChatGPT = false;
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
    bool operator==(const ModelInfo &other) const {
        return  filename == other.filename;
    }
};
Q_DECLARE_METATYPE(ModelInfo)

class InstalledModels : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit InstalledModels(QObject *parent) : QSortFilterProxyModel(parent) {}

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
    Q_PROPERTY(QString localModelsPath READ localModelsPath WRITE setLocalModelsPath NOTIFY localModelsPathChanged)
    Q_PROPERTY(InstalledModels* installedModels READ installedModels NOTIFY installedModelsChanged)
    Q_PROPERTY(DownloadableModels* downloadableModels READ downloadableModels NOTIFY downloadableModelsChanged)
    Q_PROPERTY(QList<QString> userDefaultModelList READ userDefaultModelList NOTIFY userDefaultModelListChanged)

public:
    static ModelList *globalInstance();

    enum Roles {
        NameRole = Qt::UserRole + 1,
        FilenameRole,
        DirpathRole,
        FilesizeRole,
        Md5sumRole,
        CalcHashRole,
        InstalledRole,
        DefaultRole,
        ChatGPTRole,
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
        TypeRole
    };

    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> roles;
        roles[NameRole] = "name";
        roles[FilenameRole] = "filename";
        roles[DirpathRole] = "dirpath";
        roles[FilesizeRole] = "filesize";
        roles[Md5sumRole] = "md5sum";
        roles[CalcHashRole] = "calcHash";
        roles[InstalledRole] = "installed";
        roles[DefaultRole] = "isDefault";
        roles[ChatGPTRole] = "isChatGPT";
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
        return roles;
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant data(const QString &filename, int role) const;
    void updateData(const QString &filename, int role, const QVariant &value);

    int count() const { return m_models.size(); }

    bool contains(const QString &filename) const;
    Q_INVOKABLE ModelInfo modelInfo(const QString &filename) const;
    ModelInfo defaultModelInfo() const;

    void addModel(const QString &filename);

    Q_INVOKABLE QString defaultLocalModelsPath() const;
    Q_INVOKABLE QString localModelsPath() const;
    Q_INVOKABLE void setLocalModelsPath(const QString &modelPath);

    const QList<ModelInfo> exportModelList() const;
    const QList<QString> userDefaultModelList() const;

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

Q_SIGNALS:
    void countChanged();
    void localModelsPathChanged();
    void installedModelsChanged();
    void downloadableModelsChanged();
    void userDefaultModelListChanged();

private Q_SLOTS:
    void updateModelsFromDirectory();

private:
    QString modelDirPath(const QString &modelName, bool isChatGPT);
    int indexForModel(ModelInfo *model);
    QVariant dataInternal(const ModelInfo *info, int role) const;
    static bool lessThan(const ModelInfo* a, const ModelInfo* b);

private:
    mutable QRecursiveMutex m_mutex;
    InstalledModels *m_installedModels;
    DownloadableModels *m_downloadableModels;
    QList<ModelInfo*> m_models;
    QHash<QString, ModelInfo*> m_modelMap;
    QString m_localModelsPath;
    QFileSystemWatcher *m_watcher;

private:
    explicit ModelList();
    ~ModelList() {}
    friend class MyModelList;
};

#endif // MODELLIST_H
