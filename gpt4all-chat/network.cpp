#include "network.h"

#include "chatlistmodel.h"
#include "llm.h"
#include "localdocs.h"
#include "mysettings.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QScreen>

//#define DEBUG

static const char MIXPANEL_TOKEN[] = "ce362e568ddaee16ed243eaffb5860a2";

#if defined(Q_OS_MAC)
#include <sys/sysctl.h>
std::string getCPUModel() {
    char buffer[256];
    size_t bufferlen = sizeof(buffer);
    sysctlbyname("machdep.cpu.brand_string", &buffer, &bufferlen, NULL, 0);
    return std::string(buffer);
}
#endif

class MyNetwork: public Network { };
Q_GLOBAL_STATIC(MyNetwork, networkInstance)
Network *Network::globalInstance()
{
    return networkInstance();
}

Network::Network()
    : QObject{nullptr}
{
    QSettings settings;
    settings.sync();
    m_uniqueId = settings.value("uniqueId", generateUniqueId()).toString();
    settings.setValue("uniqueId", m_uniqueId);
    settings.sync();
    m_sessionId = generateUniqueId();
    connect(MySettings::globalInstance(), &MySettings::networkIsActiveChanged, this, &Network::handleIsActiveChanged);
    connect(MySettings::globalInstance(), &MySettings::networkUsageStatsActiveChanged, this, &Network::handleUsageStatsActiveChanged);
    if (MySettings::globalInstance()->networkIsActive())
        sendHealth();
    if (MySettings::globalInstance()->networkUsageStatsActive())
        sendIpify();
    connect(&m_networkManager, &QNetworkAccessManager::sslErrors, this,
        &Network::handleSslErrors);
}

void Network::handleIsActiveChanged()
{
    if (MySettings::globalInstance()->networkUsageStatsActive())
        sendHealth();
}

void Network::handleUsageStatsActiveChanged()
{
    if (!MySettings::globalInstance()->networkUsageStatsActive())
        sendOptOut();
    else {
        // model might be loaded already when user opt-in for first time
        sendStartup();
        sendIpify();
    }
}

QString Network::generateUniqueId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

bool Network::packageAndSendJson(const QString &ingestId, const QString &json)
{
    if (!MySettings::globalInstance()->networkIsActive())
        return false;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) {
        qDebug() << "Couldn't parse: " << json << err.errorString();
        return false;
    }

    Q_ASSERT(doc.isObject());
    Q_ASSERT(ChatListModel::globalInstance()->currentChat());
    QJsonObject object = doc.object();
    object.insert("source", "gpt4all-chat");
    object.insert("agent_id", ChatListModel::globalInstance()->currentChat()->modelInfo().filename());
    object.insert("submitter_id", m_uniqueId);
    object.insert("ingest_id", ingestId);

    QString attribution = MySettings::globalInstance()->networkAttribution();
    if (!attribution.isEmpty())
        object.insert("network/attribution", attribution);

    QString promptTemplate = ChatListModel::globalInstance()->currentChat()->modelInfo().promptTemplate();
    object.insert("prompt_template", promptTemplate);

    QJsonDocument newDoc;
    newDoc.setObject(object);

#if defined(DEBUG)
    printf("%s\n", qPrintable(newDoc.toJson(QJsonDocument::Indented)));
    fflush(stdout);
#endif

    QUrl jsonUrl("https://api.gpt4all.io/v1/ingest/chat");
    QNetworkRequest request(jsonUrl);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);
    QByteArray body(newDoc.toJson(QJsonDocument::Compact));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *jsonReply = m_networkManager.post(request, body);
    connect(qApp, &QCoreApplication::aboutToQuit, jsonReply, &QNetworkReply::abort);
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

#if defined(DEBUG)
    QByteArray jsonData = jsonReply->readAll();
    QJsonParseError err;

    QJsonDocument document = QJsonDocument::fromJson(jsonData, &err);
    if (err.error != QJsonParseError::NoError) {
        qDebug() << "ERROR: Couldn't parse: " << jsonData << err.errorString();
        return;
    }

    printf("%s\n", qPrintable(document.toJson(QJsonDocument::Indented)));
    fflush(stdout);
#endif

    jsonReply->deleteLater();
}

void Network::handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    QUrl url = reply->request().url();
    for (const auto &e : errors)
        qWarning() << "ERROR: Received ssl error:" << e.errorString() << "for" << url;
}

void Network::sendOptOut()
{
    QJsonObject properties;
    properties.insert("token", MIXPANEL_TOKEN);
    properties.insert("time", QDateTime::currentSecsSinceEpoch());
    properties.insert("distinct_id", m_uniqueId);
    properties.insert("$insert_id", generateUniqueId());

    QJsonObject event;
    event.insert("event", "opt_out");
    event.insert("properties", properties);

    QJsonArray array;
    array.append(event);

    QJsonDocument doc;
    doc.setArray(array);
    sendMixpanel(doc.toJson(QJsonDocument::Compact), true /*isOptOut*/);

#if defined(DEBUG)
    printf("%s %s\n", qPrintable("opt_out"), qPrintable(doc.toJson(QJsonDocument::Indented)));
    fflush(stdout);
#endif
}

