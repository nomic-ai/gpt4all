#ifndef TOOL_H
#define TOOL_H

#include <QObject>
#include <QJsonObject>
#include <jinja2cpp/value.h>

using namespace Qt::Literals::StringLiterals;

namespace ToolEnums {
    Q_NAMESPACE
    enum class Error {
        NoError = 0,
        TimeoutError = 2,
        UnknownError = 499,
    };
    Q_ENUM_NS(Error)

    enum class UsageMode {
        Disabled = 0,           // Completely disabled
        Enabled = 1,            // Enabled and the model decides whether to run
        ForceUsage = 2,         // Attempt to force usage of the tool rather than let the LLM decide. NOTE: Not always possible.
    };
    Q_ENUM_NS(UsageMode)

    enum class ConfirmationMode {
        NoConfirmation = 0,             // No confirmation required
        AskBeforeRunning = 1,           // User is queried on every execution
        AskBeforeRunningRecursive = 2,  // User is queried if the tool is invoked in a recursive tool call
    };
    Q_ENUM_NS(ConfirmationMode)

    // Ordered in increasing levels of privacy
    enum class PrivacyScope {
        None = 0,               // Tool call data does not have any privacy scope
        LocalOrg = 1,           // Tool call data does not leave the local organization
        Local = 2               // Tool call data does not leave the machine
    };
    Q_ENUM_NS(PrivacyScope)
}

class Tool : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString description READ description CONSTANT)
    Q_PROPERTY(QString function READ function CONSTANT)
    Q_PROPERTY(ToolEnums::PrivacyScope privacyScope READ privacyScope CONSTANT)
    Q_PROPERTY(QJsonObject paramSchema READ paramSchema CONSTANT)
    Q_PROPERTY(QJsonObject exampleParams READ exampleParams CONSTANT)
    Q_PROPERTY(QUrl url READ url CONSTANT)
    Q_PROPERTY(bool isBuiltin READ isBuiltin CONSTANT)
    Q_PROPERTY(ToolEnums::UsageMode usageMode READ usageMode NOTIFY usageModeChanged)
    Q_PROPERTY(ToolEnums::ConfirmationMode confirmationMode READ confirmationMode NOTIFY confirmationModeChanged)
    Q_PROPERTY(bool excerpts READ excerpts CONSTANT)

public:
    Tool() : QObject(nullptr) {}
    virtual ~Tool() {}

    virtual QString run(const QJsonObject &parameters, qint64 timeout = 2000) = 0;
    virtual ToolEnums::Error error() const { return ToolEnums::Error::NoError; }
    virtual QString errorString() const { return QString(); }

    // [Required] Human readable name of the tool.
    virtual QString name() const = 0;

    // [Required] Human readable description of the tool.
    virtual QString description() const = 0;

    // [Required] Must be unique. Name of the function to invoke. Must be a-z, A-Z, 0-9, or contain underscores and dashes, with a maximum length of 64.
    virtual QString function() const = 0;

    // [Required] The privacy scope.
    virtual ToolEnums::PrivacyScope privacyScope() const = 0;

    // [Optional] Json schema describing the tool's parameters. An empty object specifies no parameters.
    // https://json-schema.org/understanding-json-schema/
    // https://platform.openai.com/docs/api-reference/runs/createRun#runs-createrun-tools
    // https://github.com/ollama/ollama/blob/main/docs/api.md#chat-request-with-tools
    // FIXME: This should be validated against json schema
    virtual QJsonObject paramSchema() const { return QJsonObject(); }

    // [Optional] An example of the parameters for this tool call. NOTE: This should only include parameters
    // that the model is responsible for generating.
    virtual QJsonObject exampleParams() const { return QJsonObject(); }

    // [Optional] The local file or remote resource use to invoke the tool.
    virtual QUrl url() const { return QUrl(); }

    // [Optional] Whether the tool is built-in.
    virtual bool isBuiltin() const { return false; }

    // [Optional] The usage mode.
    virtual ToolEnums::UsageMode usageMode() const { return ToolEnums::UsageMode::Disabled; }

    // [Optional] The confirmation mode.
    virtual ToolEnums::ConfirmationMode confirmationMode() const { return ToolEnums::ConfirmationMode::NoConfirmation; }

    // [Optional] Whether json result produces source excerpts.
    virtual bool excerpts() const { return false; }

    bool operator==(const Tool &other) const {
        return function() == other.function();
    }
    bool operator!=(const Tool &other) const {
        return !(*this == other);
    }

    jinja2::Value jinjaValue() const;

Q_SIGNALS:
    void usageModeChanged();
    void confirmationModeChanged();
};

#endif // TOOL_H
