#ifndef MYSETTINGS_H
#define MYSETTINGS_H

#include "modellist.h" // IWYU pragma: keep

#include <QDateTime>
#include <QLatin1StringView>
#include <QList>
#include <QModelIndex>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTranslator>
#include <QVector>

#include <cstdint>
#include <memory>
#include <optional>

namespace MySettingsEnums {
    Q_NAMESPACE

    /* NOTE: values of these enums are used as indices for the corresponding combo boxes in
     *       ApplicationSettings.qml, as well as the corresponding name lists in mysettings.cpp */

    enum class SuggestionMode {
        LocalDocsOnly = 0,
        On            = 1,
        Off           = 2,
    };
    Q_ENUM_NS(SuggestionMode)

    enum class ChatTheme {
        Light      = 0,
        Dark       = 1,
        LegacyDark = 2,
    };
    Q_ENUM_NS(ChatTheme)

    enum class FontSize {
        Small  = 0,
        Medium = 1,
        Large  = 2,
    };
    Q_ENUM_NS(FontSize)
}
using namespace MySettingsEnums;

class MySettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int threadCount READ threadCount WRITE setThreadCount NOTIFY threadCountChanged)
    Q_PROPERTY(bool systemTray READ systemTray WRITE setSystemTray NOTIFY systemTrayChanged)
    Q_PROPERTY(bool serverChat READ serverChat WRITE setServerChat NOTIFY serverChatChanged)
    Q_PROPERTY(QString modelPath READ modelPath WRITE setModelPath NOTIFY modelPathChanged)
    Q_PROPERTY(QString userDefaultModel READ userDefaultModel WRITE setUserDefaultModel NOTIFY userDefaultModelChanged)
    Q_PROPERTY(ChatTheme chatTheme READ chatTheme WRITE setChatTheme NOTIFY chatThemeChanged)
    Q_PROPERTY(FontSize fontSize READ fontSize WRITE setFontSize NOTIFY fontSizeChanged)
    Q_PROPERTY(QString languageAndLocale READ languageAndLocale WRITE setLanguageAndLocale NOTIFY languageAndLocaleChanged)
    Q_PROPERTY(bool forceMetal READ forceMetal WRITE setForceMetal NOTIFY forceMetalChanged)
    Q_PROPERTY(QString lastVersionStarted READ lastVersionStarted WRITE setLastVersionStarted NOTIFY lastVersionStartedChanged)
    Q_PROPERTY(int localDocsChunkSize READ localDocsChunkSize WRITE setLocalDocsChunkSize NOTIFY localDocsChunkSizeChanged)
    Q_PROPERTY(int localDocsRetrievalSize READ localDocsRetrievalSize WRITE setLocalDocsRetrievalSize NOTIFY localDocsRetrievalSizeChanged)
    Q_PROPERTY(bool localDocsShowReferences READ localDocsShowReferences WRITE setLocalDocsShowReferences NOTIFY localDocsShowReferencesChanged)
    Q_PROPERTY(QStringList localDocsFileExtensions READ localDocsFileExtensions WRITE setLocalDocsFileExtensions NOTIFY localDocsFileExtensionsChanged)
    Q_PROPERTY(bool localDocsUseRemoteEmbed READ localDocsUseRemoteEmbed WRITE setLocalDocsUseRemoteEmbed NOTIFY localDocsUseRemoteEmbedChanged)
    Q_PROPERTY(QString localDocsNomicAPIKey READ localDocsNomicAPIKey WRITE setLocalDocsNomicAPIKey NOTIFY localDocsNomicAPIKeyChanged)
    Q_PROPERTY(QString localDocsEmbedDevice READ localDocsEmbedDevice WRITE setLocalDocsEmbedDevice NOTIFY localDocsEmbedDeviceChanged)
    Q_PROPERTY(QString networkAttribution READ networkAttribution WRITE setNetworkAttribution NOTIFY networkAttributionChanged)
    Q_PROPERTY(bool networkIsActive READ networkIsActive WRITE setNetworkIsActive NOTIFY networkIsActiveChanged)
    Q_PROPERTY(bool networkUsageStatsActive READ networkUsageStatsActive WRITE setNetworkUsageStatsActive NOTIFY networkUsageStatsActiveChanged)
    Q_PROPERTY(QString device READ device WRITE setDevice NOTIFY deviceChanged)
    Q_PROPERTY(QStringList deviceList MEMBER m_deviceList CONSTANT)
    Q_PROPERTY(QStringList embeddingsDeviceList MEMBER m_embeddingsDeviceList CONSTANT)
    Q_PROPERTY(int networkPort READ networkPort WRITE setNetworkPort NOTIFY networkPortChanged)
    Q_PROPERTY(SuggestionMode suggestionMode READ suggestionMode WRITE setSuggestionMode NOTIFY suggestionModeChanged)
    Q_PROPERTY(QStringList uiLanguages MEMBER m_uiLanguages CONSTANT)

