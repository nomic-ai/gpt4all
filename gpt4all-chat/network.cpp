#include "network.h"
#include "llm.h"
#include "sysinfo.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QNetworkRequest>
#include <QScreen>

//#define DEBUG

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
    , m_isActive(false)
    , m_usageStatsActive(false)
    , m_shouldSendStartup(false)
{
    QSettings settings;
    settings.sync();
    m_uniqueId = settings.value("uniqueId", generateUniqueId()).toString();
    settings.setValue("uniqueId", m_uniqueId);
    settings.sync();
    m_isActive = settings.value("network/isActive", false).toBool();
    if (m_isActive)
        sendHealth();
    m_usageStatsActive = settings.value("network/usageStatsActive", false).toBool();
    if (m_usageStatsActive)
        sendIpify();
    connect(&m_networkManager, &QNetworkAccessManager::sslErrors, this,
        &Network::handleSslErrors);
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

void Network::setUsageStatsActive(bool b)
{
    QSettings settings;
    settings.setValue("network/usageStatsActive", b);
    settings.sync();
    m_usageStatsActive = b;
    emit usageStatsActiveChanged();
    if (!m_usageStatsActive)
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
    if (!m_isActive)
        return false;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) {
        qDebug() << "Couldn't parse: " << json << err.errorString();
        return false;
    }

    Q_ASSERT(doc.isObject());
    Q_ASSERT(LLM::globalInstance()->chatListModel()->currentChat());
    QJsonObject object = doc.object();
    object.insert("source", "gpt4all-chat");
    object.insert("agent_id", LLM::globalInstance()->chatListModel()->currentChat()->modelName());
    object.insert("submitter_id", m_uniqueId);
    object.insert("ingest_id", ingestId);

    QSettings settings;
    settings.sync();
    QString attribution = settings.value("network/attribution", QString()).toString();
    if (!attribution.isEmpty())
        object.insert("network/attribution", attribution);

    QString promptTemplate = settings.value("promptTemplate", QString()).toString();
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
    printf("%s\n", qPrintable(document.toJson(QJsonDocument::Indented)));
    fflush(stdout);
#endif

    jsonReply->deleteLater();
}

void Network::handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    QUrl url = reply->request().url();
    for (auto e : errors)
        qWarning() << "ERROR: Received ssl error:" << e.errorString() << "for" << url;
}

void Network::sendOptOut()
{
    QJsonObject properties;
    properties.insert("token", "ce362e568ddaee16ed243eaffb5860a2");
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
    if (!m_usageStatsActive)
        return;
    sendMixpanelEvent("model_load");
}

void Network::sendResetContext(int conversationLength)
{
    if (!m_usageStatsActive)
        return;

    KeyValue kv;
    kv.key = QString("length");
    kv.value = QJsonValue(conversationLength);
    sendMixpanelEvent("reset_context", QVector<KeyValue>{kv});
}

void Network::sendStartup()
{
    if (!m_usageStatsActive)
        return;
    m_shouldSendStartup = true;
    if (m_ipify.isEmpty())
        return; // when it completes it will send
    sendMixpanelEvent("startup");
}

void Network::sendCheckForUpdates()
{
    if (!m_usageStatsActive)
        return;
    sendMixpanelEvent("check_for_updates");
}

void Network::sendModelDownloaderDialog()
{
    if (!m_usageStatsActive)
        return;
    sendMixpanelEvent("download_dialog");
}

void Network::sendInstallModel(const QString &model)
{
    if (!m_usageStatsActive)
        return;
    KeyValue kv;
    kv.key = QString("model");
    kv.value = QJsonValue(model);
    sendMixpanelEvent("install_model", QVector<KeyValue>{kv});
}

void Network::sendRemoveModel(const QString &model)
{
    if (!m_usageStatsActive)
        return;
    KeyValue kv;
    kv.key = QString("model");
    kv.value = QJsonValue(model);
    sendMixpanelEvent("remove_model", QVector<KeyValue>{kv});
}

void Network::sendDownloadStarted(const QString &model)
{
    if (!m_usageStatsActive)
        return;
    KeyValue kv;
    kv.key = QString("model");
    kv.value = QJsonValue(model);
    sendMixpanelEvent("download_started", QVector<KeyValue>{kv});
}

void Network::sendDownloadCanceled(const QString &model)
{
    if (!m_usageStatsActive)
        return;
    KeyValue kv;
    kv.key = QString("model");
    kv.value = QJsonValue(model);
    sendMixpanelEvent("download_canceled", QVector<KeyValue>{kv});
}

void Network::sendDownloadError(const QString &model, int code, const QString &errorString)
{
    if (!m_usageStatsActive)
        return;
    KeyValue kv;
    kv.key = QString("model");
    kv.value = QJsonValue(model);
    KeyValue kvCode;
    kvCode.key = QString("code");
    kvCode.value = QJsonValue(code);
    KeyValue kvError;
    kvError.key = QString("error");
    kvError.value = QJsonValue(errorString);
    sendMixpanelEvent("download_error", QVector<KeyValue>{kv, kvCode, kvError});
}

