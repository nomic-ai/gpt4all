#include "mysettings.h"
#include "modellist.h"
#include "../gpt4all-backend/llmodel.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

static int      default_threadCount         = std::min(4, (int32_t) std::thread::hardware_concurrency());
static bool     default_saveChatsContext    = false;
static bool     default_serverChat          = false;
static QString  default_userDefaultModel    = "Application default";
static bool     default_forceMetal          = false;
static QString  default_lastVersionStarted  = "";
static int      default_localDocsChunkSize  = 256;
static QString  default_chatTheme           = "Dark";
static QString  default_fontSize            = "Small";
static int      default_localDocsRetrievalSize  = 3;
static bool     default_localDocsShowReferences = true;
static QString  default_networkAttribution      = "";
static bool     default_networkIsActive         = false;
static bool     default_networkUsageStatsActive = false;
static QString  default_device              = "Auto";

static QString defaultLocalModelsPath()
{
    QString localPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        + "/";
    QString testWritePath = localPath + QString("test_write.txt");
    QString canonicalLocalPath = QFileInfo(localPath).canonicalFilePath() + "/";
    QDir localDir(localPath);
    if (!localDir.exists()) {
        if (!localDir.mkpath(localPath)) {
            qWarning() << "ERROR: Local download directory can't be created:" << canonicalLocalPath;
            return canonicalLocalPath;
        }
    }

    if (QFileInfo::exists(testWritePath))
        return canonicalLocalPath;

    QFile testWriteFile(testWritePath);
    if (testWriteFile.open(QIODeviceBase::ReadWrite)) {
        testWriteFile.close();
        return canonicalLocalPath;
    }

    qWarning() << "ERROR: Local download path appears not writeable:" << canonicalLocalPath;
    return canonicalLocalPath;
}

class MyPrivateSettings: public MySettings { };
Q_GLOBAL_STATIC(MyPrivateSettings, settingsInstance)
MySettings *MySettings::globalInstance()
{
    return settingsInstance();
}

MySettings::MySettings()
    : QObject{nullptr}
{
    QSettings::setDefaultFormat(QSettings::IniFormat);

    std::vector<LLModel::GPUDevice> devices = LLModel::Implementation::availableGPUDevices();
    QVector<QString> deviceList{ "Auto" };
    for (LLModel::GPUDevice &d : devices)
        deviceList << QString::fromStdString(d.name);
    deviceList << "CPU";
    setDeviceList(deviceList);
}

Q_INVOKABLE QVector<QString> MySettings::deviceList() const
{
    return m_deviceList;
}

void MySettings::setDeviceList(const QVector<QString> &deviceList)
{
    m_deviceList = deviceList;
    emit deviceListChanged();
}

void MySettings::restoreModelDefaults(const ModelInfo &model)
{
    setModelTemperature(model, model.m_temperature);
    setModelTopP(model, model.m_topP);
    setModelTopK(model, model.m_topK);;
    setModelMaxLength(model, model.m_maxLength);
    setModelPromptBatchSize(model, model.m_promptBatchSize);
    setModelContextLength(model, model.m_contextLength);
    setModelGpuLayers(model, model.m_gpuLayers);
    setModelRepeatPenalty(model, model.m_repeatPenalty);
    setModelRepeatPenaltyTokens(model, model.m_repeatPenaltyTokens);
    setModelPromptTemplate(model, model.m_promptTemplate);
    setModelSystemPrompt(model, model.m_systemPrompt);
}

void MySettings::restoreApplicationDefaults()
{
    setChatTheme(default_chatTheme);
    setFontSize(default_fontSize);
    setDevice(default_device);
    setThreadCount(default_threadCount);
    setSaveChatsContext(default_saveChatsContext);
    setServerChat(default_serverChat);
    setModelPath(defaultLocalModelsPath());
    setUserDefaultModel(default_userDefaultModel);
    setForceMetal(default_forceMetal);
}

void MySettings::restoreLocalDocsDefaults()
{
    setLocalDocsChunkSize(default_localDocsChunkSize);
    setLocalDocsRetrievalSize(default_localDocsRetrievalSize);
    setLocalDocsShowReferences(default_localDocsShowReferences);
}

void MySettings::eraseModel(const ModelInfo &m)
{
    QSettings settings;
    settings.remove(QString("model-%1").arg(m.id()));
    settings.sync();
}

