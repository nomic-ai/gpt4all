#include "mysettings.h"

#include "chatllm.h"
#include "modellist.h"

#include <gpt4all-backend/llmodel.h>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGlobalStatic>
#include <QGuiApplication>
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

// used only for settings serialization, do not translate
static const QStringList suggestionModeNames { "LocalDocsOnly", "On", "Off" };
static const QStringList chatThemeNames      { "Light", "Dark", "LegacyDark" };
static const QStringList fontSizeNames       { "Small", "Medium", "Large" };

// psuedo-enum
namespace ModelSettingsKey { namespace {
    auto ChatTemplate   = "chatTemplate"_L1;
    auto PromptTemplate = "promptTemplate"_L1; // legacy
    auto SystemMessage  = "systemMessage"_L1;
    auto SystemPrompt   = "systemPrompt"_L1;   // legacy
} } // namespace ModelSettingsKey::(anonymous)

namespace defaults {

static const int     threadCount             = std::min(4, (int32_t) std::thread::hardware_concurrency());
static const bool    forceMetal              = false;
static const bool    networkIsActive         = false;
static const bool    networkUsageStatsActive = false;
static const QString device                  = "Auto";
static const QString languageAndLocale       = "System Locale";

} // namespace defaults

static const QVariantMap basicDefaults {
    { "chatTheme",                QVariant::fromValue(ChatTheme::Light) },
    { "fontSize",                 QVariant::fromValue(FontSize::Small) },
    { "lastVersionStarted",       "" },
    { "networkPort",              4891, },
    { "systemTray",               false },
    { "serverChat",               false },
    { "userDefaultModel",         "Application default" },
    { "suggestionMode",           QVariant::fromValue(SuggestionMode::LocalDocsOnly) },
    { "localdocs/chunkSize",      512 },
    { "localdocs/retrievalSize",  3 },
    { "localdocs/showReferences", true },
    { "localdocs/fileExtensions", QStringList { "docx", "pdf", "txt", "md", "rst" } },
    { "localdocs/useRemoteEmbed", false },
    { "localdocs/nomicAPIKey",    "" },
    { "localdocs/embedDevice",    "Auto" },
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

static QStringList getDevices(bool skipKompute = false)
{
    QStringList deviceList;
#if defined(Q_OS_MAC) && defined(__aarch64__)
    deviceList << "Metal";
#else
    std::vector<LLModel::GPUDevice> devices = LLModel::Implementation::availableGPUDevices();
    for (LLModel::GPUDevice &d : devices) {
        if (!skipKompute || strcmp(d.backend, "kompute"))
            deviceList << QString::fromStdString(d.selectionName());
    }
#endif
    deviceList << "CPU";
    return deviceList;
}

static QString getUiLanguage(const QString directory, const QString fileName)
{
    QTranslator translator;
    const QString filePath = directory + QDir::separator() + fileName;
    if (translator.load(filePath)) {
        const QString lang = fileName.mid(fileName.indexOf('_') + 1,
            fileName.lastIndexOf('.') - fileName.indexOf('_') - 1);
        return lang;
    }

    qDebug() << "ERROR: Failed to load translation file:" << filePath;
    return QString();
}

static QStringList getUiLanguages(const QString &modelPath)
{
    QStringList languageList;
    static const QStringList releasedLanguages = { "en_US", "it_IT", "zh_CN", "zh_TW", "es_MX", "pt_BR", "ro_RO" };

    // Add the language translations from model path files first which is used by translation developers
    // to load translations in progress without having to rebuild all of GPT4All from source
    {
        const QDir dir(modelPath);
        const QStringList qmFiles = dir.entryList({"*.qm"}, QDir::Files);
        for (const QString &fileName : qmFiles)
            languageList << getUiLanguage(modelPath, fileName);
    }

    // Now add the internal language translations
    {
        const QDir dir(":/i18n");
        const QStringList qmFiles = dir.entryList({"*.qm"}, QDir::Files);
        for (const QString &fileName : qmFiles) {
            const QString lang = getUiLanguage(":/i18n", fileName);
            if (!languageList.contains(lang) && releasedLanguages.contains(lang))
                languageList.append(lang);
        }
    }
    return languageList;
}

static QString modelSettingName(const ModelInfo &info, auto &&name)
{
    return u"model-%1/%2"_s.arg(info.id(), name);
}

class MyPrivateSettings: public MySettings { };
Q_GLOBAL_STATIC(MyPrivateSettings, settingsInstance)
MySettings *MySettings::globalInstance()
{
    return settingsInstance();
}

MySettings::MySettings()
    : QObject(nullptr)
    , m_deviceList(getDevices())
    , m_embeddingsDeviceList(getDevices(/*skipKompute*/ true))
    , m_uiLanguages(getUiLanguages(modelPath()))
{
}

QVariant MySettings::checkJinjaTemplateError(const QString &tmpl)
{
    if (auto err = ChatLLM::checkJinjaTemplateError(tmpl.toStdString()))
        return QString::fromStdString(*err);
    return QVariant::fromValue(nullptr);
}

// Unset settings come from ModelInfo. Listen for changes so we can emit our own setting-specific signals.
void MySettings::onModelInfoChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles)
{
    auto settingChanged = [&](const auto &info, auto role, const auto &name) {
        return (roles.isEmpty() || roles.contains(role)) && !m_settings.contains(modelSettingName(info, name));
    };

    auto &modelList = dynamic_cast<const ModelList &>(*QObject::sender());
    for (int row = topLeft.row(); row <= bottomRight.row(); row++) {
        using enum ModelList::Roles;
        using namespace ModelSettingsKey;
        auto index = topLeft.siblingAtRow(row);
        if (auto info = modelList.modelInfo(index.data(IdRole).toString()); !info.id().isNull()) {
            if (settingChanged(info, ChatTemplateRole, ChatTemplate))
                emit chatTemplateChanged(info, /*fromInfo*/ true);
            if (settingChanged(info, SystemMessageRole, SystemMessage))
                emit systemMessageChanged(info, /*fromInfo*/ true);
        }
    }
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

int MySettings::getEnumSetting(const QString &setting, const QStringList &valueNames) const
{
    int idx = valueNames.indexOf(getBasicSetting(setting).toString());
    return idx != -1 ? idx : *reinterpret_cast<const int *>(basicDefaults.value(setting).constData());
}

void MySettings::restoreModelDefaults(const ModelInfo &info)
{
    setModelTemperature(info, info.m_temperature);
    setModelTopP(info, info.m_topP);
    setModelMinP(info, info.m_minP);
    setModelTopK(info, info.m_topK);
    setModelMaxLength(info, info.m_maxLength);
    setModelPromptBatchSize(info, info.m_promptBatchSize);
    setModelContextLength(info, info.m_contextLength);
    setModelGpuLayers(info, info.m_gpuLayers);
    setModelRepeatPenalty(info, info.m_repeatPenalty);
    setModelRepeatPenaltyTokens(info, info.m_repeatPenaltyTokens);
    resetModelChatTemplate (info);
    resetModelSystemMessage(info);
    setModelChatNamePrompt(info, info.m_chatNamePrompt);
    setModelSuggestedFollowUpPrompt(info, info.m_suggestedFollowUpPrompt);
}

void MySettings::restoreApplicationDefaults()
{
    setChatTheme(basicDefaults.value("chatTheme").value<ChatTheme>());
    setFontSize(basicDefaults.value("fontSize").value<FontSize>());
    setDevice(defaults::device);
    setThreadCount(defaults::threadCount);
    setSystemTray(basicDefaults.value("systemTray").toBool());
    setServerChat(basicDefaults.value("serverChat").toBool());
    setNetworkPort(basicDefaults.value("networkPort").toInt());
    setModelPath(defaultLocalModelsPath());
    setUserDefaultModel(basicDefaults.value("userDefaultModel").toString());
    setForceMetal(defaults::forceMetal);
    setSuggestionMode(basicDefaults.value("suggestionMode").value<SuggestionMode>());
    setLanguageAndLocale(defaults::languageAndLocale);
}

void MySettings::restoreLocalDocsDefaults()
{
    setLocalDocsChunkSize(basicDefaults.value("localdocs/chunkSize").toInt());
    setLocalDocsRetrievalSize(basicDefaults.value("localdocs/retrievalSize").toInt());
    setLocalDocsShowReferences(basicDefaults.value("localdocs/showReferences").toBool());
    setLocalDocsFileExtensions(basicDefaults.value("localdocs/fileExtensions").toStringList());
    setLocalDocsUseRemoteEmbed(basicDefaults.value("localdocs/useRemoteEmbed").toBool());
    setLocalDocsNomicAPIKey(basicDefaults.value("localdocs/nomicAPIKey").toString());
    setLocalDocsEmbedDevice(basicDefaults.value("localdocs/embedDevice").toString());
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

QVariant MySettings::getModelSetting(QLatin1StringView name, const ModelInfo &info) const
{
    QLatin1StringView nameL1(name);
    return m_settings.value(modelSettingName(info, nameL1), info.getField(nameL1));
}

QVariant MySettings::getModelSetting(const char *name, const ModelInfo &info) const
{
    return getModelSetting(QLatin1StringView(name), info);
}

void MySettings::setModelSetting(QLatin1StringView name, const ModelInfo &info, const QVariant &value, bool force,
                                 bool signal)
{
    if (!force && (info.id().isEmpty() || getModelSetting(name, info) == value))
        return;

    QLatin1StringView nameL1(name);
    QString settingName = modelSettingName(info, nameL1);
    if (info.getField(nameL1) == value && !info.shouldSaveMetadata())
        m_settings.remove(settingName);
    else
        m_settings.setValue(settingName, value);
    if (signal && !force)
        QMetaObject::invokeMethod(this, u"%1Changed"_s.arg(nameL1).toLatin1().constData(), Q_ARG(ModelInfo, info));
}

void MySettings::setModelSetting(const char *name, const ModelInfo &info, const QVariant &value, bool force,
                                 bool signal)
{
    setModelSetting(QLatin1StringView(name), info, value, force, signal);
}

QString   MySettings::modelFilename               (const ModelInfo &info) const { return getModelSetting("filename",                info).toString(); }
QString   MySettings::modelDescription            (const ModelInfo &info) const { return getModelSetting("description",             info).toString(); }
QString   MySettings::modelUrl                    (const ModelInfo &info) const { return getModelSetting("url",                     info).toString(); }
QString   MySettings::modelQuant                  (const ModelInfo &info) const { return getModelSetting("quant",                   info).toString(); }
QString   MySettings::modelType                   (const ModelInfo &info) const { return getModelSetting("type",                    info).toString(); }
bool      MySettings::modelIsClone                (const ModelInfo &info) const { return getModelSetting("isClone",                 info).toBool(); }
bool      MySettings::modelIsDiscovered           (const ModelInfo &info) const { return getModelSetting("isDiscovered",            info).toBool(); }
int       MySettings::modelLikes                  (const ModelInfo &info) const { return getModelSetting("likes",                   info).toInt(); }
int       MySettings::modelDownloads              (const ModelInfo &info) const { return getModelSetting("downloads",               info).toInt(); }
QDateTime MySettings::modelRecency                (const ModelInfo &info) const { return getModelSetting("recency",                 info).toDateTime(); }
double    MySettings::modelTemperature            (const ModelInfo &info) const { return getModelSetting("temperature",             info).toDouble(); }
double    MySettings::modelTopP                   (const ModelInfo &info) const { return getModelSetting("topP",                    info).toDouble(); }
double    MySettings::modelMinP                   (const ModelInfo &info) const { return getModelSetting("minP",                    info).toDouble(); }
int       MySettings::modelTopK                   (const ModelInfo &info) const { return getModelSetting("topK",                    info).toInt(); }
int       MySettings::modelMaxLength              (const ModelInfo &info) const { return getModelSetting("maxLength",               info).toInt(); }
int       MySettings::modelPromptBatchSize        (const ModelInfo &info) const { return getModelSetting("promptBatchSize",         info).toInt(); }
int       MySettings::modelContextLength          (const ModelInfo &info) const { return getModelSetting("contextLength",           info).toInt(); }
int       MySettings::modelGpuLayers              (const ModelInfo &info) const { return getModelSetting("gpuLayers",               info).toInt(); }
double    MySettings::modelRepeatPenalty          (const ModelInfo &info) const { return getModelSetting("repeatPenalty",           info).toDouble(); }
int       MySettings::modelRepeatPenaltyTokens    (const ModelInfo &info) const { return getModelSetting("repeatPenaltyTokens",     info).toInt(); }
QString   MySettings::modelChatNamePrompt         (const ModelInfo &info) const { return getModelSetting("chatNamePrompt",          info).toString(); }
QString   MySettings::modelSuggestedFollowUpPrompt(const ModelInfo &info) const { return getModelSetting("suggestedFollowUpPrompt", info).toString(); }

auto MySettings::getUpgradeableModelSetting(
    const ModelInfo &info, QLatin1StringView legacyKey, QLatin1StringView newKey
) const -> UpgradeableSetting
{
    if (info.id().isEmpty()) {
        qWarning("%s: got null model", Q_FUNC_INFO);
        return {};
    }

    auto value = m_settings.value(modelSettingName(info, legacyKey));
    if (value.isValid())
        return { UpgradeableSetting::legacy_tag, value.toString() };

    value = getModelSetting(newKey, info);
    if (!value.isNull())
        return value.toString();
    return {}; // neither a default nor an override
}

bool MySettings::isUpgradeableModelSettingSet(
    const ModelInfo &info, QLatin1StringView legacyKey, QLatin1StringView newKey
) const
{
    if (info.id().isEmpty()) {
        qWarning("%s: got null model", Q_FUNC_INFO);
        return false;
    }

    if (m_settings.contains(modelSettingName(info, legacyKey)))
        return true;

    // NOTE: unlike getUpgradeableSetting(), this ignores the default
    return m_settings.contains(modelSettingName(info, newKey));
}

auto MySettings::modelChatTemplate(const ModelInfo &info) const -> UpgradeableSetting
{
    using namespace ModelSettingsKey;
    return getUpgradeableModelSetting(info, PromptTemplate, ChatTemplate);
}

bool MySettings::isModelChatTemplateSet(const ModelInfo &info) const
{
    using namespace ModelSettingsKey;
    return isUpgradeableModelSettingSet(info, PromptTemplate, ChatTemplate);
}

auto MySettings::modelSystemMessage(const ModelInfo &info) const -> UpgradeableSetting
{
    using namespace ModelSettingsKey;
    return getUpgradeableModelSetting(info, SystemPrompt, SystemMessage);
}

bool MySettings::isModelSystemMessageSet(const ModelInfo &info) const
{
    using namespace ModelSettingsKey;
    return isUpgradeableModelSettingSet(info, SystemPrompt, SystemMessage);
}

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

bool MySettings::setUpgradeableModelSetting(
    const ModelInfo &info, const QString &value, QLatin1StringView legacyKey, QLatin1StringView newKey
) {
    if (info.id().isEmpty()) {
        qWarning("%s: got null model", Q_FUNC_INFO);
        return false;
    }

    auto legacyModelKey = modelSettingName(info, legacyKey);
    auto newModelKey    = modelSettingName(info, newKey   );
    bool changed = false;
    if (m_settings.contains(legacyModelKey)) {
        m_settings.remove(legacyModelKey);
        changed = true;
    }
    auto oldValue = m_settings.value(newModelKey);
    if (!oldValue.isValid() || oldValue.toString() != value) {
        m_settings.setValue(newModelKey, value);
        changed = true;
    }
    return changed;
}

bool MySettings::resetUpgradeableModelSetting(
    const ModelInfo &info, QLatin1StringView legacyKey, QLatin1StringView newKey
) {
    if (info.id().isEmpty()) {
        qWarning("%s: got null model", Q_FUNC_INFO);
        return false;
    }

    auto legacyModelKey = modelSettingName(info, legacyKey);
    auto newModelKey    = modelSettingName(info, newKey   );
    bool changed        = false;
    if (m_settings.contains(legacyModelKey)) {
        m_settings.remove(legacyModelKey);
        changed = true;
    }
    if (m_settings.contains(newModelKey)) {
        m_settings.remove(newModelKey);
        changed = true;
    }
    return changed;
}

void MySettings::setModelChatTemplate(const ModelInfo &info, const QString &value)
{
    using namespace ModelSettingsKey;
    if (setUpgradeableModelSetting(info, value, PromptTemplate, ChatTemplate))
        emit chatTemplateChanged(info);
}

void MySettings::resetModelChatTemplate(const ModelInfo &info)
{
    using namespace ModelSettingsKey;
    if (resetUpgradeableModelSetting(info, PromptTemplate, ChatTemplate))
        emit chatTemplateChanged(info);
}

void MySettings::setModelSystemMessage(const ModelInfo &info, const QString &value)
{
    using namespace ModelSettingsKey;
    if (setUpgradeableModelSetting(info, value, SystemPrompt, SystemMessage))
        emit systemMessageChanged(info);
}

void MySettings::resetModelSystemMessage(const ModelInfo &info)
{
    using namespace ModelSettingsKey;
    if (resetUpgradeableModelSetting(info, SystemPrompt, SystemMessage))
        emit systemMessageChanged(info);
}

void MySettings::setModelChatNamePrompt(const ModelInfo &info, const QString &value, bool force)
{
    setModelSetting("chatNamePrompt", info, value, force, true);
}

void MySettings::setModelSuggestedFollowUpPrompt(const ModelInfo &info, const QString &value, bool force)
{
    setModelSetting("suggestedFollowUpPrompt", info, value, force, true);
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

bool        MySettings::systemTray() const              { return getBasicSetting("systemTray"              ).toBool(); }
bool        MySettings::serverChat() const              { return getBasicSetting("serverChat"              ).toBool(); }
int         MySettings::networkPort() const             { return getBasicSetting("networkPort"             ).toInt(); }
QString     MySettings::userDefaultModel() const        { return getBasicSetting("userDefaultModel"        ).toString(); }
QString     MySettings::lastVersionStarted() const      { return getBasicSetting("lastVersionStarted"      ).toString(); }
int         MySettings::localDocsChunkSize() const      { return getBasicSetting("localdocs/chunkSize"     ).toInt(); }
int         MySettings::localDocsRetrievalSize() const  { return getBasicSetting("localdocs/retrievalSize" ).toInt(); }
bool        MySettings::localDocsShowReferences() const { return getBasicSetting("localdocs/showReferences").toBool(); }
QStringList MySettings::localDocsFileExtensions() const { return getBasicSetting("localdocs/fileExtensions").toStringList(); }
bool        MySettings::localDocsUseRemoteEmbed() const { return getBasicSetting("localdocs/useRemoteEmbed").toBool(); }
QString     MySettings::localDocsNomicAPIKey() const    { return getBasicSetting("localdocs/nomicAPIKey"   ).toString(); }
QString     MySettings::localDocsEmbedDevice() const    { return getBasicSetting("localdocs/embedDevice"   ).toString(); }
QString     MySettings::networkAttribution() const      { return getBasicSetting("network/attribution"     ).toString(); }

ChatTheme      MySettings::chatTheme() const      { return ChatTheme     (getEnumSetting("chatTheme", chatThemeNames)); }
FontSize       MySettings::fontSize() const       { return FontSize      (getEnumSetting("fontSize",  fontSizeNames)); }
SuggestionMode MySettings::suggestionMode() const { return SuggestionMode(getEnumSetting("suggestionMode", suggestionModeNames)); }

void MySettings::setSystemTray(bool value)                            { setBasicSetting("systemTray",               value); }
void MySettings::setServerChat(bool value)                            { setBasicSetting("serverChat",               value); }
void MySettings::setNetworkPort(int value)                            { setBasicSetting("networkPort",              value); }
void MySettings::setUserDefaultModel(const QString &value)            { setBasicSetting("userDefaultModel",         value); }
void MySettings::setLastVersionStarted(const QString &value)          { setBasicSetting("lastVersionStarted",       value); }
void MySettings::setLocalDocsChunkSize(int value)                     { setBasicSetting("localdocs/chunkSize",      value, "localDocsChunkSize"); }
void MySettings::setLocalDocsRetrievalSize(int value)                 { setBasicSetting("localdocs/retrievalSize",  value, "localDocsRetrievalSize"); }
void MySettings::setLocalDocsShowReferences(bool value)               { setBasicSetting("localdocs/showReferences", value, "localDocsShowReferences"); }
void MySettings::setLocalDocsFileExtensions(const QStringList &value) { setBasicSetting("localdocs/fileExtensions", value, "localDocsFileExtensions"); }
void MySettings::setLocalDocsUseRemoteEmbed(bool value)               { setBasicSetting("localdocs/useRemoteEmbed", value, "localDocsUseRemoteEmbed"); }
void MySettings::setLocalDocsNomicAPIKey(const QString &value)        { setBasicSetting("localdocs/nomicAPIKey",    value, "localDocsNomicAPIKey"); }
void MySettings::setLocalDocsEmbedDevice(const QString &value)        { setBasicSetting("localdocs/embedDevice",    value, "localDocsEmbedDevice"); }
void MySettings::setNetworkAttribution(const QString &value)          { setBasicSetting("network/attribution",      value, "networkAttribution"); }

void MySettings::setChatTheme(ChatTheme value)           { setBasicSetting("chatTheme",      chatThemeNames     .value(int(value))); }
void MySettings::setFontSize(FontSize value)             { setBasicSetting("fontSize",       fontSizeNames      .value(int(value))); }
void MySettings::setSuggestionMode(SuggestionMode value) { setBasicSetting("suggestionMode", suggestionModeNames.value(int(value))); }

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
    if (device() != value) {
        m_settings.setValue("device", value);
        emit deviceChanged();
    }
}

bool MySettings::forceMetal() const
{
    return m_forceMetal;
}

void MySettings::setForceMetal(bool value)
{
    if (m_forceMetal != value) {
        m_forceMetal = value;
        emit forceMetalChanged(value);
    }
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

QString MySettings::languageAndLocale() const
{
    auto value = m_settings.value("languageAndLocale");
    if (!value.isValid())
        return defaults::languageAndLocale;
    return value.toString();
}

QString MySettings::filePathForLocale(const QLocale &locale)
{
    // Check and see if we have a translation for the chosen locale and set it if possible otherwise
    // we return the filepath for the 'en_US' translation
    QStringList uiLanguages = locale.uiLanguages();
    for (int i = 0; i < uiLanguages.size(); ++i)
        uiLanguages[i].replace('-', '_');

    // Scan this directory for files named like gpt4all_%1.qm that match and if so return them first
    // this is the model download directory and it can be used by translation developers who are
    // trying to test their translations by just compiling the translation with the lrelease tool
    // rather than having to recompile all of GPT4All
    QString directory = modelPath();
    for (const QString &bcp47Name : uiLanguages) {
        QString filePath = u"%1/gpt4all_%2.qm"_s.arg(directory, bcp47Name);
        QFileInfo filePathInfo(filePath);
        if (filePathInfo.exists()) return filePath;
    }

    // Now scan the internal built-in translations
    for (QString bcp47Name : uiLanguages) {
        QString filePath = u":/i18n/gpt4all_%1.qm"_s.arg(bcp47Name);
        QFileInfo filePathInfo(filePath);
        if (filePathInfo.exists()) return filePath;
    }
    return u":/i18n/gpt4all_en_US.qm"_s;
}

void MySettings::setLanguageAndLocale(const QString &bcp47Name)
{
    if (!bcp47Name.isEmpty() && languageAndLocale() != bcp47Name)
        m_settings.setValue("languageAndLocale", bcp47Name);

    // When the app is started this method is called with no bcp47Name given which sets the translation
    // to either the default which is the system locale or the one explicitly set by the user previously.
    QLocale locale;
    const QString l = languageAndLocale();
    if (l == "System Locale")
        locale = QLocale::system();
    else
        locale = QLocale(l);

    // If we previously installed a translator, then remove it
    if (m_translator) {
        if (!qGuiApp->removeTranslator(m_translator.get())) {
            qDebug() << "ERROR: Failed to remove the previous translator";
        } else {
            m_translator.reset();
        }
    }

    // We expect that the translator was removed and is now a nullptr
    Q_ASSERT(!m_translator);

    const QString filePath = filePathForLocale(locale);
    if (!m_translator) {
        // Create a new translator object on the heap
        m_translator = std::make_unique<QTranslator>(this);
        bool success = m_translator->load(filePath);
        Q_ASSERT(success);
        if (!success) {
            qDebug() << "ERROR: Failed to load translation file:" << filePath;
            m_translator.reset();
        }

        // If we've successfully loaded it, then try and install it
        if (!qGuiApp->installTranslator(m_translator.get())) {
            qDebug() << "ERROR: Failed to install the translator:" << filePath;
            m_translator.reset();
        }
    }

    // Finally, set the locale whether we have a translation or not
    QLocale::setDefault(locale);
    emit languageAndLocaleChanged();
}
