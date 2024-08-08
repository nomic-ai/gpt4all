#ifndef TOOL_H
#define TOOL_H

#include <QObject>
#include <QJsonObject>

using namespace Qt::Literals::StringLiterals;

namespace ToolEnums {
    Q_NAMESPACE
    enum class Error {
        NoError = 0,
        TimeoutError = 2,
        UnknownError = 499,
    };
    Q_ENUM_NS(Error)
}

class Tool : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString description MEMBER description)
    Q_PROPERTY(QString function MEMBER function)
    Q_PROPERTY(QJsonObject paramSchema MEMBER paramSchema)
    Q_PROPERTY(QUrl url MEMBER url)
    Q_PROPERTY(bool isEnabled MEMBER isEnabled)
    Q_PROPERTY(bool isBuiltin MEMBER isBuiltin)
    Q_PROPERTY(bool forceUsage MEMBER forceUsage)
    Q_PROPERTY(bool excerpts MEMBER excerpts)

public:
    Tool() : QObject(nullptr) {}
    virtual ~Tool() {}

    virtual QString run(const QJsonObject &parameters, qint64 timeout = 2000) = 0;
    virtual ToolEnums::Error error() const { return ToolEnums::Error::NoError; }
    virtual QString errorString() const { return QString(); }

    QString name;               // [Required] Human readable name of the tool.
    QString description;        // [Required] Human readable description of the tool.
    QString function;           // [Required] Must be unique. Name of the function to invoke. Must be a-z, A-Z, 0-9, or contain underscores and dashes, with a maximum length of 64.
    QJsonObject paramSchema;    // [Optional] Json schema describing the tool's parameters. An empty object specifies no parameters.
                                // https://json-schema.org/understanding-json-schema/
    QUrl url;                   // [Optional] The local file or remote resource use to invoke the tool.
    bool isEnabled = false;     // [Optional] Whether the tool is currently enabled
    bool isBuiltin = false;     // [Optional] Whether the tool is built-in
    bool forceUsage = false;    // [Optional] Whether we should attempt to force usage of the tool rather than let the LLM decide. NOTE: Not always possible.
    bool excerpts = false;      // [Optional] Whether json result produces source excerpts.

    // FIXME: Should we go with essentially the OpenAI/ollama consensus for these tool
    // info files? If you install a tool in GPT4All should it need to meet the spec for these:
    // https://platform.openai.com/docs/api-reference/runs/createRun#runs-createrun-tools
    // https://github.com/ollama/ollama/blob/main/docs/api.md#chat-request-with-tools

    bool operator==(const Tool &other) const {
        return function == other.function;
    }
    bool operator!=(const Tool &other) const {
        return !(*this == other);
    }
};

#endif // TOOL_H
