#ifndef NETWORK_H
#define NETWORK_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class Network : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isActive READ isActive WRITE setActive NOTIFY activeChanged)
public:
    static Network *globalInstance();

    bool isActive() const { return m_isActive; }
    void setActive(bool b);

    Q_INVOKABLE QString generateUniqueId() const;
    Q_INVOKABLE bool sendConversation(const QString &ingestId, const QString &conversation);

Q_SIGNALS:
    void activeChanged();
    void healthCheckFailed(int code);

private Q_SLOTS:
    void handleHealthFinished();
    void handleJsonUploadFinished();
    void handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);

private:
    void sendHealth();
    bool packageAndSendJson(const QString &ingestId, const QString &json);

private:
    bool m_isActive;
    QString m_uniqueId;
    QNetworkAccessManager m_networkManager;
    QVector<QNetworkReply*> m_activeUploads;

private:
    explicit Network();
    ~Network() {}
    friend class MyNetwork;
};

#endif // LLM_H
