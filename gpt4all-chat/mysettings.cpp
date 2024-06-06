#include "mysettings.h"

#include "../gpt4all-backend/llmodel.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGlobalStatic>
#include <QIODevice>
#include <QMetaObject>
#include <QStandardPaths>
#include <QStringList>
#include <QThread>
#include <QUrl>
#include <QVariant>
#include <QtLogging>

#include <algorithm>
#include <string>
#include <thread>
#include <vector>

using namespace Qt::Literals::StringLiterals;

namespace defaults {

static const int     threadCount             = std::min(4, (int32_t) std::thread::hardware_concurrency());
static const bool    forceMetal              = false;
static const bool    networkIsActive         = false;
static const bool    networkUsageStatsActive = false;
static const QString device                  = "Auto";

} // namespace defaults

static const QVariantMap basicDefaults {
    { "saveChatsContext",         false },
    { "serverChat",               false },
    { "userDefaultModel",         "Application default" },
    { "lastVersionStarted",       "" },
    { "localDocs/chunkSize",      256 },
    { "chatTheme",                "Dark" },
    { "fontSize",                 "Small" },
    { "localDocs/retrievalSize",  3 },
    { "localDocs/showReferences", true },
    { "network/attribution",      "" },
    { "networkPort",              4891, },
};

static QString defaultLocalModelsPath()
{
    QString localPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        + "/";
    QString testWritePath = localPath + u"test_write.txt"_s;
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
    QVector<QString> deviceList{ "Auto" };
#if defined(Q_OS_MAC) && defined(__aarch64__)
    deviceList << "Metal";
#else
    std::vector<LLModel::GPUDevice> devices = LLModel::Implementation::availableGPUDevices();
    for (LLModel::GPUDevice &d : devices)
        deviceList << QString::fromStdString(d.selectionName());
#endif
    deviceList << "CPU";
    setDeviceList(deviceList);
}

QVariant MySettings::getBasicSetting(const QString &name) const
{
    return m_settings.value(name, basicDefaults.value(name));
}

void MySettings::setBasicSetting(const QString &name, const QVariant &value, std::optional<QString> signal)
{
    if (getBasicSetting(name) == value)
        return;

    m_settings.setValue(name, value);
    QMetaObject::invokeMethod(this, u"%1Changed"_s.arg(signal.value_or(name)).toLatin1().constData());
}

Q_INVOKABLE QVector<QString> MySettings::deviceList() const
{
    return m_deviceList;
}

void MySettings::setDeviceList(const QVector<QString> &value)
{
    m_deviceList = value;
    emit deviceListChanged();
}

void MySettings::restoreModelDefaults(const ModelInfo &model)
{
    setModelTemperature(model, model.m_temperature);
    setModelTopP(model, model.m_topP);
    setModelMinP(model, model.m_minP);
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
    setChatTheme(basicDefaults.value("chatTheme").toString());
    setFontSize(basicDefaults.value("fontSize").toString());
    setDevice(defaults::device);
    setThreadCount(defaults::threadCount);
    setSaveChatsContext(basicDefaults.value("saveChatsContext").toBool());
    setServerChat(basicDefaults.value("serverChat").toBool());
    setNetworkPort(basicDefaults.value("networkPort").toInt());
    setModelPath(defaultLocalModelsPath());
    setUserDefaultModel(basicDefaults.value("userDefaultModel").toString());
    setForceMetal(defaults::forceMetal);
}

void MySettings::restoreLocalDocsDefaults()
{
    setLocalDocsChunkSize(basicDefaults.value("localDocs/chunkSize").toInt());
    setLocalDocsRetrievalSize(basicDefaults.value("localDocs/retrievalSize").toInt());
    setLocalDocsShowReferences(basicDefaults.value("localDocs/showReferences").toBool());
}

void MySettings::eraseModel(const ModelInfo &m)
{
    m_settings.remove(u"model-%1"_s.arg(m.id()));
}

QString MySettings::modelName(const ModelInfo &m) const
{
    return m_settings.value(u"model-%1/name"_s.arg(m.id()),
        !m.m_name.isEmpty() ? m.m_name : m.m_filename).toString();
}

