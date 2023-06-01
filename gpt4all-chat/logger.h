#ifndef LOGGER_H
#define LOGGER_H
#include <QObject>
#include <QFile>

class Logger : public QObject
{
    Q_OBJECT

    QFile file;

    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

public:
    static Logger *globalInstance();

    explicit Logger();
    friend class MyLogger;
};

#endif // LOGGER_H
