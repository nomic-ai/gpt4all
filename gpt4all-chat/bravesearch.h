#ifndef BRAVESEARCH_H
#define BRAVESEARCH_H

#include "sourceexcerpt.h"

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class BraveAPIWorker : public QObject {
    Q_OBJECT
public:
    BraveAPIWorker()
        : QObject(nullptr)
        , m_networkManager(nullptr)
        , m_topK(1) {}
    virtual ~BraveAPIWorker() {}

    QPair<QString, QList<SourceExcerpt>> response() const { return m_response; }

public Q_SLOTS:
    void request(const QString &apiKey, const QString &query, int topK);

Q_SIGNALS:
    void finished();

private Q_SLOTS:
    void handleFinished();
    void handleErrorOccurred(QNetworkReply::NetworkError code);

private:
    QNetworkAccessManager *m_networkManager;
    QPair<QString, QList<SourceExcerpt>> m_response;
    int m_topK;
};

class BraveSearch : public QObject {
    Q_OBJECT
public:
    BraveSearch()
        : QObject(nullptr) {}
    virtual ~BraveSearch() {}

    QPair<QString, QList<SourceExcerpt>> search(const QString &apiKey, const QString &query, int topK, unsigned long timeout = 2000);

Q_SIGNALS:
    void request(const QString &apiKey, const QString &query, int topK);
};

#endif // BRAVESEARCH_H
