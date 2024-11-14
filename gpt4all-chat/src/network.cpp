#include "network.h"

#include "chat.h"
#include "chatlistmodel.h"
#include "download.h"
#include "llm.h"
#include "localdocs.h"
#include "localdocsmodel.h"
#include "modellist.h"
#include "mysettings.h"
#include "utils.h"

#include <gpt4all-backend/llmodel.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QGlobalStatic>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibraryInfo>
#include <QNetworkRequest>
#include <QScreen>
#include <QSettings>
#include <QSize>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QSysInfo>
#include <Qt>
#include <QtGlobal>
#include <QtLogging>
#include <QUrl>
#include <QUuid>

#include <cmath>
#include <cstring>
#include <utility>

#ifdef __GLIBC__
#   include <gnu/libc-version.h>
#endif

using namespace Qt::Literals::StringLiterals;

//#define DEBUG

#define STR_(x) #x
#define STR(x) STR_(x)

static const char MIXPANEL_TOKEN[] = "ce362e568ddaee16ed243eaffb5860a2";

#ifdef __clang__
#ifdef __apple_build_version__
static const char COMPILER_NAME[] = "Apple Clang";
#else
static const char COMPILER_NAME[] = "LLVM Clang";
#endif
static const char COMPILER_VER[]  = STR(__clang_major__) "." STR(__clang_minor__) "." STR(__clang_patchlevel__);
#elif defined(_MSC_VER)
static const char COMPILER_NAME[] = "MSVC";
static const char COMPILER_VER[]  = STR(_MSC_VER) " (" STR(_MSC_FULL_VER) ")";
#elif defined(__GNUC__)
static const char COMPILER_NAME[] = "GCC";
static const char COMPILER_VER[]  = STR(__GNUC__) "." STR(__GNUC_MINOR__) "." STR(__GNUC_PATCHLEVEL__);
#endif


#if defined(Q_OS_MAC)

#include <sys/sysctl.h>
static std::optional<QString> getSysctl(const char *name)
{
    char buffer[256] = "";
    size_t bufferlen = sizeof(buffer);
    if (sysctlbyname(name, &buffer, &bufferlen, NULL, 0) < 0) {
        int err = errno;
        qWarning().nospace() << "sysctlbyname(\"" << name << "\") failed: " << strerror(err);
        return std::nullopt;
    }
    return std::make_optional<QString>(buffer);
}

static QString getCPUModel() { return getSysctl("machdep.cpu.brand_string").value_or(u"(unknown)"_s); }

#elif defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)

