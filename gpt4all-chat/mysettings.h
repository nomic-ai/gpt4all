#ifndef MYSETTINGS_H
#define MYSETTINGS_H

#include <QObject>
#include <QMutex>

#include "modellist.h"

class MySettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int threadCount READ threadCount WRITE setThreadCount NOTIFY threadCountChanged)
    Q_PROPERTY(bool saveChats READ saveChats WRITE setSaveChats NOTIFY saveChatsChanged)
    Q_PROPERTY(bool saveChatGPTChats READ saveChatGPTChats WRITE setSaveChatGPTChats NOTIFY saveChatGPTChatsChanged)
    Q_PROPERTY(bool serverChat READ serverChat WRITE setServerChat NOTIFY serverChatChanged)
    Q_PROPERTY(QString modelPath READ modelPath WRITE setModelPath NOTIFY modelPathChanged)
    Q_PROPERTY(QString userDefaultModel READ userDefaultModel WRITE setUserDefaultModel NOTIFY userDefaultModelChanged)
    Q_PROPERTY(QString chatTheme READ chatTheme WRITE setChatTheme NOTIFY chatThemeChanged)
    Q_PROPERTY(QString fontSize READ fontSize WRITE setFontSize NOTIFY fontSizeChanged)
    Q_PROPERTY(bool forceMetal READ forceMetal WRITE setForceMetal NOTIFY forceMetalChanged)
    Q_PROPERTY(QString lastVersionStarted READ lastVersionStarted WRITE setLastVersionStarted NOTIFY lastVersionStartedChanged)
    Q_PROPERTY(int localDocsChunkSize READ localDocsChunkSize WRITE setLocalDocsChunkSize NOTIFY localDocsChunkSizeChanged)
    Q_PROPERTY(int localDocsRetrievalSize READ localDocsRetrievalSize WRITE setLocalDocsRetrievalSize NOTIFY localDocsRetrievalSizeChanged)
    Q_PROPERTY(bool localDocsShowReferences READ localDocsShowReferences WRITE setLocalDocsShowReferences NOTIFY localDocsShowReferencesChanged)
    Q_PROPERTY(QString networkAttribution READ networkAttribution WRITE setNetworkAttribution NOTIFY networkAttributionChanged)
    Q_PROPERTY(bool networkIsActive READ networkIsActive WRITE setNetworkIsActive NOTIFY networkIsActiveChanged)
    Q_PROPERTY(bool networkUsageStatsActive READ networkUsageStatsActive WRITE setNetworkUsageStatsActive NOTIFY networkUsageStatsActiveChanged)

public:
    static MySettings *globalInstance();

    // Restore methods
    Q_INVOKABLE void restoreModelDefaults(const ModelInfo &model);
    Q_INVOKABLE void restoreApplicationDefaults();
    Q_INVOKABLE void restoreLocalDocsDefaults();

    // Model/Character settings
    void eraseModel(const ModelInfo &m);
    QString modelName(const ModelInfo &m) const;
    Q_INVOKABLE void setModelName(const ModelInfo &m, const QString &name, bool force = false);
    QString modelFilename(const ModelInfo &m) const;
    Q_INVOKABLE void setModelFilename(const ModelInfo &m, const QString &filename, bool force = false);
    double modelTemperature(const ModelInfo &m) const;
    Q_INVOKABLE void setModelTemperature(const ModelInfo &m, double t, bool force = false);
    double modelTopP(const ModelInfo &m) const;
    Q_INVOKABLE void setModelTopP(const ModelInfo &m, double p, bool force = false);
    int modelTopK(const ModelInfo &m) const;
    Q_INVOKABLE void setModelTopK(const ModelInfo &m, int k, bool force = false);
    int modelMaxLength(const ModelInfo &m) const;
    Q_INVOKABLE void setModelMaxLength(const ModelInfo &m, int l, bool force = false);
    int modelPromptBatchSize(const ModelInfo &m) const;
    Q_INVOKABLE void setModelPromptBatchSize(const ModelInfo &m, int s, bool force = false);
    double modelRepeatPenalty(const ModelInfo &m) const;
    Q_INVOKABLE void setModelRepeatPenalty(const ModelInfo &m, double p, bool force = false);
    int modelRepeatPenaltyTokens(const ModelInfo &m) const;
    Q_INVOKABLE void setModelRepeatPenaltyTokens(const ModelInfo &m, int t, bool force = false);
    QString modelPromptTemplate(const ModelInfo &m) const;
    Q_INVOKABLE void setModelPromptTemplate(const ModelInfo &m, const QString &t, bool force = false);
    QString modelSystemPrompt(const ModelInfo &m) const;
    Q_INVOKABLE void setModelSystemPrompt(const ModelInfo &m, const QString &p, bool force = false);

    // Application settings
    int threadCount() const;
    void setThreadCount(int c);
    bool saveChats() const;
    void setSaveChats(bool b);
    bool saveChatGPTChats() const;
    void setSaveChatGPTChats(bool b);
    bool serverChat() const;
    void setServerChat(bool b);
    QString modelPath() const;
    void setModelPath(const QString &p);
    QString userDefaultModel() const;
    void setUserDefaultModel(const QString &u);
    QString chatTheme() const;
    void setChatTheme(const QString &u);
    QString fontSize() const;
    void setFontSize(const QString &u);
    bool forceMetal() const;
    void setForceMetal(bool b);

    // Release/Download settings
    QString lastVersionStarted() const;
    void setLastVersionStarted(const QString &v);

    // Localdocs settings
    int localDocsChunkSize() const;
    void setLocalDocsChunkSize(int s);
    int localDocsRetrievalSize() const;
    void setLocalDocsRetrievalSize(int s);
    bool localDocsShowReferences() const;
    void setLocalDocsShowReferences(bool b);

    // Network settings
    QString networkAttribution() const;
    void setNetworkAttribution(const QString &a);
    bool networkIsActive() const;
    void setNetworkIsActive(bool b);
    bool networkUsageStatsActive() const;
    void setNetworkUsageStatsActive(bool b);

    QString attemptModelLoad() const;
    void setAttemptModelLoad(const QString &modelFile);

Q_SIGNALS:
    void nameChanged(const ModelInfo &model);
    void filenameChanged(const ModelInfo &model);
    void temperatureChanged(const ModelInfo &model);
    void topPChanged(const ModelInfo &model);
    void topKChanged(const ModelInfo &model);
    void maxLengthChanged(const ModelInfo &model);
    void promptBatchSizeChanged(const ModelInfo &model);
    void repeatPenaltyChanged(const ModelInfo &model);
    void repeatPenaltyTokensChanged(const ModelInfo &model);
    void promptTemplateChanged(const ModelInfo &model);
    void systemPromptChanged(const ModelInfo &model);
    void threadCountChanged();
    void saveChatsChanged();
    void saveChatGPTChatsChanged();
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
    void networkAttributionChanged();
    void networkIsActiveChanged();
    void networkUsageStatsActiveChanged();
    void attemptModelLoadChanged();

private:
    bool m_forceMetal;

private:
    explicit MySettings();
    ~MySettings() {}
    friend class MyPrivateSettings;
};

#endif // MYSETTINGS_H
