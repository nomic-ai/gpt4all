#include "logger.h"

#include <QFile>
#include <QDebug>
#include <QStandardPaths>
#include <QDateTime>
#include <iostream>

class MyLogger: public Logger { };
Q_GLOBAL_STATIC(MyLogger, loggerInstance)
Logger *Logger::globalInstance()
{
    return loggerInstance();
}

Logger::Logger()
{
    // Get log file dir
    auto dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    // Remove old log file
    QFile::remove(dir+"/log-prev.txt");
    QFile::rename(dir+"/log.txt", dir+"/log-prev.txt");
    // Open new log file
    m_file.setFileName(dir+"/log.txt");
    if (!m_file.open(QIODevice::NewOnly | QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open log file, logging to stdout...";
        m_file.open(stdout, QIODevice::WriteOnly | QIODevice::Text);
    }
    // On success, install message handler
    qInstallMessageHandler(Logger::messageHandler);
}

void Logger::messageHandler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    auto logger = globalInstance();
    // Get message type as string
    QString typeString;
    switch (type) {
    case QtDebugMsg:
        typeString = "Debug";
        break;
    case QtInfoMsg:
        typeString = "Info";
        break;
    case QtWarningMsg:
        typeString = "Warning";
        break;
    case QtCriticalMsg:
        typeString = "Critical";
        break;
    case QtFatalMsg:
        typeString = "Fatal";
        break;
    default:
        typeString = "???";
    }
    // Get time and date
    auto timestamp = QDateTime::currentDateTime().toString();
    // Write message
    const std::string out = QString("[%1] (%2): %4\n").arg(typeString, timestamp, msg).toStdString();
    logger->m_file.write(out.c_str());
    logger->m_file.flush();
    std::cerr << out;
    fflush(stderr);
}
