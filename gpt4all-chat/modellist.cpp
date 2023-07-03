#include "modellist.h"
#include "mysettings.h"

#include <algorithm>

//#define USE_LOCAL_MODELSJSON

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
    return isInstalled;
}

int InstalledModels::count() const
{
    return rowCount();
}

QString InstalledModels::firstFilename() const
{
    if (rowCount() > 0) {
        QModelIndex firstIndex = index(0, 0);
        return sourceModel()->data(firstIndex, ModelList::FilenameRole).toString();
    } else {
        return QString();
    }
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
    bool showInGUI = !sourceModel()->data(index, ModelList::DisableGUIRole).toBool();
    return withinLimit && isDownloadable && showInGUI;
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
    , m_installedModels(new InstalledModels(this))
    , m_downloadableModels(new DownloadableModels(this))
{
    m_installedModels->setSourceModel(this);
    m_downloadableModels->setSourceModel(this);
    m_watcher = new QFileSystemWatcher(this);
    const QString exePath = QCoreApplication::applicationDirPath() + QDir::separator();
    m_watcher->addPath(exePath);
    m_watcher->addPath(MySettings::globalInstance()->modelPath());
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &ModelList::updateModelsFromDirectory);
    connect(MySettings::globalInstance(), &MySettings::modelPathChanged, this, &ModelList::updateModelList);
    updateModelsFromDirectory();
    updateModelList();
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
        if (info->installed && (info->name == userDefaultModelName || info->filename == userDefaultModelName)) {
            foundUserDefault = true;
            models.prepend(info->name.isEmpty() ? info->filename : info->name);
        } else if (info->installed) {
            models.append(info->name.isEmpty() ? info->filename : info->name);
        }
    }

    const QString defaultFileName = "Application default";
    if (foundUserDefault)
        models.append(defaultFileName);
    else
        models.prepend(defaultFileName);
    return models;
}

ModelInfo ModelList::defaultModelInfo() const
{
    QMutexLocker locker(&m_mutex);

    QSettings settings;
    settings.sync();

    // The user default model can be set by the user in the settings dialog. The "default" user
    // default model is "Application default" which signals we should use the default model that was
    // specified by the models.json file.
    const QString userDefaultModelName = MySettings::globalInstance()->userDefaultModel();
    const bool hasUserDefaultName = !userDefaultModelName.isEmpty() && userDefaultModelName != "Application default";
    const QString defaultModelName = settings.value("defaultModel").toString();
    const bool hasDefaultName = hasUserDefaultName ? false : !defaultModelName.isEmpty();

    ModelInfo *defaultModel = nullptr;
    for (ModelInfo *info : m_models) {
        if (!info->installed)
            continue;
        defaultModel = info;

        // If we don't have either setting, then just use the first model that is installed
        if (!hasUserDefaultName && !hasDefaultName)
            break;

        // If we don't have a user specified default, but *do* have a default setting and match, then use it
        if (!hasUserDefaultName && hasDefaultName && (defaultModel->name == defaultModelName || defaultModel->filename == defaultModelName))
            break;

        // If we have a user specified default and match, then use it
        if (hasUserDefaultName && (defaultModel->name == userDefaultModelName || defaultModel->filename == userDefaultModelName))
            break;
    }
    if (defaultModel)
        return *defaultModel;
    return ModelInfo();
}

bool ModelList::contains(const QString &filename) const
{
    QMutexLocker locker(&m_mutex);
    return m_modelMap.contains(filename);
}

bool ModelList::lessThan(const ModelInfo* a, const ModelInfo* b)
{
    // Rule 1: Non-empty 'order' before empty
    if (a->order.isEmpty() != b->order.isEmpty()) {
        return !a->order.isEmpty();
    }

    // Rule 2: Both 'order' are non-empty, sort alphanumerically
    if (!a->order.isEmpty() && !b->order.isEmpty()) {
        return a->order < b->order;
    }

    // Rule 3: Both 'order' are empty, sort by filename
    return a->filename < b->filename;
}

