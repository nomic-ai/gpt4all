#include "mysettings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

static double   default_temperature         = 0.7;
static double   default_topP                = 0.1;
static int      default_topK                = 40;
static int      default_maxLength           = 4096;
static int      default_promptBatchSize     = 128;
static double   default_repeatPenalty       = 1.18;
static int      default_repeatPenaltyTokens = 64;
static QString  default_promptTemplate      = "### Human:\n%1\n### Assistant:\n";
static int      default_threadCount         = 0;
static bool     default_saveChats           = false;
static bool     default_saveChatGPTChats    = true;
static bool     default_serverChat          = false;
static QString  default_userDefaultModel    = "Application default";
static bool     default_forceMetal          = false;

static QString defaultLocalModelsPath()
{
    QString localPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        + "/";
    QString testWritePath = localPath + QString("test_write.txt");
    QString canonicalLocalPath = QFileInfo(localPath).canonicalFilePath() + "/";
    QDir localDir(localPath);
    if (!localDir.exists()) {
        if (!localDir.mkpath(localPath)) {
            qWarning() << "ERROR: Local download directory can't be created:" << canonicalLocalPath;
            return canonicalLocalPath;
        }
    }

    if (QFileInfo::exists(testWritePath))
        return canonicalLocalPath;

    QFile testWriteFile(testWritePath);
    if (testWriteFile.open(QIODeviceBase::ReadWrite)) {
        testWriteFile.close();
        return canonicalLocalPath;
    }

    qWarning() << "ERROR: Local download path appears not writeable:" << canonicalLocalPath;
    return canonicalLocalPath;
}

class MyPrivateSettings: public MySettings { };
Q_GLOBAL_STATIC(MyPrivateSettings, settingsInstance)
MySettings *MySettings::globalInstance()
{
    return settingsInstance();
}

MySettings::MySettings()
    : QObject{nullptr}
{
    QSettings::setDefaultFormat(QSettings::IniFormat);
}

void MySettings::restoreGenerationDefaults()
{
    setTemperature(default_temperature);
    setTopP(default_topP);
    setTopK(default_topK);
    setMaxLength(default_maxLength);
    setPromptBatchSize(default_promptBatchSize);
    setRepeatPenalty(default_repeatPenalty);
    setRepeatPenaltyTokens(default_repeatPenaltyTokens);
    setPromptTemplate(default_promptTemplate);
}

void MySettings::restoreApplicationDefaults()
{
    setThreadCount(default_threadCount);
    setSaveChats(default_saveChats);
    setSaveChatGPTChats(default_saveChatGPTChats);
    setServerChat(default_serverChat);
    setModelPath(defaultLocalModelsPath());
    setUserDefaultModel(default_userDefaultModel);
    setForceMetal(default_forceMetal);
}

double MySettings::temperature() const
{
    QSettings setting;
    setting.sync();
    return setting.value("temperature", default_temperature).toDouble();
}

void MySettings::setTemperature(double t)
{
    if (temperature() == t)
        return;

    QSettings setting;
    setting.setValue("temperature", t);
    setting.sync();
    emit temperatureChanged();
}

double MySettings::topP() const
{
    QSettings setting;
    setting.sync();
    return setting.value("topP", default_topP).toDouble();
}

void MySettings::setTopP(double p)
{
    if (topP() == p)
        return;

    QSettings setting;
    setting.setValue("topP", p);
    setting.sync();
    emit topPChanged();
}

int MySettings::topK() const
{
    QSettings setting;
    setting.sync();
    return setting.value("topK", default_topK).toInt();
}

void MySettings::setTopK(int k)
{
    if (topK() == k)
        return;

    QSettings setting;
    setting.setValue("topK", k);
    setting.sync();
    emit topKChanged();
}

int MySettings::maxLength() const
{
    QSettings setting;
    setting.sync();
    return setting.value("maxLength", default_maxLength).toInt();
}

void MySettings::setMaxLength(int l)
{
    if (maxLength() == l)
        return;

    QSettings setting;
    setting.setValue("maxLength", l);
    setting.sync();
    emit maxLengthChanged();
}

