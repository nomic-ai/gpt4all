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
    ToolEnums::Error error() const { return m_error; }
    QString errorString() const { return m_errorString; }

public Q_SLOTS:
    void request(const QString &apiKey, const QString &query, int count);

Q_SIGNALS:
    void finished();

private Q_SLOTS:
    QString cleanBraveResponse(const QByteArray& jsonResponse);
    void handleFinished();
    void handleErrorOccurred(QNetworkReply::NetworkError code);

private:
    QNetworkAccessManager *m_networkManager;
    QString m_response;
    ToolEnums::Error m_error;
    QString m_errorString;
};

class BraveSearch : public Tool {
    Q_OBJECT
public:
    BraveSearch() : Tool(), m_error(ToolEnums::Error::NoError) {}
    virtual ~BraveSearch() {}

    QString run(const QJsonObject &parameters, qint64 timeout = 2000) override;
    ToolEnums::Error error() const override { return m_error; }
    QString errorString() const override { return m_errorString; }

    QString name() const override { return tr("Web Search"); }
    QString description() const override { return tr("Search the web"); }
    QString function() const override { return "web_search"; }
    QJsonObject paramSchema() const override;
    QJsonObject exampleParams() const override;
    bool isBuiltin() const override { return true; }
    ToolEnums::UsageMode usageMode() const override;
    bool excerpts() const override { return true; }

private:
    ToolEnums::Error m_error;
    QString m_errorString;
};

#endif // BRAVESEARCH_H
