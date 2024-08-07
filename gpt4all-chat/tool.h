#ifndef TOOL_H
#define TOOL_H

#include <QObject>
#include <QJsonObject>

using namespace Qt::Literals::StringLiterals;

namespace ToolEnums {
    Q_NAMESPACE
    enum class ConnectionType {
        BuiltinConnection          = 0, // A built-in tool with bespoke connection type
        LocalConnection            = 1, // Starts a local process and communicates via stdin/stdout/stderr
        LocalServerConnection      = 2, // Connects to an existing local process and communicates via stdin/stdout/stderr
        RemoteConnection           = 3, // Starts a remote process and communicates via some networking protocol TBD
        RemoteServerConnection     = 4  // Connects to an existing remote process and communicates via some networking protocol TBD
    };
    Q_ENUM_NS(ConnectionType)

    enum class Error {
        NoError = 0,
        TimeoutError = 2,
        UnknownError = 499,
    };
}

struct ToolInfo {
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString description MEMBER description)
    Q_PROPERTY(QJsonObject parameters MEMBER parameters)
    Q_PROPERTY(bool isEnabled MEMBER isEnabled)
    Q_PROPERTY(ToolEnums::ConnectionType connectionType MEMBER connectionType)

public:
    QString name;
    QString description;
    QJsonObject parameters;
    bool isEnabled;
    ToolEnums::ConnectionType connectionType;

    // FIXME: Should we go with essentially the OpenAI/ollama consensus for these tool
    // info files? If you install a tool in GPT4All should it need to meet the spec for these:
    // https://platform.openai.com/docs/api-reference/runs/createRun#runs-createrun-tools
    // https://github.com/ollama/ollama/blob/main/docs/api.md#chat-request-with-tools
    QJsonObject toJson() const
    {
        QJsonObject result;
        result.insert("name", name);
        result.insert("description", description);
        result.insert("parameters", parameters);
        return result;
    }

    static ToolInfo fromJson(const QString &json);

    bool operator==(const ToolInfo &other) const {
        return name == other.name;
    }
    bool operator!=(const ToolInfo &other) const {
        return !(*this == other);
    }
};
Q_DECLARE_METATYPE(ToolInfo)

class Tool : public QObject {
    Q_OBJECT
public:
    Tool() : QObject(nullptr) {}
    virtual ~Tool() {}

    virtual QString run(const QJsonObject &parameters, qint64 timeout = 2000) = 0;
    virtual ToolEnums::Error error() const { return ToolEnums::Error::NoError; }
    virtual QString errorString() const { return QString(); }
};

#endif // TOOL_H
