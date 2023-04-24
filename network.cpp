#include "network.h"
#include "llm.h"

#include <QCoreApplication>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QNetworkRequest>

//#define DEBUG

class MyNetwork: public Network { };
Q_GLOBAL_STATIC(MyNetwork, networkInstance)
Network *Network::globalInstance()
{
    return networkInstance();
}

Network::Network()
    : QObject{nullptr}
    , m_isActive(false)
{
    QSettings settings;
    settings.sync();
    m_uniqueId = settings.value("uniqueId", generateUniqueId()).toString();
    settings.setValue("uniqueId", m_uniqueId);
    settings.sync();
    setActive(settings.value("network/isActive", false).toBool());
}

void Network::setActive(bool b)
{
    QSettings settings;
    settings.setValue("network/isActive", b);
    settings.sync();
    m_isActive = b;
    emit activeChanged();
    if (m_isActive)
        sendHealth();
}

QString Network::generateUniqueId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

bool Network::packageAndSendJson(const QString &ingestId, const QString &json)
{
    if (!m_isActive)
        return false;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) {
        qDebug() << "Couldn't parse: " << json << err.errorString();
        return false;
    }

    Q_ASSERT(doc.isObject());
    QJsonObject object = doc.object();
    object.insert("source", "gpt4all-chat");
    object.insert("agent_id", LLM::globalInstance()->modelName());
    object.insert("submitter_id", m_uniqueId);
    object.insert("ingest_id", ingestId);

    QSettings settings;
    settings.sync();
    QString attribution = settings.value("attribution", QString()).toString();
    if (!attribution.isEmpty())
        object.insert("attribution", attribution);

    QJsonDocument newDoc;
    newDoc.setObject(object);

#if defined(DEBUG)
    printf("%s", qPrintable(newDoc.toJson(QJsonDocument::Indented)));
    fflush(stdout);
#endif
    QUrl jsonUrl("https://api.gpt4all.io/v1/ingest/chat");
    QNetworkRequest request(jsonUrl);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);
    QByteArray body(newDoc.toJson());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *jsonReply = m_networkManager.post(request, body);
    connect(jsonReply, &QNetworkReply::finished, this, &Network::handleJsonUploadFinished);
    m_activeUploads.append(jsonReply);
    return true;
}

void Network::handleJsonUploadFinished()
{
    QNetworkReply *jsonReply = qobject_cast<QNetworkReply *>(sender());
    if (!jsonReply)
        return;

    m_activeUploads.removeAll(jsonReply);

    QVariant response = jsonReply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    Q_ASSERT(response.isValid());
    bool ok;
    int code = response.toInt(&ok);
    if (!ok)
        qWarning() << "ERROR: ingest invalid response.";
    if (code != 200) {
        qWarning() << "ERROR: ingest response != 200 code:" << code;
        sendHealth();
    }

    QByteArray jsonData = jsonReply->readAll();
    QJsonParseError err;
    QJsonDocument document = QJsonDocument::fromJson(jsonData, &err);
    if (err.error != QJsonParseError::NoError) {
        qDebug() << "ERROR: Couldn't parse: " << jsonData << err.errorString();
        return;
    }

#if defined(DEBUG)
    printf("%s", qPrintable(document.toJson(QJsonDocument::Indented)));
    fflush(stdout);
#endif

    jsonReply->deleteLater();
}

bool Network::sendConversation(const QString &ingestId, const QString &conversation)
{
    return packageAndSendJson(ingestId, conversation);
}

void Network::sendHealth()
{
    QUrl healthUrl("https://api.gpt4all.io/v1/health");
    QNetworkRequest request(healthUrl);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);
    QNetworkReply *healthReply = m_networkManager.get(request);
    connect(healthReply, &QNetworkReply::finished, this, &Network::handleHealthFinished);
}

void Network::handleHealthFinished()
{
    QNetworkReply *healthReply = qobject_cast<QNetworkReply *>(sender());
    if (!healthReply)
        return;

    QVariant response = healthReply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    Q_ASSERT(response.isValid());
    bool ok;
    int code = response.toInt(&ok);
    if (!ok)
        qWarning() << "ERROR: health invalid response.";
    if (code != 200) {
        qWarning() << "ERROR: health response != 200 code:" << code;
        emit healthCheckFailed(code);
        setActive(false);
    }
    healthReply->deleteLater();
}