void ModelList::addModel(const QString &filename)
{
    const bool hasModel = contains(filename);
    Q_ASSERT(!hasModel);
    if (hasModel) {
        qWarning() << "ERROR: model list already contains" << filename;
        return;
    }

    beginInsertRows(QModelIndex(), m_models.size(), m_models.size());
    int modelSizeAfter = 0;
    {
        QMutexLocker locker(&m_mutex);
        ModelInfo *info = new ModelInfo;
        info->filename = filename;
        m_models.append(info);
        m_modelMap.insert(filename, info);
        std::stable_sort(m_models.begin(), m_models.end(), ModelList::lessThan);
        modelSizeAfter = m_models.size();
    }
    endInsertRows();
    emit dataChanged(index(0, 0), index(modelSizeAfter - 1, 0));
    emit userDefaultModelListChanged();
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
        case NameRole:
            return info->name;
        case FilenameRole:
            return info->filename;
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
        case ChatGPTRole:
            return info->isChatGPT;
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
    }

    return QVariant();
}

QVariant ModelList::data(const QString &filename, int role) const
{
    QMutexLocker locker(&m_mutex);
    ModelInfo *info = m_modelMap.value(filename);
    return dataInternal(info, role);
}

QVariant ModelList::data(const QModelIndex &index, int role) const
{
    QMutexLocker locker(&m_mutex);
    if (!index.isValid() || index.row() < 0 || index.row() >= m_models.size())
        return QVariant();
    const ModelInfo *info = m_models.at(index.row());
    return dataInternal(info, role);
}

void ModelList::updateData(const QString &filename, int role, const QVariant &value)
{
    int modelSize;
    bool updateInstalled;
    bool updateIncomplete;
    int index;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_modelMap.contains(filename)) {
            qWarning() << "ERROR: cannot update as model map does not contain" << filename;
            return;
        }

        ModelInfo *info = m_modelMap.value(filename);
        index = m_models.indexOf(info);
        if (index == -1) {
            qWarning() << "ERROR: cannot update as model list does not contain" << filename;
            return;
        }

        switch (role) {
        case NameRole:
            info->name = value.toString(); break;
        case FilenameRole:
            info->filename = value.toString(); break;
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
        case ChatGPTRole:
            info->isChatGPT = value.toBool(); break;
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
        }

        // Extra guarantee that these always remains in sync with filesystem
        QFileInfo fileInfo(info->dirpath + info->filename);
        if (info->installed != fileInfo.exists()) {
            info->installed = fileInfo.exists();
            updateInstalled = true;
        }
        QFileInfo incompleteInfo(incompleteDownloadPath(info->filename));
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

ModelInfo ModelList::modelInfo(const QString &filename) const
{
    QMutexLocker locker(&m_mutex);
    if (!m_modelMap.contains(filename))
        return ModelInfo();
    return *m_modelMap.value(filename);
}

QString ModelList::modelDirPath(const QString &modelName, bool isChatGPT)
{
    QVector<QString> possibleFilePaths;
    if (isChatGPT)
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
                if ((filename.endsWith(".bin") && filename.contains("ggml") && !filename.startsWith("incomplete"))
                    || (filename.endsWith(".txt") && filename.startsWith("chatgpt-"))) {

                    QString filePath = it.filePath();
                    QFileInfo info(filePath);

                    if (!info.exists())
                        continue;

                    if (!contains(filename))
                        addModel(filename);

                    updateData(filename, ChatGPTRole, filename.startsWith("chatgpt-"));
                    updateData(filename, DirpathRole, path);
                    updateData(filename, FilesizeRole, toFileSize(info.size()));
                }
            }
        }
    };

    processDirectory(exePath);
    if (localPath != exePath)
        processDirectory(localPath);
}

void ModelList::updateModelList()
{
#if defined(USE_LOCAL_MODELSJSON)
    QUrl jsonUrl("file://" + QDir::homePath() + "/dev/large_language_models/gpt4all/gpt4all-chat/metadata/models.json");
#else
    QUrl jsonUrl("http://gpt4all.io/models/models.json");
#endif
    QNetworkRequest request(jsonUrl);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);
    QNetworkReply *jsonReply = m_networkManager.get(request);
    QEventLoop loop;
    connect(jsonReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(1500, &loop, &QEventLoop::quit);
    loop.exec();
    if (jsonReply->error() == QNetworkReply::NoError && jsonReply->isFinished()) {
        QByteArray jsonData = jsonReply->readAll();
        jsonReply->deleteLater();
        parseModelsJsonFile(jsonData);
    } else {
        qWarning() << "Could not download models.json";
    }
    delete jsonReply;
}

