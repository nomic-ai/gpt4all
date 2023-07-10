#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <QDirIterator>

#include "llm.h"
#include "modellist.h"
#include "chatlistmodel.h"
#include "localdocs.h"
#include "download.h"
#include "network.h"
#include "mysettings.h"
#include "config.h"
#include "logger.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("nomic.ai");
    QCoreApplication::setOrganizationDomain("gpt4all.io");
    QCoreApplication::setApplicationName("GPT4All");
    QCoreApplication::setApplicationVersion(APP_VERSION);

    Logger::globalInstance();

    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    qmlRegisterSingletonInstance("mysettings", 1, 0, "MySettings", MySettings::globalInstance());
    qmlRegisterSingletonInstance("modellist", 1, 0, "ModelList", ModelList::globalInstance());
    qmlRegisterSingletonInstance("chatlistmodel", 1, 0, "ChatListModel", ChatListModel::globalInstance());
    qmlRegisterSingletonInstance("llm", 1, 0, "LLM", LLM::globalInstance());
    qmlRegisterSingletonInstance("download", 1, 0, "Download", Download::globalInstance());
    qmlRegisterSingletonInstance("network", 1, 0, "Network", Network::globalInstance());
    qmlRegisterSingletonInstance("localdocs", 1, 0, "LocalDocs", LocalDocs::globalInstance());
    const QUrl url(u"qrc:/gpt4all/main.qml"_qs);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        }, Qt::QueuedConnection);
    engine.load(url);

#if 0
    QDirIterator it("qrc:", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        qDebug() << it.next();
    }
#endif

    return app.exec();
}
