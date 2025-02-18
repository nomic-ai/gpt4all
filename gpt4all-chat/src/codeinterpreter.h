#ifndef CODEINTERPRETER_H
#define CODEINTERPRETER_H

#include "tool.h"
#include "toolcallparser.h"

#include <QObject>
#include <QString>
#include <QThread>
#include <QtAssert>

class QJSEngine;


class JavaScriptConsoleCapture : public QObject
{
    Q_OBJECT
public:
    QString output;
    Q_INVOKABLE void log(const QString &message)
    {
        const int maxLength = 1024;
        if (output.length() >= maxLength)
            return;

        if (output.length() + message.length() + 1 > maxLength) {
            static const QString trunc = "\noutput truncated at " + QString::number(maxLength) + " characters...";
            int remainingLength = maxLength - output.length();
            if (remainingLength > 0)
                output.append(message.left(remainingLength));
            output.append(trunc);
            Q_ASSERT(output.length() > maxLength);
        } else {
            output.append(message + "\n");
        }
    }
};

class CodeInterpreterWorker : public QObject {
    Q_OBJECT
public:
    CodeInterpreterWorker();
    virtual ~CodeInterpreterWorker() {}

    void reset();
    QString response() const { return m_response; }
    ToolEnums::Error error() const { return m_error; }
    QString errorString() const { return m_errorString; }
    bool interrupt();

public Q_SLOTS:
    void request(const QString &code);

Q_SIGNALS:
    void finished();

private:
    QString m_response;
    ToolEnums::Error m_error = ToolEnums::Error::NoError;
    QString m_errorString;
    QThread m_thread;
    JavaScriptConsoleCapture m_consoleCapture;
    QJSEngine *m_engine = nullptr;
};

class CodeInterpreter : public Tool
{
    Q_OBJECT
public:
    explicit CodeInterpreter();
    virtual ~CodeInterpreter() {}

    void run(const QList<ToolParam> &params) override;
    bool interrupt() override;

    ToolEnums::Error error() const override { return m_error; }
    QString errorString() const override { return m_errorString; }

    QString name() const override { return tr("Code Interpreter"); }
    QString description() const override { return tr("compute javascript code using console.log as output"); }
    QString function() const override { return ToolCallConstants::CodeInterpreterFunction; }
    QList<ToolParamInfo> parameters() const override;
    virtual QString symbolicFormat() const override;
    QString examplePrompt() const override;
    QString exampleCall() const override;
    QString exampleReply() const override;

Q_SIGNALS:
    void request(const QString &code);

private:
    ToolEnums::Error m_error = ToolEnums::Error::NoError;
    QString m_errorString;
    CodeInterpreterWorker *m_worker;
};

#endif // CODEINTERPRETER_H
