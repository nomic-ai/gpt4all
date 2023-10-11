#ifndef NETWORK_H
#define NETWORK_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonValue>

struct KeyValue {
    QString key;
    QJsonValue value;
};

class Network : public QObject
{
    Q_OBJECT
public:
    static Network *globalInstance();

    Q_INVOKABLE QString generateUniqueId() const;
    Q_INVOKABLE bool sendConversation(const QString &ingestId, const QString &conversation);

Q_SIGNALS:
    void healthCheckFailed(int code);

public Q_SLOTS:
    void sendOptOut();
    void sendModelLoaded();
    void sendStartup();
    void sendCheckForUpdates();
    Q_INVOKABLE void sendModelDownloaderDialog();
    Q_INVOKABLE void sendResetContext(int conversationLength);
    void sendInstallModel(const QString &model);
    void sendRemoveModel(const QString &model);
    void sendDownloadStarted(const QString &model);
    void sendDownloadCanceled(const QString &model);
    void sendDownloadError(const QString &model, int code, const QString &errorString);
    void sendDownloadFinished(const QString &model, bool success);
    Q_INVOKABLE void sendSettingsDialog();
    Q_INVOKABLE void sendNetworkToggled(bool active);
    Q_INVOKABLE void sendNewChat(int count);
    Q_INVOKABLE void sendRemoveChat();
    Q_INVOKABLE void sendRenameChat();
    Q_INVOKABLE void sendNonCompatHardware();
    void sendChatStarted();
    void sendRecalculatingContext(int conversationLength);

private Q_SLOTS:
    void handleIpifyFinished();
    void handleHealthFinished();
    void handleJsonUploadFinished();
    void handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
    void handleMixpanelFinished();
    void handleIsActiveChanged();
    void handleUsageStatsActiveChanged();

private:
    void sendHealth();
    void sendIpify();
    void sendMixpanelEvent(const QString &event, const QVector<KeyValue> &values = QVector<KeyValue>());
    void sendMixpanel(const QByteArray &json, bool isOptOut = false);
    bool packageAndSendJson(const QString &ingestId, const QString &json);

private:
    bool m_shouldSendStartup;
    QString m_ipify;
    QString m_uniqueId;
    QNetworkAccessManager m_networkManager;
    QVector<QNetworkReply*> m_activeUploads;

private:
    explicit Network();
    ~Network() {}
    friend class MyNetwork;
};

#endif // LLM_H
