#ifndef LOGGER_H
#define LOGGER_H

#include <QFile>
#include <QString>
#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#   include <QtLogging>
#endif

class Logger
{
    QFile m_file;

    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

public:
    static Logger *globalInstance();

    explicit Logger();
    friend class MyLogger;
};

#endif // LOGGER_H