void MySettings::setModelName(const ModelInfo &m, const QString &name, bool force)
{
    if ((modelName(m) == name || m.id().isEmpty()) && !force)
        return;

    if ((m.m_name == name || m.m_filename == name) && !m.shouldSaveMetadata())
        m_settings.remove(u"model-%1/name"_s.arg(m.id()));
    else
        m_settings.setValue(u"model-%1/name"_s.arg(m.id()), name);
    if (!force)
        emit nameChanged(m);
}

QString MySettings::modelFilename(const ModelInfo &m) const
{
    return m_settings.value(u"model-%1/filename"_s.arg(m.id()), m.m_filename).toString();
}

void MySettings::setModelFilename(const ModelInfo &m, const QString &filename, bool force)
{
    if ((modelFilename(m) == filename || m.id().isEmpty()) && !force)
        return;

    if (m.m_filename == filename && !m.shouldSaveMetadata())
        m_settings.remove(u"model-%1/filename"_s.arg(m.id()));
    else
        m_settings.setValue(u"model-%1/filename"_s.arg(m.id()), filename);
    if (!force)
        emit filenameChanged(m);
}

QString MySettings::modelDescription(const ModelInfo &m) const
{
    return m_settings.value(u"model-%1/description"_s.arg(m.id()), m.m_description).toString();
}

void MySettings::setModelDescription(const ModelInfo &m, const QString &d, bool force)
{
    if ((modelDescription(m) == d || m.id().isEmpty()) && !force)
        return;

    if (m.m_description == d && !m.shouldSaveMetadata())
        m_settings.remove(u"model-%1/description"_s.arg(m.id()));
    else
        m_settings.setValue(u"model-%1/description"_s.arg(m.id()), d);
}

QString MySettings::modelUrl(const ModelInfo &m) const
{
    return m_settings.value(u"model-%1/url"_s.arg(m.id()), m.m_url).toString();
}

void MySettings::setModelUrl(const ModelInfo &m, const QString &u, bool force)
{
    if ((modelUrl(m) == u || m.id().isEmpty()) && !force)
        return;

    if (m.m_url == u && !m.shouldSaveMetadata())
        m_settings.remove(u"model-%1/url"_s.arg(m.id()));
    else
        m_settings.setValue(u"model-%1/url"_s.arg(m.id()), u);
}

QString MySettings::modelQuant(const ModelInfo &m) const
{
    return m_settings.value(u"model-%1/quant"_s.arg(m.id()), m.m_quant).toString();
}

void MySettings::setModelQuant(const ModelInfo &m, const QString &q, bool force)
{
    if ((modelUrl(m) == q || m.id().isEmpty()) && !force)
        return;

    if (m.m_quant == q && !m.shouldSaveMetadata())
        m_settings.remove(u"model-%1/quant"_s.arg(m.id()));
    else
        m_settings.setValue(u"model-%1/quant"_s.arg(m.id()), q);
}

QString MySettings::modelType(const ModelInfo &m) const
{
    return m_settings.value(u"model-%1/type"_s.arg(m.id()), m.m_type).toString();
}

void MySettings::setModelType(const ModelInfo &m, const QString &t, bool force)
{
    if ((modelType(m) == t || m.id().isEmpty()) && !force)
        return;

    if (m.m_type == t && !m.shouldSaveMetadata())
        m_settings.remove(u"model-%1/type"_s.arg(m.id()));
    else
        m_settings.setValue(u"model-%1/type"_s.arg(m.id()), t);
}

bool MySettings::modelIsClone(const ModelInfo &m) const
{
    return m_settings.value(u"model-%1/isClone"_s.arg(m.id()), m.m_isClone).toBool();
}

void MySettings::setModelIsClone(const ModelInfo &m, bool b, bool force)
{
    if ((modelIsClone(m) == b || m.id().isEmpty()) && !force)
        return;

    if (m.m_isClone == b && !m.shouldSaveMetadata())
        m_settings.remove(u"model-%1/isClone"_s.arg(m.id()));
    else
        m_settings.setValue(u"model-%1/isClone"_s.arg(m.id()), b);
}