QString MySettings::modelName(const ModelInfo &m) const
{
    QSettings setting;
    setting.sync();
    return setting.value(QString("model-%1").arg(m.id()) + "/name",
        !m.m_name.isEmpty() ? m.m_name : m.m_filename).toString();
}

void MySettings::setModelName(const ModelInfo &m, const QString &name, bool force)
{
    if ((modelName(m) == name || m.id().isEmpty()) && !force)
        return;

    QSettings setting;
    if ((m.m_name == name || m.m_filename == name) && !m.isClone)
        setting.remove(QString("model-%1").arg(m.id()) + "/name");
    else
        setting.setValue(QString("model-%1").arg(m.id()) + "/name", name);
    if (m.isClone)
        setting.setValue(QString("model-%1").arg(m.id()) + "/isClone", "true");
    setting.sync();
    if (!force)
        emit nameChanged(m);
}

QString MySettings::modelFilename(const ModelInfo &m) const
{
    QSettings setting;
    setting.sync();
    return setting.value(QString("model-%1").arg(m.id()) + "/filename", m.m_filename).toString();
}

void MySettings::setModelFilename(const ModelInfo &m, const QString &filename, bool force)
{
    if ((modelFilename(m) == filename || m.id().isEmpty()) && !force)
        return;

    QSettings setting;
    if (m.m_filename == filename && !m.isClone)
        setting.remove(QString("model-%1").arg(m.id()) + "/filename");
    else
        setting.setValue(QString("model-%1").arg(m.id()) + "/filename", filename);
    setting.sync();
    if (!force)
        emit filenameChanged(m);
}

double MySettings::modelTemperature(const ModelInfo &m) const
{
    QSettings setting;
    setting.sync();
    return setting.value(QString("model-%1").arg(m.id()) + "/temperature", m.m_temperature).toDouble();
}

void MySettings::setModelTemperature(const ModelInfo &m, double t, bool force)
{
    if (modelTemperature(m) == t && !force)
        return;

    QSettings setting;
    if (m.m_temperature == t && !m.isClone)
        setting.remove(QString("model-%1").arg(m.id()) + "/temperature");
    else
        setting.setValue(QString("model-%1").arg(m.id()) + "/temperature", t);
    setting.sync();
    if (!force)
        emit temperatureChanged(m);
}

double MySettings::modelTopP(const ModelInfo &m) const
{
    QSettings setting;
    setting.sync();
    return setting.value(QString("model-%1").arg(m.id()) + "/topP", m.m_topP).toDouble();
}

void MySettings::setModelTopP(const ModelInfo &m, double p, bool force)
{
    if (modelTopP(m) == p && !force)
        return;

    QSettings setting;
    if (m.m_topP == p && !m.isClone)
        setting.remove(QString("model-%1").arg(m.id()) + "/topP");
    else
        setting.setValue(QString("model-%1").arg(m.id()) + "/topP", p);
    setting.sync();
    if (!force)
        emit topPChanged(m);
}

int MySettings::modelTopK(const ModelInfo &m) const
{
    QSettings setting;
    setting.sync();
    return setting.value(QString("model-%1").arg(m.id()) + "/topK", m.m_topK).toInt();
}

void MySettings::setModelTopK(const ModelInfo &m, int k, bool force)
{
    if (modelTopK(m) == k && !force)
        return;

    QSettings setting;
    if (m.m_topK == k && !m.isClone)
        setting.remove(QString("model-%1").arg(m.id()) + "/topK");
    else
        setting.setValue(QString("model-%1").arg(m.id()) + "/topK", k);
    setting.sync();
    if (!force)
        emit topKChanged(m);
}

int MySettings::modelMaxLength(const ModelInfo &m) const
{
    QSettings setting;
    setting.sync();
    return setting.value(QString("model-%1").arg(m.id()) + "/maxLength", m.m_maxLength).toInt();
}

void MySettings::setModelMaxLength(const ModelInfo &m, int l, bool force)
{
    if (modelMaxLength(m) == l && !force)
        return;

    QSettings setting;
    if (m.m_maxLength == l && !m.isClone)
        setting.remove(QString("model-%1").arg(m.id()) + "/maxLength");
    else
        setting.setValue(QString("model-%1").arg(m.id()) + "/maxLength", l);
    setting.sync();
    if (!force)
        emit maxLengthChanged(m);
}