void Network::sendModelLoaded()
{
    trackChatEvent("model_load", {
        {"requestedDevice", MySettings::globalInstance()->device()},
        {"usingServer", ChatListModel::globalInstance()->currentChat()->isServer()},
    });
}

void Network::sendStartup()
{
    const auto *display = QGuiApplication::primaryScreen();
    sendMixpanelEvent("startup", {
        {"$screen_dpi", display->physicalDotsPerInch()},
        {"display", QString("%1x%2").arg(display->size().width()).arg(display->size().height())},
        {"ram", LLM::globalInstance()->systemTotalRAMInGB()},
#if defined(Q_OS_MAC)
        {"cpu", QString::fromStdString(getCPUModel())},
#endif
    });
}

void Network::sendNetworkToggled(bool isActive)
{
    sendMixpanelEvent("network_toggled", { {"isActive", isActive} });
}

void Network::trackChatEvent(const QString &ev, QVariantMap props)
{
    const auto &curChat = ChatListModel::globalInstance()->currentChat();
    props.insert("model", curChat->modelInfo().filename());
    props.insert("actualDevice", curChat->device());
    props.insert("doc_collections_enabled", curChat->collectionList().count());
    props.insert("doc_collections_total", LocalDocs::globalInstance()->localDocsModel()->rowCount());
    sendMixpanelEvent(ev, props);
}

void Network::sendMixpanelEvent(const QString &ev, const QVariantMap &props)
{
    if (!MySettings::globalInstance()->networkUsageStatsActive())
        return;

    Q_ASSERT(ChatListModel::globalInstance()->currentChat());
    QJsonObject properties;

    // basic properties
    properties.insert("token", MIXPANEL_TOKEN);
    properties.insert("time", QDateTime::currentSecsSinceEpoch());
    properties.insert("distinct_id", m_uniqueId); // effectively a device ID
    properties.insert("$insert_id", generateUniqueId());

    // reserved properties
    if (!m_ipify.isEmpty())
        properties.insert("ip", m_ipify);

    // standard properties
    properties.insert("$os", QSysInfo::prettyProductName());
    properties.insert("$app_version_string", QCoreApplication::applicationVersion());

    // recommended properties
    properties.insert("session_id", m_sessionId);

    // custom properties
    properties.insert("name", QCoreApplication::applicationName() + " v" + QCoreApplication::applicationVersion());

    for (const auto &[key, value]: props.asKeyValueRange())
        properties.insert(key, QJsonValue::fromVariant(value));

    QJsonObject event;
    event.insert("event", ev);
    event.insert("properties", properties);

    QJsonArray array;
    array.append(event);

    QJsonDocument doc;
    doc.setArray(array);
    sendMixpanel(doc.toJson(QJsonDocument::Compact));

#if defined(DEBUG)
    printf("%s %s\n", qPrintable(ev), qPrintable(doc.toJson(QJsonDocument::Indented)));
    fflush(stdout);
#endif
}

void Network::sendIpify()
{
    if (!MySettings::globalInstance()->networkUsageStatsActive() || !m_ipify.isEmpty())
        return;

    QUrl ipifyUrl("https://api.ipify.org");
    QNetworkRequest request(ipifyUrl);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);
    QNetworkReply *reply = m_networkManager.get(request);
    connect(qApp, &QCoreApplication::aboutToQuit, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::finished, this, &Network::handleIpifyFinished);
}

void Network::sendMixpanel(const QByteArray &json, bool isOptOut)
{
    if (!MySettings::globalInstance()->networkUsageStatsActive() && !isOptOut)
        return;

    QUrl trackUrl("https://api.mixpanel.com/track");
    QNetworkRequest request(trackUrl);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *trackReply = m_networkManager.post(request, json);
    connect(qApp, &QCoreApplication::aboutToQuit, trackReply, &QNetworkReply::abort);
    connect(trackReply, &QNetworkReply::finished, this, &Network::handleMixpanelFinished);
}

void Network::handleIpifyFinished()
{
    Q_ASSERT(MySettings::globalInstance()->networkUsageStatsActive());
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;

    QVariant response = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    Q_ASSERT(response.isValid());
    bool ok;
    int code = response.toInt(&ok);
    if (!ok)
        qWarning() << "ERROR: ipify invalid response.";
    if (code != 200)
        qWarning() << "ERROR: ipify response != 200 code:" << code;
    m_ipify = reply->readAll();
#if defined(DEBUG)
    printf("ipify finished %s\n", qPrintable(m_ipify));
    fflush(stdout);
#endif
    reply->deleteLater();

    sendMixpanelEvent("ipify_finished");
}

void Network::handleMixpanelFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;

    QVariant response = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    Q_ASSERT(response.isValid());
    bool ok;
    int code = response.toInt(&ok);
    if (!ok)
        qWarning() << "ERROR: track invalid response.";
    if (code != 200)
        qWarning() << "ERROR: track response != 200 code:" << code;
#if defined(DEBUG)
    printf("mixpanel finished %s\n", qPrintable(reply->readAll()));
    fflush(stdout);
#endif
    reply->deleteLater();
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
    connect(qApp, &QCoreApplication::aboutToQuit, healthReply, &QNetworkReply::abort);
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
        MySettings::globalInstance()->setNetworkIsActive(false);
    }
    healthReply->deleteLater();
}