bool MySettings::modelIsDiscovered(const ModelInfo &m) const
{
    return m_settings.value(u"model-%1/isDiscovered"_s.arg(m.id()), m.m_isDiscovered).toBool();
}

void MySettings::setModelIsDiscovered(const ModelInfo &m, bool b, bool force)
{
    if ((modelIsDiscovered(m) == b || m.id().isEmpty()) && !force)
        return;

    if (m.m_isDiscovered == b && !m.shouldSaveMetadata())
        m_settings.remove(u"model-%1/isDiscovered"_s.arg(m.id()));
    else
        m_settings.setValue(u"model-%1/isDiscovered"_s.arg(m.id()), b);
}

int MySettings::modelLikes(const ModelInfo &m) const
{
    return m_settings.value(u"model-%1/likes"_s.arg(m.id()), m.m_likes).toInt();
}

void MySettings::setModelLikes(const ModelInfo &m, int l, bool force)
{
    if ((modelLikes(m) == l || m.id().isEmpty()) && !force)
        return;

    if (m.m_likes == l && !m.shouldSaveMetadata())
        m_settings.remove(u"model-%1/likes"_s.arg(m.id()));
    else
        m_settings.setValue(u"model-%1/likes"_s.arg(m.id()), l);
}

int MySettings::modelDownloads(const ModelInfo &m) const
{
    return m_settings.value(u"model-%1/downloads"_s.arg(m.id()), m.m_downloads).toInt();
}

void MySettings::setModelDownloads(const ModelInfo &m, int d, bool force)
{
    if ((modelDownloads(m) == d || m.id().isEmpty()) && !force)
        return;

    if (m.m_downloads == d && !m.shouldSaveMetadata())
        m_settings.remove(u"model-%1/downloads"_s.arg(m.id()));
    else
        m_settings.setValue(u"model-%1/downloads"_s.arg(m.id()), d);
}

QDateTime MySettings::modelRecency(const ModelInfo &m) const
{
    return m_settings.value(u"model-%1/recency"_s.arg(m.id()), m.m_recency).toDateTime();
}

void MySettings::setModelRecency(const ModelInfo &m, const QDateTime &r, bool force)
{
    if ((modelRecency(m) == r || m.id().isEmpty()) && !force)
        return;

    if (m.m_recency == r && !m.shouldSaveMetadata())
        m_settings.remove(u"model-%1/recency"_s.arg(m.id()));
    else
        m_settings.setValue(u"model-%1/recency"_s.arg(m.id()), r);
}

double MySettings::modelTemperature(const ModelInfo &m) const
{
    return m_settings.value(u"model-%1/temperature"_s.arg(m.id()), m.m_temperature).toDouble();
}

void MySettings::setModelTemperature(const ModelInfo &m, double t, bool force)
{
    if (modelTemperature(m) == t && !force)
        return;

    if (m.m_temperature == t && !m.shouldSaveMetadata())
        m_settings.remove(u"model-%1/temperature"_s.arg(m.id()));
    else
        m_settings.setValue(u"model-%1/temperature"_s.arg(m.id()), t);
    if (!force)
        emit temperatureChanged(m);
}

double MySettings::modelTopP(const ModelInfo &m) const
{
    return m_settings.value(u"model-%1/topP"_s.arg(m.id()), m.m_topP).toDouble();
}

double MySettings::modelMinP(const ModelInfo &m) const
{
    return m_settings.value(u"model-%1/minP"_s.arg(m.id()), m.m_minP).toDouble();
}

void MySettings::setModelTopP(const ModelInfo &m, double p, bool force)
{
    if (modelTopP(m) == p && !force)
        return;

    if (m.m_topP == p && !m.shouldSaveMetadata())
        m_settings.remove(u"model-%1/topP"_s.arg(m.id()));
    else
        m_settings.setValue(u"model-%1/topP"_s.arg(m.id()), p);
    if (!force)
        emit topPChanged(m);
}

void MySettings::setModelMinP(const ModelInfo &m, double p, bool force)
{
    if (modelMinP(m) == p && !force)
        return;

    if (m.m_minP == p && !m.shouldSaveMetadata())
        m_settings.remove(u"model-%1/minP"_s.arg(m.id()));
    else
        m_settings.setValue(u"model-%1/minP"_s.arg(m.id()), p);
    if (!force)
        emit minPChanged(m);
}