void Network::sendDownloadFinished(const QString &model, bool success)
{
    if (!m_usageStatsActive)
        return;
    KeyValue kv;
    kv.key = QString("model");
    kv.value = QJsonValue(model);
    KeyValue kvSuccess;
    kvSuccess.key = QString("success");
    kvSuccess.value = QJsonValue(success);
    sendMixpanelEvent("download_finished", QVector<KeyValue>{kv, kvSuccess});
}

void Network::sendSettingsDialog()
{
    if (!m_usageStatsActive)
        return;
    sendMixpanelEvent("settings_dialog");
}

void Network::sendNetworkToggled(bool isActive)
{
    if (!m_usageStatsActive)
        return;
    KeyValue kv;
    kv.key = QString("isActive");
    kv.value = QJsonValue(isActive);
    sendMixpanelEvent("network_toggled", QVector<KeyValue>{kv});
}

void Network::sendSaveChatsToggled(bool isActive)
{
    if (!m_usageStatsActive)
        return;
    KeyValue kv;
    kv.key = QString("isActive");
    kv.value = QJsonValue(isActive);
    sendMixpanelEvent("savechats_toggled", QVector<KeyValue>{kv});
}

void Network::sendNewChat(int count)
{
    if (!m_usageStatsActive)
        return;
    KeyValue kv;
    kv.key = QString("number_of_chats");
    kv.value = QJsonValue(count);
    sendMixpanelEvent("new_chat", QVector<KeyValue>{kv});
}

void Network::sendRemoveChat()
{
    if (!m_usageStatsActive)
        return;
    sendMixpanelEvent("remove_chat");
}

void Network::sendRenameChat()
{
    if (!m_usageStatsActive)
        return;
    sendMixpanelEvent("rename_chat");
}

void Network::sendChatStarted()
{
    if (!m_usageStatsActive)
        return;
    sendMixpanelEvent("chat_started");
}

void Network::sendRecalculatingContext(int conversationLength)
{
    if (!m_usageStatsActive)
        return;

    KeyValue kv;
    kv.key = QString("length");
    kv.value = QJsonValue(conversationLength);
    sendMixpanelEvent("recalc_context", QVector<KeyValue>{kv});
}

void Network::sendNonCompatHardware()
{
    if (!m_usageStatsActive)
        return;
    sendMixpanelEvent("noncompat_hardware");
}

void Network::sendMixpanelEvent(const QString &ev, const QVector<KeyValue> &values)
{
    if (!m_usageStatsActive)
        return;

    Q_ASSERT(LLM::globalInstance()->chatListModel()->currentChat());
    QJsonObject properties;
    properties.insert("token", "ce362e568ddaee16ed243eaffb5860a2");
    properties.insert("time", QDateTime::currentSecsSinceEpoch());
    properties.insert("distinct_id", m_uniqueId);
    properties.insert("$insert_id", generateUniqueId());
    properties.insert("$os", QSysInfo::prettyProductName());
    if (!m_ipify.isEmpty())
        properties.insert("ip", m_ipify);
    properties.insert("name", QCoreApplication::applicationName() + " v"
        + QCoreApplication::applicationVersion());
    properties.insert("model", LLM::globalInstance()->chatListModel()->currentChat()->modelName());

    // Some additional startup information
    if (ev == "startup") {
        const QSize display = QGuiApplication::primaryScreen()->size();
        properties.insert("display", QString("%1x%2").arg(display.width()).arg(display.height()));
        properties.insert("ram", getSystemTotalRAM());
#if defined(__x86_64__) || defined(__i386__)
        properties.insert("avx", bool(__builtin_cpu_supports("avx")));
        properties.insert("avx2", bool(__builtin_cpu_supports("avx2")));
        properties.insert("fma", bool(__builtin_cpu_supports("fma")));
#endif
#if defined(Q_OS_MAC)
        properties.insert("cpu", QString::fromStdString(getCPUModel()));
#endif
    }

    for (auto p : values)
        properties.insert(p.key, p.value);

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
    if (!m_usageStatsActive || !m_ipify.isEmpty())
        return;

    QUrl ipifyUrl("https://api.ipify.org");
    QNetworkRequest request(ipifyUrl);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);
    QNetworkReply *reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, &Network::handleIpifyFinished);
}

void Network::sendMixpanel(const QByteArray &json, bool isOptOut)
{
    if (!m_usageStatsActive && !isOptOut)
        return;

    QUrl trackUrl("https://api.mixpanel.com/track");
    QNetworkRequest request(trackUrl);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *trackReply = m_networkManager.post(request, json);
    connect(trackReply, &QNetworkReply::finished, this, &Network::handleMixpanelFinished);
}

void Network::handleIpifyFinished()
{
    Q_ASSERT(m_usageStatsActive);
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
    m_ipify = qPrintable(reply->readAll());
#if defined(DEBUG)
    printf("ipify finished %s\n", m_ipify.toLatin1().constData());
    fflush(stdout);
#endif
    reply->deleteLater();

    if (m_shouldSendStartup)
        sendStartup();
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