private:
    explicit MySettings();
    ~MySettings() override = default;

public Q_SLOTS:
    void onModelInfoChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles = {});

public:
    static MySettings *globalInstance();

    Q_INVOKABLE static QVariant checkJinjaTemplateError(const QString &tmpl);

    // Restore methods
    Q_INVOKABLE void restoreModelDefaults(const ModelInfo &info);
    Q_INVOKABLE void restoreApplicationDefaults();
    Q_INVOKABLE void restoreLocalDocsDefaults();

    // Model/Character settings
    void eraseModel(const ModelInfo &info);
    QString modelName(const ModelInfo &info) const;
    Q_INVOKABLE void setModelName(const ModelInfo &info, const QString &name, bool force = false);
    QString modelFilename(const ModelInfo &info) const;
    Q_INVOKABLE void setModelFilename(const ModelInfo &info, const QString &filename, bool force = false);

    QString modelDescription(const ModelInfo &info) const;
    void setModelDescription(const ModelInfo &info, const QString &value, bool force = false);
    QString modelUrl(const ModelInfo &info) const;
    void setModelUrl(const ModelInfo &info, const QString &value, bool force = false);
    QString modelQuant(const ModelInfo &info) const;
    void setModelQuant(const ModelInfo &info, const QString &value, bool force = false);
    QString modelType(const ModelInfo &info) const;
    void setModelType(const ModelInfo &info, const QString &value, bool force = false);
    bool modelIsClone(const ModelInfo &info) const;
    void setModelIsClone(const ModelInfo &info, bool value, bool force = false);
    bool modelIsDiscovered(const ModelInfo &info) const;
    void setModelIsDiscovered(const ModelInfo &info, bool value, bool force = false);
    int modelLikes(const ModelInfo &info) const;
    void setModelLikes(const ModelInfo &info, int value, bool force = false);
    int modelDownloads(const ModelInfo &info) const;
    void setModelDownloads(const ModelInfo &info, int value, bool force = false);
    QDateTime modelRecency(const ModelInfo &info) const;
    void setModelRecency(const ModelInfo &info, const QDateTime &value, bool force = false);

    double modelTemperature(const ModelInfo &info) const;
    Q_INVOKABLE void setModelTemperature(const ModelInfo &info, double value, bool force = false);
    double modelTopP(const ModelInfo &info) const;
    Q_INVOKABLE void setModelTopP(const ModelInfo &info, double value, bool force = false);
    double modelMinP(const ModelInfo &info) const;
    Q_INVOKABLE void setModelMinP(const ModelInfo &info, double value, bool force = false);
    int modelTopK(const ModelInfo &info) const;
    Q_INVOKABLE void setModelTopK(const ModelInfo &info, int value, bool force = false);
    int modelMaxLength(const ModelInfo &info) const;
    Q_INVOKABLE void setModelMaxLength(const ModelInfo &info, int value, bool force = false);
    int modelPromptBatchSize(const ModelInfo &info) const;
    Q_INVOKABLE void setModelPromptBatchSize(const ModelInfo &info, int value, bool force = false);
    double modelRepeatPenalty(const ModelInfo &info) const;
    Q_INVOKABLE void setModelRepeatPenalty(const ModelInfo &info, double value, bool force = false);
    int modelRepeatPenaltyTokens(const ModelInfo &info) const;
    Q_INVOKABLE void setModelRepeatPenaltyTokens(const ModelInfo &info, int value, bool force = false);
    auto modelChatTemplate(const ModelInfo &info) const -> UpgradeableSetting;
    Q_INVOKABLE bool isModelChatTemplateSet(const ModelInfo &info) const;
    Q_INVOKABLE void setModelChatTemplate(const ModelInfo &info, const QString &value);
    Q_INVOKABLE void resetModelChatTemplate(const ModelInfo &info);
    auto modelSystemMessage(const ModelInfo &info) const -> UpgradeableSetting;
    Q_INVOKABLE bool isModelSystemMessageSet(const ModelInfo &info) const;
    Q_INVOKABLE void setModelSystemMessage(const ModelInfo &info, const QString &value);
    Q_INVOKABLE void resetModelSystemMessage(const ModelInfo &info);
    int modelContextLength(const ModelInfo &info) const;
    Q_INVOKABLE void setModelContextLength(const ModelInfo &info, int value, bool force = false);
    int modelGpuLayers(const ModelInfo &info) const;
    Q_INVOKABLE void setModelGpuLayers(const ModelInfo &info, int value, bool force = false);
    QString modelChatNamePrompt(const ModelInfo &info) const;
    Q_INVOKABLE void setModelChatNamePrompt(const ModelInfo &info, const QString &value, bool force = false);
    QString modelSuggestedFollowUpPrompt(const ModelInfo &info) const;
    Q_INVOKABLE void setModelSuggestedFollowUpPrompt(const ModelInfo &info, const QString &value, bool force = false);

    // Application settings
    int threadCount() const;
    void setThreadCount(int value);
    bool systemTray() const;
    void setSystemTray(bool value);
    bool serverChat() const;
    void setServerChat(bool value);
    QString modelPath();
    void setModelPath(const QString &value);
    QString userDefaultModel() const;
    void setUserDefaultModel(const QString &value);
    ChatTheme chatTheme() const;
    void setChatTheme(ChatTheme value);
    FontSize fontSize() const;
    void setFontSize(FontSize value);
    bool forceMetal() const;
    void setForceMetal(bool value);
    QString device();
    void setDevice(const QString &value);
    int32_t contextLength() const;
    void setContextLength(int32_t value);
    int32_t gpuLayers() const;
    void setGpuLayers(int32_t value);
    SuggestionMode suggestionMode() const;
    void setSuggestionMode(SuggestionMode value);

    QString languageAndLocale() const;
    void setLanguageAndLocale(const QString &bcp47Name = QString()); // called on startup with QString()

    // Release/Download settings
    QString lastVersionStarted() const;
    void setLastVersionStarted(const QString &value);

    // Localdocs settings
    int localDocsChunkSize() const;
    void setLocalDocsChunkSize(int value);
    int localDocsRetrievalSize() const;
    void setLocalDocsRetrievalSize(int value);
    bool localDocsShowReferences() const;
    void setLocalDocsShowReferences(bool value);
    QStringList localDocsFileExtensions() const;
    void setLocalDocsFileExtensions(const QStringList &value);
    bool localDocsUseRemoteEmbed() const;
    void setLocalDocsUseRemoteEmbed(bool value);
    QString localDocsNomicAPIKey() const;
    void setLocalDocsNomicAPIKey(const QString &value);
    QString localDocsEmbedDevice() const;
    void setLocalDocsEmbedDevice(const QString &value);

    // Network settings
    QString networkAttribution() const;
    void setNetworkAttribution(const QString &value);
    bool networkIsActive() const;
    Q_INVOKABLE bool isNetworkIsActiveSet() const;
    void setNetworkIsActive(bool value);
    bool networkUsageStatsActive() const;
    Q_INVOKABLE bool isNetworkUsageStatsActiveSet() const;
    void setNetworkUsageStatsActive(bool value);
    int networkPort() const;
    void setNetworkPort(int value);