static bool operator==(const ModelInfo& lhs, const ModelInfo& rhs) {
    return lhs.filename == rhs.filename && lhs.md5sum == rhs.md5sum;
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

void ModelList::parseModelsJsonFile(const QByteArray &jsonData)
{
    QJsonParseError err;
    QJsonDocument document = QJsonDocument::fromJson(jsonData, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "ERROR: Couldn't parse: " << jsonData << err.errorString();
        return;
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

        if (!contains(modelFilename))
            addModel(modelFilename);

        if (!modelName.isEmpty())
            updateData(modelFilename, ModelList::NameRole, modelName);
        updateData(modelFilename, ModelList::FilesizeRole, modelFilesize);
        updateData(modelFilename, ModelList::Md5sumRole, modelMd5sum);
        updateData(modelFilename, ModelList::DefaultRole, isDefault);
        updateData(modelFilename, ModelList::DescriptionRole, description);
        updateData(modelFilename, ModelList::RequiresVersionRole, requiresVersion);
        updateData(modelFilename, ModelList::DeprecatedVersionRole, deprecatedVersion);
        updateData(modelFilename, ModelList::UrlRole, url);
        updateData(modelFilename, ModelList::DisableGUIRole, disableGUI);
        updateData(modelFilename, ModelList::OrderRole, order);
        updateData(modelFilename, ModelList::RamrequiredRole, ramrequired);
        updateData(modelFilename, ModelList::ParametersRole, parameters);
        updateData(modelFilename, ModelList::QuantRole, quant);
        updateData(modelFilename, ModelList::TypeRole, type);
    }

    const QString chatGPTDesc = tr("<ul><li>Requires personal OpenAI API key.</li><li>WARNING: Will send"
        " your chats to OpenAI!</li><li>Your API key will be stored on disk</li><li>Will only be used"
        " to communicate with OpenAI</li><li>You can apply for an API key"
        " <a href=\"https://platform.openai.com/account/api-keys\">here.</a></li>");

    {
        const QString modelFilename = "chatgpt-gpt-3.5-turbo.txt";
        if (!contains(modelFilename))
            addModel(modelFilename);
        updateData(modelFilename, ModelList::NameRole, "ChatGPT-3.5 Turbo");
        updateData(modelFilename, ModelList::FilesizeRole, "minimal");
        updateData(modelFilename, ModelList::ChatGPTRole, true);
        updateData(modelFilename, ModelList::DescriptionRole,
            tr("<strong>OpenAI's ChatGPT model GPT-3.5 Turbo</strong><br>") + chatGPTDesc);
        updateData(modelFilename, ModelList::RequiresVersionRole, "2.4.2");
        updateData(modelFilename, ModelList::OrderRole, "ca");
        updateData(modelFilename, ModelList::RamrequiredRole, 0);
        updateData(modelFilename, ModelList::ParametersRole, "?");
        updateData(modelFilename, ModelList::QuantRole, "NA");
        updateData(modelFilename, ModelList::TypeRole, "GPT");
    }

    {
        const QString modelFilename = "chatgpt-gpt-4.txt";
        if (!contains(modelFilename))
            addModel(modelFilename);
        updateData(modelFilename, ModelList::NameRole, "ChatGPT-4");
        updateData(modelFilename, ModelList::FilesizeRole, "minimal");
        updateData(modelFilename, ModelList::ChatGPTRole, true);
        updateData(modelFilename, ModelList::DescriptionRole,
            tr("<strong>OpenAI's ChatGPT model GPT-4</strong><br>") + chatGPTDesc);
        updateData(modelFilename, ModelList::RequiresVersionRole, "2.4.2");
        updateData(modelFilename, ModelList::OrderRole, "cb");
        updateData(modelFilename, ModelList::RamrequiredRole, 0);
        updateData(modelFilename, ModelList::ParametersRole, "?");
        updateData(modelFilename, ModelList::QuantRole, "NA");
        updateData(modelFilename, ModelList::TypeRole, "GPT");
    }

    if (installedModels()->count()) {
        const QString firstModel =
            installedModels()->firstFilename();
        QSettings settings;
        settings.setValue("defaultModel", firstModel);
        settings.sync();
    }
}
