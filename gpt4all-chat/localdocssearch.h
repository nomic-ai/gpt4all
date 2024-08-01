#ifndef LOCALDOCSSEARCH_H
#define LOCALDOCSSEARCH_H

#include "tool.h"

#include <QObject>
#include <QString>

class LocalDocsWorker : public QObject {
    Q_OBJECT
public:
    LocalDocsWorker();
    virtual ~LocalDocsWorker() {}

    QString response() const { return m_response; }

    void request(const QList<QString> &collections, const QString &text, int count);

Q_SIGNALS:
    void requestRetrieveFromDB(const QList<QString> &collections, const QString &text, int count, QString &jsonResponse);
    void finished();

private:
    QString m_response;
};

class LocalDocsSearch : public Tool {
    Q_OBJECT
public:
    LocalDocsSearch() : Tool() {}
    virtual ~LocalDocsSearch() {}

    QString run(const QJsonObject &parameters, qint64 timeout = 2000) override;
};

#endif // LOCALDOCSSEARCH_H