Q_SIGNALS:
    void nameChanged(const ModelInfo &info);
    void filenameChanged(const ModelInfo &info);
    void descriptionChanged(const ModelInfo &info);
    void temperatureChanged(const ModelInfo &info);
    void topPChanged(const ModelInfo &info);
    void minPChanged(const ModelInfo &info);
    void topKChanged(const ModelInfo &info);
    void maxLengthChanged(const ModelInfo &info);
    void promptBatchSizeChanged(const ModelInfo &info);
    void contextLengthChanged(const ModelInfo &info);
    void gpuLayersChanged(const ModelInfo &info);
    void repeatPenaltyChanged(const ModelInfo &info);
    void repeatPenaltyTokensChanged(const ModelInfo &info);
    void chatTemplateChanged(const ModelInfo &info, bool fromInfo = false);
    void systemMessageChanged(const ModelInfo &info, bool fromInfo = false);
    void chatNamePromptChanged(const ModelInfo &info);
    void suggestedFollowUpPromptChanged(const ModelInfo &info);
    void threadCountChanged();
    void systemTrayChanged();
    void serverChatChanged();
    void modelPathChanged();
    void userDefaultModelChanged();
    void chatThemeChanged();
    void fontSizeChanged();
    void forceMetalChanged(bool);
    void lastVersionStartedChanged();
    void localDocsChunkSizeChanged();
    void localDocsRetrievalSizeChanged();
    void localDocsShowReferencesChanged();
    void localDocsFileExtensionsChanged();
    void localDocsUseRemoteEmbedChanged();
    void localDocsNomicAPIKeyChanged();
    void localDocsEmbedDeviceChanged();
    void networkAttributionChanged();
    void networkIsActiveChanged();
    void networkPortChanged();
    void networkUsageStatsActiveChanged();
    void attemptModelLoadChanged();
    void deviceChanged();
    void suggestionModeChanged();
    void languageAndLocaleChanged();

