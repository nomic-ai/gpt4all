#ifndef CODEINTERPRETER_H
#define CODEINTERPRETER_H

#include "tool.h"
#include "toolcallparser.h"

#include <QJSEngine>
#include <QObject>
#include <QString>
#include <QtGlobal>

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

    QString response() const { return m_response; }

    void request(const QString &code);
    void interrupt(qint64 timeout) { m_timeout = timeout; m_engine.setInterrupted(true); }
    ToolEnums::Error error() const { return m_error; }
    QString errorString() const { return m_errorString; }

Q_SIGNALS:
    void finished();

private:
    qint64 m_timeout = 0;
    QJSEngine m_engine;
    QString m_response;
    ToolEnums::Error m_error = ToolEnums::Error::NoError;
    QString m_errorString;
};

class CodeInterpreter : public Tool
{
    Q_OBJECT
public:
    explicit CodeInterpreter() : Tool(), m_error(ToolEnums::Error::NoError) {}
    virtual ~CodeInterpreter() {}

    QString run(const QList<ToolParam> &params, qint64 timeout = 2000) override;
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

private:
    ToolEnums::Error m_error = ToolEnums::Error::NoError;
    QString m_errorString;
};

#endif // CODEINTERPRETER_H
