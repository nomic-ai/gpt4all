#ifndef LOGGER_H
#define LOGGER_H
#include <QFile>

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
