#include "codeinterpreter.h"

#include <QJSValue>
#include <QStringList>
#include <QThread>
#include <QVariant>

using namespace Qt::Literals::StringLiterals;


QString CodeInterpreter::run(const QList<ToolParam> &params, qint64 timeout)
{
    m_error = ToolEnums::Error::NoError;
    m_errorString = QString();

    Q_ASSERT(params.count() == 1
          && params.first().name == "code"
          && params.first().type == ToolEnums::ParamType::String);

    const QString code = params.first().value.toString();

    QThread workerThread;
    CodeInterpreterWorker worker;
    worker.moveToThread(&workerThread);
    connect(&worker, &CodeInterpreterWorker::finished, &workerThread, &QThread::quit, Qt::DirectConnection);
    connect(&workerThread, &QThread::started, [&worker, code]() {
        worker.request(code);
    });
    workerThread.start();
    bool timedOut = !workerThread.wait(timeout);
    if (timedOut) {
        worker.interrupt(timeout); // thread safe
        m_error = ToolEnums::Error::TimeoutError;
    }
    workerThread.quit();
    workerThread.wait();
    if (!timedOut) {
        m_error = worker.error();
        m_errorString = worker.errorString();
    }
    return worker.response();
}

QList<ToolParamInfo> CodeInterpreter::parameters() const
{
    return {{
        "code",
        ToolEnums::ParamType::String,
        "javascript code to compute",
        true
    }};
}

QString CodeInterpreter::symbolicFormat() const
{
    return "{human readable plan to complete the task}\n" + ToolCallConstants::CodeInterpreterPrefix + "{code}\n" + ToolCallConstants::CodeInterpreterSuffix;
}

QString CodeInterpreter::examplePrompt() const
{
    return R"(Write code to check if a number is prime, use that to see if the number 7 is prime)";
}

QString CodeInterpreter::exampleCall() const
{
    static const QString example = R"(function isPrime(n) {
    if (n <= 1) {
        return false;
    }
    for (let i = 2; i <= Math.sqrt(n); i++) {
        if (n % i === 0) {
            return false;
        }
    }
    return true;
}

const number = 7;
console.log(`The number ${number} is prime: ${isPrime(number)}`);
)";

    return "Certainly! Let's compute the answer to whether the number 7 is prime.\n" + ToolCallConstants::CodeInterpreterPrefix + example + ToolCallConstants::CodeInterpreterSuffix;
}

QString CodeInterpreter::exampleReply() const
{
    return R"("The computed result shows that 7 is a prime number.)";
}

CodeInterpreterWorker::CodeInterpreterWorker()
    : QObject(nullptr)
{
}

void CodeInterpreterWorker::request(const QString &code)
{
    JavaScriptConsoleCapture consoleCapture;
    QJSValue consoleInternalObject = m_engine.newQObject(&consoleCapture);
    m_engine.globalObject().setProperty("console_internal", consoleInternalObject);

    // preprocess console.log args in JS since Q_INVOKE doesn't support varargs
    auto consoleObject = m_engine.evaluate(uR"(
        class Console {
            log(...args) {
                if (args.length && typeof args[0] === 'string')
                    throw new Error('console.log string formatting not supported');
                let cat = '';
                for (const arg of args) {
                    cat += String(arg);
                }
                console_internal.log(cat);
            }
        }

        new Console();
    )"_s);
    m_engine.globalObject().setProperty("console", consoleObject);

    const QJSValue result = m_engine.evaluate(code);

    QString resultString;

    if (m_engine.isInterrupted()) {
        resultString = QString("Error: code execution was timed out as it exceeded %1 ms. Code must be written to ensure execution does not timeout.").arg(m_timeout);
    } else if (result.isError()) {
        // NOTE: We purposely do not set the m_error or m_errorString for the code interpreter since
        // we *want* the model to see the response has an error so it can hopefully correct itself. The
        // error member variables are intended for tools that have error conditions that cannot be corrected.
        // For instance, a tool depending upon the network might set these error variables if the network
        // is not available.
        const QStringList lines = code.split('\n');
        const int line = result.property("lineNumber").toInt();
        const int index = line - 1;
        const QString lineContent = (index >= 0 && index < lines.size()) ? lines.at(index) : "Line not found in code.";
            resultString = QString("Uncaught exception at line %1: %2\n\t%3")
                .arg(line)
                .arg(result.toString())
                .arg(lineContent);
        m_error = ToolEnums::Error::UnknownError;
        m_errorString = resultString;
    } else {
        resultString = result.isUndefined() ? QString() : result.toString();
    }

    if (resultString.isEmpty())
        resultString = consoleCapture.output;
    else if (!consoleCapture.output.isEmpty())
        resultString += "\n" + consoleCapture.output;
    m_response = resultString;
    emit finished();
}