int MySettings::promptBatchSize() const
{
    QSettings setting;
    setting.sync();
    return setting.value("promptBatchSize", default_promptBatchSize).toInt();
}

void MySettings::setPromptBatchSize(int s)
{
    if (promptBatchSize() == s)
        return;

    QSettings setting;
    setting.setValue("promptBatchSize", s);
    setting.sync();
    emit promptBatchSizeChanged();
}

double MySettings::repeatPenalty() const
{
    QSettings setting;
    setting.sync();
    return setting.value("repeatPenalty", default_repeatPenalty).toDouble();
}

void MySettings::setRepeatPenalty(double p)
{
    if (repeatPenalty() == p)
        return;

    QSettings setting;
    setting.setValue("repeatPenalty", p);
    setting.sync();
    emit repeatPenaltyChanged();
}

int MySettings::repeatPenaltyTokens() const
{
    QSettings setting;
    setting.sync();
    return setting.value("repeatPenaltyTokens", default_repeatPenaltyTokens).toInt();
}

void MySettings::setRepeatPenaltyTokens(int t)
{
    if (repeatPenaltyTokens() == t)
        return;

    QSettings setting;
    setting.setValue("repeatPenaltyTokens", t);
    setting.sync();
    emit repeatPenaltyTokensChanged();
}

QString MySettings::promptTemplate() const
{
    QSettings setting;
    setting.sync();
    return setting.value("promptTemplate", default_promptTemplate).toString();
}

void MySettings::setPromptTemplate(const QString &t)
{
    if (promptTemplate() == t)
        return;

    QSettings setting;
    setting.setValue("promptTemplate", t);
    setting.sync();
    emit promptTemplateChanged();
}

int MySettings::threadCount() const
{
    QSettings setting;
    setting.sync();
    return setting.value("threadCount", default_threadCount).toInt();
}

void MySettings::setThreadCount(int c)
{
    if (threadCount() == c)
        return;

    QSettings setting;
    setting.setValue("threadCount", c);
    setting.sync();
    emit threadCountChanged();
}

bool MySettings::saveChats() const
{
    QSettings setting;
    setting.sync();
    return setting.value("saveChats", default_saveChats).toBool();
}

void MySettings::setSaveChats(bool b)
{
    if (saveChats() == b)
        return;

    QSettings setting;
    setting.setValue("saveChats", b);
    setting.sync();
    emit saveChatsChanged();
}

bool MySettings::saveChatGPTChats() const
{
    QSettings setting;
    setting.sync();
    return setting.value("saveChatGPTChats", default_saveChatGPTChats).toBool();
}

void MySettings::setSaveChatGPTChats(bool b)
{
    if (saveChatGPTChats() == b)
        return;

    QSettings setting;
    setting.setValue("saveChatGPTChats", b);
    setting.sync();
    emit saveChatGPTChatsChanged();
}

bool MySettings::serverChat() const
{
    QSettings setting;
    setting.sync();
    return setting.value("serverChat", default_serverChat).toBool();
}

void MySettings::setServerChat(bool b)
{
    if (serverChat() == b)
        return;

    QSettings setting;
    setting.setValue("serverChat", b);
    setting.sync();
    emit serverChatChanged();
}

QString MySettings::modelPath() const
{
    QSettings setting;
    setting.sync();
    return setting.value("modelPath", defaultLocalModelsPath()).toString();
}

void MySettings::setModelPath(const QString &p)
{
    if (modelPath() == p)
        return;

    QSettings setting;
    setting.setValue("modelPath", p);
    setting.sync();
    emit modelPathChanged();
}

QString MySettings::userDefaultModel() const
{
    QSettings setting;
    setting.sync();
    return setting.value("userDefaultModel", default_userDefaultModel).toString();
}

void MySettings::setUserDefaultModel(const QString &u)
{
    if (userDefaultModel() == u)
        return;

    QSettings setting;
    setting.setValue("userDefaultModel", u);
    setting.sync();
    emit userDefaultModelChanged();
}

bool MySettings::forceMetal() const
{
    return m_forceMetal;
}

void MySettings::setForceMetal(bool b)
{
    if (m_forceMetal == b)
        return;
    m_forceMetal = b;
    emit forceMetalChanged(b);
}
