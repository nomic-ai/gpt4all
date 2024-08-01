#ifndef BRAVESEARCH_H
#define BRAVESEARCH_H

#include "tool.h"

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class BraveAPIWorker : public QObject {
    Q_OBJECT
public:
    BraveAPIWorker()
        : QObject(nullptr)
        , m_networkManager(nullptr) {}
    virtual ~BraveAPIWorker() {}

    QString response() const { return m_response; }

public Q_SLOTS:
    void request(const QString &apiKey, const QString &query, int count);

Q_SIGNALS:
    void finished();

private Q_SLOTS:
    void handleFinished();
    void handleErrorOccurred(QNetworkReply::NetworkError code);

private:
    QNetworkAccessManager *m_networkManager;
    QString m_response;
};

class BraveSearch : public Tool {
    Q_OBJECT
public:
    BraveSearch() : Tool() {}
    virtual ~BraveSearch() {}

    QString run(const QJsonObject &parameters, qint64 timeout = 2000) override;
};

#endif // BRAVESEARCH_H
