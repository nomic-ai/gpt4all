#include "mysettings.h"

#include "../gpt4all-backend/llmodel.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGlobalStatic>
#include <QIODevice>
#include <QMap>
#include <QMetaObject>
#include <QStandardPaths>
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
    { "chatTheme",                "Dark" },
    { "fontSize",                 "Small" },
    { "lastVersionStarted",       "" },
    { "networkPort",              4891, },
    { "saveChatsContext",         false },
    { "serverChat",               false },
    { "userDefaultModel",         "Application default" },
    { "localdocs/chunkSize",      256 },
    { "localdocs/retrievalSize",  3 },
    { "localdocs/showReferences", true },
    { "localdocs/fileExtensions", QStringList { "txt", "pdf", "md", "rst" } },
    { "network/attribution",      "" },
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

void MySettings::restoreModelDefaults(const ModelInfo &info)
{
    setModelTemperature(info, info.m_temperature);
    setModelTopP(info, info.m_topP);
    setModelMinP(info, info.m_minP);
    setModelTopK(info, info.m_topK);;
    setModelMaxLength(info, info.m_maxLength);
    setModelPromptBatchSize(info, info.m_promptBatchSize);
    setModelContextLength(info, info.m_contextLength);
    setModelGpuLayers(info, info.m_gpuLayers);
    setModelRepeatPenalty(info, info.m_repeatPenalty);
    setModelRepeatPenaltyTokens(info, info.m_repeatPenaltyTokens);
    setModelPromptTemplate(info, info.m_promptTemplate);
    setModelSystemPrompt(info, info.m_systemPrompt);
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
    setLocalDocsChunkSize(basicDefaults.value("localdocs/chunkSize").toInt());
    setLocalDocsRetrievalSize(basicDefaults.value("localdocs/retrievalSize").toInt());
    setLocalDocsFileExtensions(basicDefaults.value("localdocs/fileExtensions").toStringList());
    setLocalDocsShowReferences(basicDefaults.value("localdocs/showReferences").toBool());
}

void MySettings::eraseModel(const ModelInfo &info)
{
    m_settings.remove(u"model-%1"_s.arg(info.id()));
}

QString MySettings::modelName(const ModelInfo &info) const
{
    return m_settings.value(u"model-%1/name"_s.arg(info.id()),
        !info.m_name.isEmpty() ? info.m_name : info.m_filename).toString();
}

void MySettings::setModelName(const ModelInfo &info, const QString &value, bool force)
{
    if ((modelName(info) == value || info.id().isEmpty()) && !force)
        return;

    if ((info.m_name == value || info.m_filename == value) && !info.shouldSaveMetadata())
        m_settings.remove(u"model-%1/name"_s.arg(info.id()));
    else
        m_settings.setValue(u"model-%1/name"_s.arg(info.id()), value);
    if (!force)
        emit nameChanged(info);
}

static QString modelSettingName(const ModelInfo &info, const QString &name)
{
    return u"model-%1/%2"_s.arg(info.id(), name);
}

QVariant MySettings::getModelSetting(const QString &name, const ModelInfo &info) const
{
    return m_settings.value(modelSettingName(info, name), info.getFields().value(name));
}

void MySettings::setModelSetting(const QString &name, const ModelInfo &info, const QVariant &value, bool force,
                                 bool signal)
{
    if (!force && (info.id().isEmpty() || getModelSetting(name, info) == value))
        return;

    QString settingName = modelSettingName(info, name);
    if (info.getFields().value(name) == value && !info.shouldSaveMetadata())
        m_settings.remove(settingName);
    else
        m_settings.setValue(settingName, value);
    if (signal && !force)
        QMetaObject::invokeMethod(this, u"%1Changed"_s.arg(name).toLatin1().constData(), Q_ARG(ModelInfo, info));
}