int MySettings::modelTopK(const ModelInfo &m) const
{
    return m_settings.value(u"model-%1/topK"_s.arg(m.id()), m.m_topK).toInt();
}

void MySettings::setModelTopK(const ModelInfo &m, int k, bool force)
{
    if (modelTopK(m) == k && !force)
        return;

    if (m.m_topK == k && !m.shouldSaveMetadata())
        m_settings.remove(u"model-%1/topK"_s.arg(m.id()));
    else
        m_settings.setValue(u"model-%1/topK"_s.arg(m.id()), k);
    if (!force)
        emit topKChanged(m);
}

int MySettings::modelMaxLength(const ModelInfo &m) const
{
    return m_settings.value(u"model-%1/maxLength"_s.arg(m.id()), m.m_maxLength).toInt();
}

void MySettings::setModelMaxLength(const ModelInfo &m, int l, bool force)
{
    if (modelMaxLength(m) == l && !force)
        return;

    if (m.m_maxLength == l && !m.shouldSaveMetadata())
        m_settings.remove(u"model-%1/maxLength"_s.arg(m.id()));
    else
        m_settings.setValue(u"model-%1/maxLength"_s.arg(m.id()), l);
    if (!force)
        emit maxLengthChanged(m);
}

int MySettings::modelPromptBatchSize(const ModelInfo &m) const
{
    return m_settings.value(u"model-%1/promptBatchSize"_s.arg(m.id()), m.m_promptBatchSize).toInt();
}

void MySettings::setModelPromptBatchSize(const ModelInfo &m, int s, bool force)
{
    if (modelPromptBatchSize(m) == s && !force)
        return;

    if (m.m_promptBatchSize == s && !m.shouldSaveMetadata())
        m_settings.remove(u"model-%1/promptBatchSize"_s.arg(m.id()));
    else
        m_settings.setValue(u"model-%1/promptBatchSize"_s.arg(m.id()), s);
    if (!force)
        emit promptBatchSizeChanged(m);
}

int MySettings::modelContextLength(const ModelInfo &m) const
{
    return m_settings.value(u"model-%1/contextLength"_s.arg(m.id()), m.m_contextLength).toInt();
}

void MySettings::setModelContextLength(const ModelInfo &m, int l, bool force)
{
    if (modelContextLength(m) == l && !force)
        return;

    if (m.m_contextLength == l && !m.shouldSaveMetadata())
        m_settings.remove(u"model-%1/contextLength"_s.arg(m.id()));
    else
        m_settings.setValue(u"model-%1/contextLength"_s.arg(m.id()), l);
    if (!force)
        emit contextLengthChanged(m);
}

int MySettings::modelGpuLayers(const ModelInfo &m) const
{
    return m_settings.value(u"model-%1/gpuLayers"_s.arg(m.id()), m.m_gpuLayers).toInt();
}

void MySettings::setModelGpuLayers(const ModelInfo &m, int l, bool force)
{
    if (modelGpuLayers(m) == l && !force)
        return;

    if (m.m_gpuLayers == l && !m.shouldSaveMetadata())
        m_settings.remove(u"model-%1/gpuLayers"_s.arg(m.id()));
    else
        m_settings.setValue(u"model-%1/gpuLayers"_s.arg(m.id()), l);
    if (!force)
        emit gpuLayersChanged(m);
}

double MySettings::modelRepeatPenalty(const ModelInfo &m) const
{
    return m_settings.value(u"model-%1/repeatPenalty"_s.arg(m.id()), m.m_repeatPenalty).toDouble();
}

void MySettings::setModelRepeatPenalty(const ModelInfo &m, double p, bool force)
{
    if (modelRepeatPenalty(m) == p && !force)
        return;

    if (m.m_repeatPenalty == p && !m.shouldSaveMetadata())
        m_settings.remove(u"model-%1/repeatPenalty"_s.arg(m.id()));
    else
        m_settings.setValue(u"model-%1/repeatPenalty"_s.arg(m.id()), p);
    if (!force)
        emit repeatPenaltyChanged(m);
}

