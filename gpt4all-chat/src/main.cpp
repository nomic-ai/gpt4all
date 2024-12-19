#include "chatlistmodel.h"
#include "config.h"
#include "download.h"
#include "llm.h"
#include "localdocs.h"
#include "logger.h"
#include "modellist.h"
#include "mysettings.h"
#include "network.h"
#include "toolmodel.h"

#include <gpt4all-backend/llmodel.h>
#include <singleapplication.h>

#include <QCoreApplication>
#include <QFont>
#include <QFontDatabase>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSettings>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <Qt>

#ifdef Q_OS_LINUX
#   include <QIcon>
#endif

#ifdef Q_OS_WINDOWS
#   include <windows.h>
#else
#   include <signal.h>
#endif

using namespace Qt::Literals::StringLiterals;


static void raiseWindow(QWindow *window)
{
#ifdef Q_OS_WINDOWS
    HWND hwnd = HWND(window->winId());

    // check if window is minimized to Windows task bar
    if (IsIconic(hwnd))
        ShowWindow(hwnd, SW_RESTORE);

    SetForegroundWindow(hwnd);
#else
    LLM::globalInstance()->showDockIcon();
    window->show();
    window->raise();
    window->requestActivate();
#endif
}

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("nomic.ai");
    QCoreApplication::setOrganizationDomain("gpt4all.io");
    QCoreApplication::setApplicationName("GPT4All");
    QCoreApplication::setApplicationVersion(APP_VERSION);
    QSettings::setDefaultFormat(QSettings::IniFormat);

    Logger::globalInstance();

    SingleApplication app(argc, argv, true /*allowSecondary*/);
    if (app.isSecondary()) {
#ifdef Q_OS_WINDOWS
        AllowSetForegroundWindow(DWORD(app.primaryPid()));
#endif
        app.sendMessage("RAISE_WINDOW");
        return 0;
    }

#ifdef Q_OS_LINUX
    app.setWindowIcon(QIcon(":/gpt4all/icons/gpt4all.svg"));
#endif

    // set search path before constructing the MySettings instance, which relies on this
    QString llmodelSearchPaths = QCoreApplication::applicationDirPath();
    const QString libDir = QCoreApplication::applicationDirPath() + "/../lib/";
    if (LLM::directoryExists(libDir))
        llmodelSearchPaths += ";" + libDir;
#if defined(Q_OS_MAC)
    const QString binDir = QCoreApplication::applicationDirPath() + "/../../../";
    if (LLM::directoryExists(binDir))
        llmodelSearchPaths += ";" + binDir;
    const QString frameworksDir = QCoreApplication::applicationDirPath() + "/../Frameworks/";
    if (LLM::directoryExists(frameworksDir))
        llmodelSearchPaths += ";" + frameworksDir;
#endif
    LLModel::Implementation::setImplementationsSearchPath(llmodelSearchPaths.toStdString());

    // Set the local and language translation before the qml engine has even been started. This will
    // use the default system locale unless the user has explicitly set it to use a different one.
    auto *mySettings = MySettings::globalInstance();
    mySettings->setLanguageAndLocale();

    QQmlApplicationEngine engine;

    // Add a connection here from MySettings::languageAndLocaleChanged signal to a lambda slot where I can call
    // engine.uiLanguage property
    QObject::connect(mySettings, &MySettings::languageAndLocaleChanged, [&engine]() {
        engine.setUiLanguage(MySettings::globalInstance()->languageAndLocale());
    });

    auto *modelList = ModelList::globalInstance();
    QObject::connect(modelList, &ModelList::dataChanged, mySettings, &MySettings::onModelInfoChanged);

    qmlRegisterSingletonInstance("mysettings", 1, 0, "MySettings", mySettings);
    qmlRegisterSingletonInstance("modellist", 1, 0, "ModelList", modelList);
    qmlRegisterSingletonInstance("chatlistmodel", 1, 0, "ChatListModel", ChatListModel::globalInstance());
    qmlRegisterSingletonInstance("llm", 1, 0, "LLM", LLM::globalInstance());
    qmlRegisterSingletonInstance("download", 1, 0, "Download", Download::globalInstance());
    qmlRegisterSingletonInstance("network", 1, 0, "Network", Network::globalInstance());
    qmlRegisterSingletonInstance("localdocs", 1, 0, "LocalDocs", LocalDocs::globalInstance());
    qmlRegisterSingletonInstance("toollist", 1, 0, "ToolList", ToolModel::globalInstance());
    qmlRegisterUncreatableMetaObject(ToolEnums::staticMetaObject, "toolenums", 1, 0, "ToolEnums", "Error: only enums");
    qmlRegisterUncreatableMetaObject(MySettingsEnums::staticMetaObject, "mysettingsenums", 1, 0, "MySettingsEnums", "Error: only enums");

    {
        auto fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        engine.rootContext()->setContextProperty("fixedFont", fixedFont);
    }

    const QUrl url(u"qrc:/gpt4all/main.qml"_s);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        }, Qt::QueuedConnection);
    engine.load(url);

    QObject *rootObject = engine.rootObjects().first();
    QQuickWindow *windowObject = qobject_cast<QQuickWindow *>(rootObject);
    Q_ASSERT(windowObject);
    if (windowObject)
        QObject::connect(&app, &SingleApplication::receivedMessage,
                         windowObject, [windowObject] () { raiseWindow(windowObject); } );

#if 0
    QDirIterator it("qrc:", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        qDebug() << it.next();
    }
#endif

#ifndef Q_OS_WINDOWS
    // handle signals gracefully
    struct sigaction sa;
    sa.sa_handler = [](int s) { QCoreApplication::exit(s == SIGINT ? 0 : 1); };
    sa.sa_flags   = SA_RESETHAND;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT,  &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGHUP,  &sa, nullptr);
#endif

    int res = app.exec();

    // Make sure ChatLLM threads are joined before global destructors run.
    // Otherwise, we can get a heap-use-after-free inside of llama.cpp.
    ChatListModel::globalInstance()->destroyChats();

    return res;
}