int MySettings::modelPromptBatchSize(const ModelInfo &m) const
{
    QSettings setting;
    setting.sync();
    return setting.value(QString("model-%1").arg(m.id()) + "/promptBatchSize", m.m_promptBatchSize).toInt();
}

void MySettings::setModelPromptBatchSize(const ModelInfo &m, int s, bool force)
{
    if (modelPromptBatchSize(m) == s && !force)
        return;

    QSettings setting;
    if (m.m_promptBatchSize == s && !m.isClone)
        setting.remove(QString("model-%1").arg(m.id()) + "/promptBatchSize");
    else
        setting.setValue(QString("model-%1").arg(m.id()) + "/promptBatchSize", s);
    setting.sync();
    if (!force)
        emit promptBatchSizeChanged(m);
}

int MySettings::modelContextLength(const ModelInfo &m) const
{
    QSettings setting;
    setting.sync();
    return setting.value(QString("model-%1").arg(m.id()) + "/contextLength", m.m_contextLength).toInt();
}

void MySettings::setModelContextLength(const ModelInfo &m, int l, bool force)
{
    if (modelContextLength(m) == l && !force)
        return;

    QSettings setting;
    if (m.m_contextLength == l && !m.isClone)
        setting.remove(QString("model-%1").arg(m.id()) + "/contextLength");
    else
        setting.setValue(QString("model-%1").arg(m.id()) + "/contextLength", l);
    setting.sync();
    if (!force)
        emit contextLengthChanged(m);
}

int MySettings::modelGpuLayers(const ModelInfo &m) const
{
    QSettings setting;
    setting.sync();
    return setting.value(QString("model-%1").arg(m.id()) + "/gpuLayers", m.m_gpuLayers).toInt();
}

void MySettings::setModelGpuLayers(const ModelInfo &m, int l, bool force)
{
    if (modelGpuLayers(m) == l && !force)
        return;

    QSettings setting;
    if (m.m_gpuLayers == l && !m.isClone)
        setting.remove(QString("model-%1").arg(m.id()) + "/gpuLayers");
    else
        setting.setValue(QString("model-%1").arg(m.id()) + "/gpuLayers", l);
    setting.sync();
    if (!force)
        emit gpuLayersChanged(m);
}

double MySettings::modelRepeatPenalty(const ModelInfo &m) const
{
    QSettings setting;
    setting.sync();
    return setting.value(QString("model-%1").arg(m.id()) + "/repeatPenalty", m.m_repeatPenalty).toDouble();
}

void MySettings::setModelRepeatPenalty(const ModelInfo &m, double p, bool force)
{
    if (modelRepeatPenalty(m) == p && !force)
        return;

    QSettings setting;
    if (m.m_repeatPenalty == p && !m.isClone)
        setting.remove(QString("model-%1").arg(m.id()) + "/repeatPenalty");
    else
        setting.setValue(QString("model-%1").arg(m.id()) + "/repeatPenalty", p);
    setting.sync();
    if (!force)
        emit repeatPenaltyChanged(m);
}

int MySettings::modelRepeatPenaltyTokens(const ModelInfo &m) const
{
    QSettings setting;
    setting.sync();
    return setting.value(QString("model-%1").arg(m.id()) + "/repeatPenaltyTokens", m.m_repeatPenaltyTokens).toInt();
}

void MySettings::setModelRepeatPenaltyTokens(const ModelInfo &m, int t, bool force)
{
    if (modelRepeatPenaltyTokens(m) == t && !force)
        return;

    QSettings setting;
    if (m.m_repeatPenaltyTokens == t && !m.isClone)
        setting.remove(QString("model-%1").arg(m.id()) + "/repeatPenaltyTokens");
    else
        setting.setValue(QString("model-%1").arg(m.id()) + "/repeatPenaltyTokens", t);
    setting.sync();
    if (!force)
        emit repeatPenaltyTokensChanged(m);
}

QString MySettings::modelPromptTemplate(const ModelInfo &m) const
{
    QSettings setting;
    setting.sync();
    return setting.value(QString("model-%1").arg(m.id()) + "/promptTemplate", m.m_promptTemplate).toString();
}

void MySettings::setModelPromptTemplate(const ModelInfo &m, const QString &t, bool force)
{
    if (modelPromptTemplate(m) == t && !force)
        return;

    QSettings setting;
    if (m.m_promptTemplate == t && !m.isClone)
        setting.remove(QString("model-%1").arg(m.id()) + "/promptTemplate");
    else
        setting.setValue(QString("model-%1").arg(m.id()) + "/promptTemplate", t);
    setting.sync();
    if (!force)
        emit promptTemplateChanged(m);
}