int MySettings::modelRepeatPenaltyTokens(const ModelInfo &m) const
{
    return m_settings.value(u"model-%1/repeatPenaltyTokens"_s.arg(m.id()), m.m_repeatPenaltyTokens).toInt();
}

void MySettings::setModelRepeatPenaltyTokens(const ModelInfo &m, int t, bool force)
{
    if (modelRepeatPenaltyTokens(m) == t && !force)
        return;

    if (m.m_repeatPenaltyTokens == t && !m.shouldSaveMetadata())
        m_settings.remove(u"model-%1/repeatPenaltyTokens"_s.arg(m.id()));
    else
        m_settings.setValue(u"model-%1/repeatPenaltyTokens"_s.arg(m.id()), t);
    if (!force)
        emit repeatPenaltyTokensChanged(m);
}

QString MySettings::modelPromptTemplate(const ModelInfo &m) const
{
    return m_settings.value(u"model-%1/promptTemplate"_s.arg(m.id()), m.m_promptTemplate).toString();
}

void MySettings::setModelPromptTemplate(const ModelInfo &m, const QString &t, bool force)
{
    if (modelPromptTemplate(m) == t && !force)
        return;

    if (m.m_promptTemplate == t && !m.shouldSaveMetadata())
        m_settings.remove(u"model-%1/promptTemplate"_s.arg(m.id()));
    else
        m_settings.setValue(u"model-%1/promptTemplate"_s.arg(m.id()), t);
    if (!force)
        emit promptTemplateChanged(m);
}

QString MySettings::modelSystemPrompt(const ModelInfo &m) const
{
    return m_settings.value(u"model-%1/systemPrompt"_s.arg(m.id()), m.m_systemPrompt).toString();
}

void MySettings::setModelSystemPrompt(const ModelInfo &m, const QString &p, bool force)
{
    if (modelSystemPrompt(m) == p && !force)
        return;

    if (m.m_systemPrompt == p && !m.shouldSaveMetadata())
        m_settings.remove(u"model-%1/systemPrompt"_s.arg(m.id()));
    else
        m_settings.setValue(u"model-%1/systemPrompt"_s.arg(m.id()), p);
    if (!force)
        emit systemPromptChanged(m);
}

int MySettings::threadCount() const
{
    int c = m_settings.value("threadCount", defaults::threadCount).toInt();
    // The old thread setting likely left many people with 0 in settings config file, which means
    // we should reset it to the default going forward
    if (c <= 0)
        c = defaults::threadCount;
    c = std::max(c, 1);
    c = std::min(c, QThread::idealThreadCount());
    return c;
}

void MySettings::setThreadCount(int value)
{
    if (threadCount() == value)
        return;

    value = std::max(value, 1);
    value = std::min(value, QThread::idealThreadCount());
    m_settings.setValue("threadCount", value);
    emit threadCountChanged();
}

bool    MySettings::saveChatsContext() const        { return getBasicSetting("saveChatsContext").toBool(); }
bool    MySettings::serverChat() const              { return getBasicSetting("serverChat").toBool(); }
int     MySettings::networkPort() const             { return getBasicSetting("networkPort").toInt(); }
QString MySettings::userDefaultModel() const        { return getBasicSetting("userDefaultModel").toString(); }
QString MySettings::chatTheme() const               { return getBasicSetting("chatTheme").toString(); }
QString MySettings::fontSize() const                { return getBasicSetting("fontSize").toString(); }
QString MySettings::lastVersionStarted() const      { return getBasicSetting("lastVersionStarted").toString(); }
int     MySettings::localDocsChunkSize() const      { return getBasicSetting("localdocs/chunkSize").toInt(); }
int     MySettings::localDocsRetrievalSize() const  { return getBasicSetting("localdocs/retrievalSize").toInt(); }
bool    MySettings::localDocsShowReferences() const { return getBasicSetting("localdocs/showReferences").toBool(); }
QString MySettings::networkAttribution() const      { return getBasicSetting("network/attribution").toString(); }