private:
    QVariant getBasicSetting(const QString &name) const;
    void setBasicSetting(const QString &name, const QVariant &value, std::optional<QString> signal = std::nullopt);
    int getEnumSetting(const QString &setting, const QStringList &valueNames) const;
    QVariant getModelSetting(QLatin1StringView name, const ModelInfo &info) const;
    QVariant getModelSetting(const char *name, const ModelInfo &info) const;
    void setModelSetting(QLatin1StringView name, const ModelInfo &info, const QVariant &value, bool force,
                         bool signal = false);
    void setModelSetting(const char *name, const ModelInfo &info, const QVariant &value, bool force,
                         bool signal = false);
    auto getUpgradeableModelSetting(
        const ModelInfo &info, QLatin1StringView legacyKey, QLatin1StringView newKey
    ) const -> UpgradeableSetting;
    bool isUpgradeableModelSettingSet(
        const ModelInfo &info, QLatin1StringView legacyKey, QLatin1StringView newKey
    ) const;
    bool setUpgradeableModelSetting(
        const ModelInfo &info, const QString &value, QLatin1StringView legacyKey, QLatin1StringView newKey
    );
    bool resetUpgradeableModelSetting(
        const ModelInfo &info, QLatin1StringView legacyKey, QLatin1StringView newKey
    );
    QString filePathForLocale(const QLocale &locale);

private:
    QSettings m_settings;
    bool m_forceMetal;
    const QStringList m_deviceList;
    const QStringList m_embeddingsDeviceList;
    const QStringList m_uiLanguages;
    std::unique_ptr<QTranslator> m_translator;

    friend class MyPrivateSettings;
};

#endif // MYSETTINGS_H