QString MySettings::modelSystemPrompt(const ModelInfo &m) const
{
    QSettings setting;
    setting.sync();
    return setting.value(QString("model-%1").arg(m.id()) + "/systemPrompt", m.m_systemPrompt).toString();
}

void MySettings::setModelSystemPrompt(const ModelInfo &m, const QString &p, bool force)
{
    if (modelSystemPrompt(m) == p && !force)
        return;

    QSettings setting;
    if (m.m_systemPrompt == p && !m.isClone)
        setting.remove(QString("model-%1").arg(m.id()) + "/systemPrompt");
    else
        setting.setValue(QString("model-%1").arg(m.id()) + "/systemPrompt", p);
    setting.sync();
    if (!force)
        emit systemPromptChanged(m);
}

int MySettings::threadCount() const
{
    QSettings setting;
    setting.sync();
    int c = setting.value("threadCount", default_threadCount).toInt();
    // The old thread setting likely left many people with 0 in settings config file, which means
    // we should reset it to the default going forward
    if (c <= 0)
        c = default_threadCount;
    c = std::max(c, 1);
    c = std::min(c, QThread::idealThreadCount());
    return c;
}

void MySettings::setThreadCount(int c)
{
    if (threadCount() == c)
        return;

    c = std::max(c, 1);
    c = std::min(c, QThread::idealThreadCount());
    QSettings setting;
    setting.setValue("threadCount", c);
    setting.sync();
    emit threadCountChanged();
}

bool MySettings::saveChatsContext() const
{
    QSettings setting;
    setting.sync();
    return setting.value("saveChatsContext", default_saveChatsContext).toBool();
}

void MySettings::setSaveChatsContext(bool b)
{
    if (saveChatsContext() == b)
        return;

    QSettings setting;
    setting.setValue("saveChatsContext", b);
    setting.sync();
    emit saveChatsContextChanged();
}

bool MySettings::serverChat() const
{
    QSettings setting;
    setting.sync();
    return setting.value("serverChat", default_serverChat).toBool();
}

void MySettings::setServerChat(bool b)
{
    if (serverChat() == b)
        return;

    QSettings setting;
    setting.setValue("serverChat", b);
    setting.sync();
    emit serverChatChanged();
}

QString MySettings::modelPath() const
{
    QSettings setting;
    setting.sync();
    // We have to migrate the old setting because I changed the setting key recklessly in v2.4.11
    // which broke a lot of existing installs
    const bool containsOldSetting = setting.contains("modelPaths");
    if (containsOldSetting) {
        const bool containsNewSetting = setting.contains("modelPath");
        if (!containsNewSetting)
            setting.setValue("modelPath", setting.value("modelPaths"));
        setting.remove("modelPaths");
        setting.sync();
    }
    return setting.value("modelPath", defaultLocalModelsPath()).toString();
}

void MySettings::setModelPath(const QString &p)
{
    QString filePath = (p.startsWith("file://") ?
                        QUrl(p).toLocalFile() : p);
    QString canonical = QFileInfo(filePath).canonicalFilePath() + "/";
    if (modelPath() == canonical)
        return;
    QSettings setting;
    setting.setValue("modelPath", canonical);
    setting.sync();
    emit modelPathChanged();
}

QString MySettings::userDefaultModel() const
{
    QSettings setting;
    setting.sync();
    return setting.value("userDefaultModel", default_userDefaultModel).toString();
}

void MySettings::setUserDefaultModel(const QString &u)
{
    if (userDefaultModel() == u)
        return;

    QSettings setting;
    setting.setValue("userDefaultModel", u);
    setting.sync();
    emit userDefaultModelChanged();
}

QString MySettings::chatTheme() const
{
    QSettings setting;
    setting.sync();
    return setting.value("chatTheme", default_chatTheme).toString();
}

void MySettings::setChatTheme(const QString &u)
{
    if (chatTheme() == u)
        return;

    QSettings setting;
    setting.setValue("chatTheme", u);
    setting.sync();
    emit chatThemeChanged();
}

QString MySettings::fontSize() const
{
    QSettings setting;
    setting.sync();
    return setting.value("fontSize", default_fontSize).toString();
}