void MySettings::setSaveChatsContext(bool value)             { setBasicSetting("saveChatsContext", value); }
void MySettings::setServerChat(bool value)                   { setBasicSetting("serverChat", value); }
void MySettings::setNetworkPort(int value)                   { setBasicSetting("networkPort", value); }
void MySettings::setUserDefaultModel(const QString &value)   { setBasicSetting("userDefaultModel", value); }
void MySettings::setChatTheme(const QString &value)          { setBasicSetting("chatTheme", value); }
void MySettings::setFontSize(const QString &value)           { setBasicSetting("fontSize", value); }
void MySettings::setLastVersionStarted(const QString &value) { setBasicSetting("lastVersionStarted", value); }
void MySettings::setLocalDocsChunkSize(int value)            { setBasicSetting("localdocs/chunkSize", value, "localDocsChunkSize"); }
void MySettings::setLocalDocsRetrievalSize(int value)        { setBasicSetting("localdocs/retrievalSize", value, "localDocsRetrievalSize"); }
void MySettings::setLocalDocsShowReferences(bool value)      { setBasicSetting("localdocs/showReferences", value, "localDocsShowReferences"); }
void MySettings::setNetworkAttribution(const QString &value) { setBasicSetting("network/attribution", value, "networkAttribution"); }

QString MySettings::modelPath()
{
    // We have to migrate the old setting because I changed the setting key recklessly in v2.4.11
    // which broke a lot of existing installs
    const bool containsOldSetting = m_settings.contains("modelPaths");
    if (containsOldSetting) {
        const bool containsNewSetting = m_settings.contains("modelPath");
        if (!containsNewSetting)
            m_settings.setValue("modelPath", m_settings.value("modelPaths"));
        m_settings.remove("modelPaths");
    }
    return m_settings.value("modelPath", defaultLocalModelsPath()).toString();
}

void MySettings::setModelPath(const QString &value)
{
    QString filePath = (value.startsWith("file://") ?
                        QUrl(value).toLocalFile() : value);
    QString canonical = QFileInfo(filePath).canonicalFilePath() + "/";
    if (modelPath() == canonical)
        return;
    m_settings.setValue("modelPath", canonical);
    emit modelPathChanged();
}

QString MySettings::device()
{
    auto value = m_settings.value("device");
    if (!value.isValid())
        return defaults::device;

    auto device = value.toString();
    if (!device.isEmpty()) {
        auto deviceStr = device.toStdString();
        auto newNameStr = LLModel::GPUDevice::updateSelectionName(deviceStr);
        if (newNameStr != deviceStr) {
            auto newName = QString::fromStdString(newNameStr);
            qWarning() << "updating device name:" << device << "->" << newName;
            device = newName;
            m_settings.setValue("device", device);
        }
    }
    return device;
}

void MySettings::setDevice(const QString &value)
{
    if (device() == value)
        return;

    m_settings.setValue("device", value);
    emit deviceChanged();
}

bool MySettings::forceMetal() const
{
    return m_forceMetal;
}

void MySettings::setForceMetal(bool value)
{
    if (m_forceMetal == value)
        return;
    m_forceMetal = value;
    emit forceMetalChanged(value);
}

bool MySettings::networkIsActive() const
{
    return m_settings.value("network/isActive", defaults::networkIsActive).toBool();
}

bool MySettings::isNetworkIsActiveSet() const
{
    return m_settings.value("network/isActive").isValid();
}

void MySettings::setNetworkIsActive(bool value)
{
    auto cur = m_settings.value("network/isActive");
    if (!cur.isValid() || cur.toBool() != value) {
        m_settings.setValue("network/isActive", value);
        emit networkIsActiveChanged();
    }
}

bool MySettings::networkUsageStatsActive() const
{
    return m_settings.value("network/usageStatsActive", defaults::networkUsageStatsActive).toBool();
}

bool MySettings::isNetworkUsageStatsActiveSet() const
{
    return m_settings.value("network/usageStatsActive").isValid();
}

void MySettings::setNetworkUsageStatsActive(bool value)
{
    auto cur = m_settings.value("network/usageStatsActive");
    if (!cur.isValid() || cur.toBool() != value) {
        m_settings.setValue("network/usageStatsActive", value);
        emit networkUsageStatsActiveChanged();
    }
}