QString   MySettings::modelFilename           (const ModelInfo &info) const { return getModelSetting("filename",            info).toString(); }
QString   MySettings::modelDescription        (const ModelInfo &info) const { return getModelSetting("description",         info).toString(); }
QString   MySettings::modelUrl                (const ModelInfo &info) const { return getModelSetting("url",                 info).toString(); }
QString   MySettings::modelQuant              (const ModelInfo &info) const { return getModelSetting("quant",               info).toString(); }
QString   MySettings::modelType               (const ModelInfo &info) const { return getModelSetting("type",                info).toString(); }
bool      MySettings::modelIsClone            (const ModelInfo &info) const { return getModelSetting("isClone",             info).toBool(); }
bool      MySettings::modelIsDiscovered       (const ModelInfo &info) const { return getModelSetting("isDiscovered",        info).toBool(); }
int       MySettings::modelLikes              (const ModelInfo &info) const { return getModelSetting("likes",               info).toInt(); }
int       MySettings::modelDownloads          (const ModelInfo &info) const { return getModelSetting("downloads",           info).toInt(); }
QDateTime MySettings::modelRecency            (const ModelInfo &info) const { return getModelSetting("recency",             info).toDateTime(); }
double    MySettings::modelTemperature        (const ModelInfo &info) const { return getModelSetting("temperature",         info).toDouble(); }
double    MySettings::modelTopP               (const ModelInfo &info) const { return getModelSetting("topP",                info).toDouble(); }
double    MySettings::modelMinP               (const ModelInfo &info) const { return getModelSetting("minP",                info).toDouble(); }
int       MySettings::modelTopK               (const ModelInfo &info) const { return getModelSetting("topK",                info).toInt(); }
int       MySettings::modelMaxLength          (const ModelInfo &info) const { return getModelSetting("maxLength",           info).toInt(); }
int       MySettings::modelPromptBatchSize    (const ModelInfo &info) const { return getModelSetting("promptBatchSize",     info).toInt(); }
int       MySettings::modelContextLength      (const ModelInfo &info) const { return getModelSetting("contextLength",       info).toInt(); }
int       MySettings::modelGpuLayers          (const ModelInfo &info) const { return getModelSetting("gpuLayers",           info).toInt(); }
double    MySettings::modelRepeatPenalty      (const ModelInfo &info) const { return getModelSetting("repeatPenalty",       info).toDouble(); }
int       MySettings::modelRepeatPenaltyTokens(const ModelInfo &info) const { return getModelSetting("repeatPenaltyTokens", info).toInt(); }
QString   MySettings::modelPromptTemplate     (const ModelInfo &info) const { return getModelSetting("promptTemplate",      info).toString(); }
QString   MySettings::modelSystemPrompt       (const ModelInfo &info) const { return getModelSetting("systemPrompt",        info).toString(); }

void MySettings::setModelFilename(const ModelInfo &info, const QString &value, bool force)
{
    setModelSetting("filename", info, value, force, true);
}

void MySettings::setModelDescription(const ModelInfo &info, const QString &value, bool force)
{
    setModelSetting("description", info, value, force, true);
}

void MySettings::setModelUrl(const ModelInfo &info, const QString &value, bool force)
{
    setModelSetting("url", info, value, force);
}

void MySettings::setModelQuant(const ModelInfo &info, const QString &value, bool force)
{
    setModelSetting("quant", info, value, force);
}

void MySettings::setModelType(const ModelInfo &info, const QString &value, bool force)
{
    setModelSetting("type", info, value, force);
}

void MySettings::setModelIsClone(const ModelInfo &info, bool value, bool force)
{
    setModelSetting("isClone", info, value, force);
}

void MySettings::setModelIsDiscovered(const ModelInfo &info, bool value, bool force)
{
    setModelSetting("isDiscovered", info, value, force);
}

void MySettings::setModelLikes(const ModelInfo &info, int value, bool force)
{
    setModelSetting("likes", info, value, force);
}

void MySettings::setModelDownloads(const ModelInfo &info, int value, bool force)
{
    setModelSetting("downloads", info, value, force);
}

void MySettings::setModelRecency(const ModelInfo &info, const QDateTime &value, bool force)
{
    setModelSetting("recency", info, value, force);
}

void MySettings::setModelTemperature(const ModelInfo &info, double value, bool force)
{
    setModelSetting("temperature", info, value, force, true);
}

void MySettings::setModelTopP(const ModelInfo &info, double value, bool force)
{
    setModelSetting("topP", info, value, force, true);
}

void MySettings::setModelMinP(const ModelInfo &info, double value, bool force)
{
    setModelSetting("minP", info, value, force, true);
}

