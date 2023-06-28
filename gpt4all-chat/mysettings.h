#ifndef MYSETTINGS_H
#define MYSETTINGS_H

#include <QObject>
#include <QMutex>

class MySettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double temperature READ temperature WRITE setTemperature NOTIFY temperatureChanged)
    Q_PROPERTY(double topP READ topP WRITE setTopP NOTIFY topPChanged)
    Q_PROPERTY(int topK READ topK WRITE setTopK NOTIFY topKChanged)
    Q_PROPERTY(int maxLength READ maxLength WRITE setMaxLength NOTIFY maxLengthChanged)
    Q_PROPERTY(int promptBatchSize READ promptBatchSize WRITE setPromptBatchSize NOTIFY promptBatchSizeChanged)
    Q_PROPERTY(double repeatPenalty READ repeatPenalty WRITE setRepeatPenalty NOTIFY repeatPenaltyChanged)
    Q_PROPERTY(int repeatPenaltyTokens READ repeatPenaltyTokens WRITE setRepeatPenaltyTokens NOTIFY repeatPenaltyTokensChanged)
    Q_PROPERTY(QString promptTemplate READ promptTemplate WRITE setPromptTemplate NOTIFY promptTemplateChanged)
    Q_PROPERTY(int threadCount READ threadCount WRITE setThreadCount NOTIFY threadCountChanged)
    Q_PROPERTY(bool saveChats READ saveChats WRITE setSaveChats NOTIFY saveChatsChanged)
    Q_PROPERTY(bool saveChatGPTChats READ saveChatGPTChats WRITE setSaveChatGPTChats NOTIFY saveChatGPTChatsChanged)
    Q_PROPERTY(bool serverChat READ serverChat WRITE setServerChat NOTIFY serverChatChanged)
    Q_PROPERTY(QString modelPath READ modelPath WRITE setModelPath NOTIFY modelPathChanged)
    Q_PROPERTY(QString userDefaultModel READ userDefaultModel WRITE setUserDefaultModel NOTIFY userDefaultModelChanged)
    Q_PROPERTY(bool forceMetal READ forceMetal WRITE setForceMetal NOTIFY forceMetalChanged)

public:
    static MySettings *globalInstance();

    // Restore methods
    Q_INVOKABLE void restoreGenerationDefaults();
    Q_INVOKABLE void restoreApplicationDefaults();

    // Generation settings
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
    double repeatPenalty() const;
    void setRepeatPenalty(double p);
    int repeatPenaltyTokens() const;
    void setRepeatPenaltyTokens(int t);
    QString promptTemplate() const;
    void setPromptTemplate(const QString &t);

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
    bool forceMetal() const;
    void setForceMetal(bool b);

Q_SIGNALS:
    void temperatureChanged();
    void topPChanged();
    void topKChanged();
    void maxLengthChanged();
    void promptBatchSizeChanged();
    void repeatPenaltyChanged();
    void repeatPenaltyTokensChanged();
    void promptTemplateChanged();
    void threadCountChanged();
    void saveChatsChanged();
    void saveChatGPTChatsChanged();
    void serverChatChanged();
    void modelPathChanged();
    void userDefaultModelChanged();
    void forceMetalChanged(bool);

private:
    bool m_forceMetal;

private:
    explicit MySettings();
    ~MySettings() {}
    friend class MyPrivateSettings;
};

#endif // MYSETTINGS_H