void MySettings::setFontSize(const QString &u)
{
    if (fontSize() == u)
        return;

    QSettings setting;
    setting.setValue("fontSize", u);
    setting.sync();
    emit fontSizeChanged();
}

QString MySettings::device() const
{
    QSettings setting;
    setting.sync();
    return setting.value("device", default_device).toString();
}

void MySettings::setDevice(const QString &u)
{
    if (device() == u)
        return;

    QSettings setting;
    setting.setValue("device", u);
    setting.sync();
    emit deviceChanged();
}

bool MySettings::forceMetal() const
{
    return m_forceMetal;
}

void MySettings::setForceMetal(bool b)
{
    if (m_forceMetal == b)
        return;
    m_forceMetal = b;
    emit forceMetalChanged(b);
}

QString MySettings::lastVersionStarted() const
{
    QSettings setting;
    setting.sync();
    return setting.value("lastVersionStarted", default_lastVersionStarted).toString();
}

void MySettings::setLastVersionStarted(const QString &v)
{
    if (lastVersionStarted() == v)
        return;

    QSettings setting;
    setting.setValue("lastVersionStarted", v);
    setting.sync();
    emit lastVersionStartedChanged();
}

int MySettings::localDocsChunkSize() const
{
    QSettings setting;
    setting.sync();
    return setting.value("localdocs/chunkSize", default_localDocsChunkSize).toInt();
}

void MySettings::setLocalDocsChunkSize(int s)
{
    if (localDocsChunkSize() == s)
        return;

    QSettings setting;
    setting.setValue("localdocs/chunkSize", s);
    setting.sync();
    emit localDocsChunkSizeChanged();
}

int MySettings::localDocsRetrievalSize() const
{
    QSettings setting;
    setting.sync();
    return setting.value("localdocs/retrievalSize", default_localDocsRetrievalSize).toInt();
}

void MySettings::setLocalDocsRetrievalSize(int s)
{
    if (localDocsRetrievalSize() == s)
        return;

    QSettings setting;
    setting.setValue("localdocs/retrievalSize", s);
    setting.sync();
    emit localDocsRetrievalSizeChanged();
}

bool MySettings::localDocsShowReferences() const
{
    QSettings setting;
    setting.sync();
    return setting.value("localdocs/showReferences", default_localDocsShowReferences).toBool();
}

void MySettings::setLocalDocsShowReferences(bool b)
{
    if (localDocsShowReferences() == b)
        return;

    QSettings setting;
    setting.setValue("localdocs/showReferences", b);
    setting.sync();
    emit localDocsShowReferencesChanged();
}

QString MySettings::networkAttribution() const
{
    QSettings setting;
    setting.sync();
    return setting.value("network/attribution", default_networkAttribution).toString();
}

void MySettings::setNetworkAttribution(const QString &a)
{
    if (networkAttribution() == a)
        return;

    QSettings setting;
    setting.setValue("network/attribution", a);
    setting.sync();
    emit networkAttributionChanged();
}

bool MySettings::networkIsActive() const
{
    QSettings setting;
    setting.sync();
    return setting.value("network/isActive", default_networkIsActive).toBool();
}

void MySettings::setNetworkIsActive(bool b)
{
    if (networkIsActive() == b)
        return;

    QSettings setting;
    setting.setValue("network/isActive", b);
    setting.sync();
    emit networkIsActiveChanged();
}

bool MySettings::networkUsageStatsActive() const
{
    QSettings setting;
    setting.sync();
    return setting.value("network/usageStatsActive", default_networkUsageStatsActive).toBool();
}

void MySettings::setNetworkUsageStatsActive(bool b)
{
    if (networkUsageStatsActive() == b)
        return;

    QSettings setting;
    setting.setValue("network/usageStatsActive", b);
    setting.sync();
    emit networkUsageStatsActiveChanged();
}

QString MySettings::attemptModelLoad() const
{
    QSettings setting;
    setting.sync();
    return setting.value("attemptModelLoad", QString()).toString();
}

void MySettings::setAttemptModelLoad(const QString &modelFile)
{
    if (attemptModelLoad() == modelFile)
        return;

    QSettings setting;
    if (modelFile.isEmpty())
        setting.remove("attemptModelLoad");
    else
        setting.setValue("attemptModelLoad", modelFile);
    setting.sync();
    emit attemptModelLoadChanged();
}