void MySettings::setModelTopK(const ModelInfo &info, int value, bool force)
{
    setModelSetting("topK", info, value, force, true);
}

void MySettings::setModelMaxLength(const ModelInfo &info, int value, bool force)
{
    setModelSetting("maxLength", info, value, force, true);
}

void MySettings::setModelPromptBatchSize(const ModelInfo &info, int value, bool force)
{
    setModelSetting("promptBatchSize", info, value, force, true);
}

void MySettings::setModelContextLength(const ModelInfo &info, int value, bool force)
{
    setModelSetting("contextLength", info, value, force, true);
}

void MySettings::setModelGpuLayers(const ModelInfo &info, int value, bool force)
{
    setModelSetting("gpuLayers", info, value, force, true);
}

void MySettings::setModelRepeatPenalty(const ModelInfo &info, double value, bool force)
{
    setModelSetting("repeatPenalty", info, value, force, true);
}

void MySettings::setModelRepeatPenaltyTokens(const ModelInfo &info, int value, bool force)
{
    setModelSetting("repeatPenaltyTokens", info, value, force, true);
}

void MySettings::setModelPromptTemplate(const ModelInfo &info, const QString &value, bool force)
{
    setModelSetting("promptTemplate", info, value, force, true);
}

void MySettings::setModelSystemPrompt(const ModelInfo &info, const QString &value, bool force)
{
    setModelSetting("systemPrompt", info, value, force, true);
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

bool        MySettings::saveChatsContext() const        { return getBasicSetting("saveChatsContext"        ).toBool(); }
bool        MySettings::serverChat() const              { return getBasicSetting("serverChat"              ).toBool(); }
int         MySettings::networkPort() const             { return getBasicSetting("networkPort"             ).toInt(); }
QString     MySettings::userDefaultModel() const        { return getBasicSetting("userDefaultModel"        ).toString(); }
QString     MySettings::chatTheme() const               { return getBasicSetting("chatTheme"               ).toString(); }
QString     MySettings::fontSize() const                { return getBasicSetting("fontSize"                ).toString(); }
QString     MySettings::lastVersionStarted() const      { return getBasicSetting("lastVersionStarted"      ).toString(); }
int         MySettings::localDocsChunkSize() const      { return getBasicSetting("localdocs/chunkSize"     ).toInt(); }
int         MySettings::localDocsRetrievalSize() const  { return getBasicSetting("localdocs/retrievalSize" ).toInt(); }
bool        MySettings::localDocsShowReferences() const { return getBasicSetting("localdocs/showReferences").toBool(); }
QStringList MySettings::localDocsFileExtensions() const { return getBasicSetting("localdocs/fileExtensions").toStringList(); }
QString     MySettings::networkAttribution() const      { return getBasicSetting("network/attribution"     ).toString(); }

void MySettings::setSaveChatsContext(bool value)                      { setBasicSetting("saveChatsContext",         value); }
void MySettings::setServerChat(bool value)                            { setBasicSetting("serverChat",               value); }
void MySettings::setNetworkPort(int value)                            { setBasicSetting("networkPort",              value); }
void MySettings::setUserDefaultModel(const QString &value)            { setBasicSetting("userDefaultModel",         value); }
void MySettings::setChatTheme(const QString &value)                   { setBasicSetting("chatTheme",                value); }
void MySettings::setFontSize(const QString &value)                    { setBasicSetting("fontSize",                 value); }
void MySettings::setLastVersionStarted(const QString &value)          { setBasicSetting("lastVersionStarted",       value); }
void MySettings::setLocalDocsChunkSize(int value)                     { setBasicSetting("localdocs/chunkSize",      value, "localDocsChunkSize"); }
void MySettings::setLocalDocsRetrievalSize(int value)                 { setBasicSetting("localdocs/retrievalSize",  value, "localDocsRetrievalSize"); }
void MySettings::setLocalDocsShowReferences(bool value)               { setBasicSetting("localdocs/showReferences", value, "localDocsShowReferences"); }
void MySettings::setLocalDocsFileExtensions(const QStringList &value) { setBasicSetting("localdocs/fileExtensions", value, "localDocsFileExtensions"); }
void MySettings::setNetworkAttribution(const QString &value)          { setBasicSetting("network/attribution",      value, "networkAttribution"); }

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