#ifndef _MSC_VER
static void get_cpuid(int level, int *regs)
{
    asm volatile("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3]) : "0" (level) : "memory");
}
#else
#define get_cpuid(level, regs) __cpuid(regs, level)
#endif

static QString getCPUModel()
{
    int regs[12];

    // EAX=800000000h: Get Highest Extended Function Implemented
    get_cpuid(0x80000000, regs);
    if (regs[0] < 0x80000004)
        return "(unknown)";

    // EAX=800000002h-800000004h: Processor Brand String
    get_cpuid(0x80000002, regs);
    get_cpuid(0x80000003, regs + 4);
    get_cpuid(0x80000004, regs + 8);

    char str[sizeof(regs) + 1];
    memcpy(str, regs, sizeof(regs));
    str[sizeof(regs)] = 0;

    return QString(str).trimmed();
}

#else

static QString getCPUModel() { return "(non-x86)"; }

#endif


class MyNetwork: public Network { };
Q_GLOBAL_STATIC(MyNetwork, networkInstance)
Network *Network::globalInstance()
{
    return networkInstance();
}

bool Network::isHttpUrlValid(QUrl url) {
    if (!url.isValid())
        return false;
    QString scheme(url.scheme());
    if (scheme != "http" && scheme != "https")
        return false;
    return true;
}

Network::Network()
    : QObject{nullptr}
{
    QSettings settings;
    m_uniqueId = settings.value("uniqueId", generateUniqueId()).toString();
    settings.setValue("uniqueId", m_uniqueId);
    m_sessionId = generateUniqueId();

    // allow sendMixpanel to be called from any thread
    connect(this, &Network::requestMixpanel, this, &Network::sendMixpanel, Qt::QueuedConnection);

    const auto *mySettings = MySettings::globalInstance();
    connect(mySettings, &MySettings::networkIsActiveChanged, this, &Network::handleIsActiveChanged);
    connect(mySettings, &MySettings::networkUsageStatsActiveChanged, this, &Network::handleUsageStatsActiveChanged);

    m_hasSentOptIn  = !Download::globalInstance()->isFirstStart() &&  mySettings->networkUsageStatsActive();
    m_hasSentOptOut = !Download::globalInstance()->isFirstStart() && !mySettings->networkUsageStatsActive();

    if (mySettings->networkIsActive())
        sendHealth();
    connect(&m_networkManager, &QNetworkAccessManager::sslErrors, this,
        &Network::handleSslErrors);
}

// NOTE: this won't be useful until we make it possible to change this via the settings page
void Network::handleUsageStatsActiveChanged()
{
    if (!MySettings::globalInstance()->networkUsageStatsActive())
        m_sendUsageStats = false;
}

void Network::handleIsActiveChanged()
{
    if (MySettings::globalInstance()->networkUsageStatsActive())
        sendHealth();
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

    auto *currentChat = ChatListModel::globalInstance()->currentChat();
    Q_ASSERT(currentChat);
    auto modelInfo = currentChat->modelInfo();

    Q_ASSERT(doc.isObject());
    QJsonObject object = doc.object();
    object.insert("source", "gpt4all-chat");
    object.insert("agent_id", modelInfo.filename());
    object.insert("submitter_id", m_uniqueId);
    object.insert("ingest_id", ingestId);

    QString attribution = MySettings::globalInstance()->networkAttribution();
    if (!attribution.isEmpty())
        object.insert("network/attribution", attribution);

    if (!modelInfo.id().isNull())
        if (auto tmpl = modelInfo.chatTemplate().asModern())
            object.insert("chat_template"_L1, *tmpl);

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
    connect(qGuiApp, &QCoreApplication::aboutToQuit, jsonReply, &QNetworkReply::abort);
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
    properties.insert("time", QDateTime::currentMSecsSinceEpoch());
    properties.insert("distinct_id", m_uniqueId);
    properties.insert("$insert_id", generateUniqueId());

    QJsonObject event;
    event.insert("event", "opt_out");
    event.insert("properties", properties);

    QJsonArray array;
    array.append(event);

    QJsonDocument doc;
    doc.setArray(array);
    emit requestMixpanel(doc.toJson(QJsonDocument::Compact));

#if defined(DEBUG)
    printf("%s %s\n", qPrintable("opt_out"), qPrintable(doc.toJson(QJsonDocument::Indented)));
    fflush(stdout);
#endif
}

void Network::sendStartup()
{
    const auto *mySettings = MySettings::globalInstance();
    Q_ASSERT(mySettings->isNetworkUsageStatsActiveSet());
    if (!mySettings->networkUsageStatsActive()) {
        // send a single opt-out per session after the user has made their selections,
        // unless this is a normal start (same version) and the user was already opted out
        if (!m_hasSentOptOut) {
            sendOptOut();
            m_hasSentOptOut = true;
        }
        return;
    }

    // only chance to enable usage stats is at the start of a new session
    m_sendUsageStats = true;

    const auto *display = QGuiApplication::primaryScreen();
    trackEvent("startup", {
        // Build info
        { "build_compiler",     COMPILER_NAME                                                         },
        { "build_compiler_ver", COMPILER_VER                                                          },
        { "build_abi",          QSysInfo::buildAbi()                                                  },
        { "build_cpu_arch",     QSysInfo::buildCpuArchitecture()                                      },
#ifdef __GLIBC__
        { "build_glibc_ver",    QStringLiteral(STR(__GLIBC__) "." STR(__GLIBC_MINOR__))               },
#endif
        { "qt_version",         QLibraryInfo::version().toString()                                    },
        { "qt_debug" ,          QLibraryInfo::isDebugBuild()                                          },
        { "qt_shared",          QLibraryInfo::isSharedBuild()                                         },
        // System info
        { "runtime_cpu_arch",   QSysInfo::currentCpuArchitecture()                                    },
#ifdef __GLIBC__
        { "runtime_glibc_ver",  gnu_get_libc_version()                                                },
#endif
        { "sys_kernel_type",    QSysInfo::kernelType()                                                },
        { "sys_kernel_ver",     QSysInfo::kernelVersion()                                             },
        { "sys_product_type",   QSysInfo::productType()                                               },
        { "sys_product_ver",    QSysInfo::productVersion()                                            },
#ifdef Q_OS_MAC
        { "sys_hw_model",       getSysctl("hw.model").value_or(u"(unknown)"_s)                        },
#endif
        { "$screen_dpi",        std::round(display->physicalDotsPerInch())                            },
        { "display",            u"%1x%2"_s.arg(display->size().width()).arg(display->size().height()) },
        { "ram",                LLM::globalInstance()->systemTotalRAMInGB()                           },
        { "cpu",                getCPUModel()                                                         },
        { "cpu_supports_avx2",  LLModel::Implementation::cpuSupportsAVX2()                            },
        // Datalake status
        { "datalake_active",    mySettings->networkIsActive()                                         },
    });
    sendIpify();

    // mirror opt-out logic so the ratio can be used to infer totals
    if (!m_hasSentOptIn) {
        trackEvent("opt_in");
        m_hasSentOptIn = true;
    }
}

void Network::trackChatEvent(const QString &ev, QVariantMap props)
{
    auto *curChat = ChatListModel::globalInstance()->currentChat();
    Q_ASSERT(curChat);
    if (!props.contains("model"))
        props.insert("model", curChat->modelInfo().filename());
    props.insert("device_backend", curChat->deviceBackend());
    props.insert("actualDevice", curChat->device());
    props.insert("doc_collections_enabled", curChat->collectionList().count());
    props.insert("doc_collections_total", LocalDocs::globalInstance()->localDocsModel()->rowCount());
    props.insert("datalake_active", MySettings::globalInstance()->networkIsActive());
    props.insert("using_server", curChat->isServer());
    trackEvent(ev, props);
}

void Network::trackEvent(const QString &ev, const QVariantMap &props)
{
    if (!m_sendUsageStats)
        return;

    QJsonObject properties;

    properties.insert("token", MIXPANEL_TOKEN);
    if (!props.contains("time"))
        properties.insert("time", QDateTime::currentMSecsSinceEpoch());
    properties.insert("distinct_id", m_uniqueId); // effectively a device ID
    properties.insert("$insert_id", generateUniqueId());

    if (!m_ipify.isEmpty())
        properties.insert("ip", m_ipify);

    properties.insert("$os", QSysInfo::prettyProductName());
    properties.insert("session_id", m_sessionId);
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
    emit requestMixpanel(doc.toJson(QJsonDocument::Compact));

#if defined(DEBUG)
    printf("%s %s\n", qPrintable(ev), qPrintable(doc.toJson(QJsonDocument::Indented)));
    fflush(stdout);
#endif
}

void Network::sendIpify()
{
    if (!m_sendUsageStats || !m_ipify.isEmpty())
        return;

    QUrl ipifyUrl("https://api.ipify.org");
    QNetworkRequest request(ipifyUrl);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);
    QNetworkReply *reply = m_networkManager.get(request);
    connect(qGuiApp, &QCoreApplication::aboutToQuit, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::finished, this, &Network::handleIpifyFinished);
}

void Network::sendMixpanel(const QByteArray &json)
{
    QUrl trackUrl("https://api.mixpanel.com/track");
    QNetworkRequest request(trackUrl);
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *trackReply = m_networkManager.post(request, json);
    connect(qGuiApp, &QCoreApplication::aboutToQuit, trackReply, &QNetworkReply::abort);
    connect(trackReply, &QNetworkReply::finished, this, &Network::handleMixpanelFinished);
}

void Network::handleIpifyFinished()
{
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

    trackEvent("ipify_complete");
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
    connect(qGuiApp, &QCoreApplication::aboutToQuit, healthReply, &QNetworkReply::abort);
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
