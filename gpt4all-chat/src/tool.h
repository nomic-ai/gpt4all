#ifndef TOOL_H
#define TOOL_H

#include <QList>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QtGlobal>

#include <jinja2cpp/value.h>

namespace ToolEnums
{
    Q_NAMESPACE
    enum class Error
    {
        NoError = 0,
        TimeoutError = 2,
        UnknownError = 499,
    };
    Q_ENUM_NS(Error)

    enum class ParamType { String, Number, Integer, Object, Array, Boolean, Null }; // json schema types
    Q_ENUM_NS(ParamType)

    enum class ParseState {
        None,
        InStart,
        Partial,
        Complete,
    };
    Q_ENUM_NS(ParseState)
}

struct ToolParamInfo
{
    QString name;
    ToolEnums::ParamType type;
    QString description;
    bool required;
};
Q_DECLARE_METATYPE(ToolParamInfo)

struct ToolParam
{
    QString name;
    ToolEnums::ParamType type;
    QVariant value;
    bool operator==(const ToolParam& other) const
    {
        return name == other.name && type == other.type && value == other.value;
    }
};
Q_DECLARE_METATYPE(ToolParam)

struct ToolCallInfo
{
    QString name;
    QList<ToolParam> params;
    QString result;
    ToolEnums::Error error = ToolEnums::Error::NoError;
    QString errorString;

    void serialize(QDataStream &stream, int version);
    bool deserialize(QDataStream &stream, int version);

    bool operator==(const ToolCallInfo& other) const
    {
        return name == other.name && result == other.result && params == other.params
            && error == other.error && errorString == other.errorString;
    }
};
Q_DECLARE_METATYPE(ToolCallInfo)

class Tool : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString description READ description CONSTANT)
    Q_PROPERTY(QString function READ function CONSTANT)
    Q_PROPERTY(QList<ToolParamInfo> parameters READ parameters CONSTANT)
    Q_PROPERTY(QString examplePrompt READ examplePrompt CONSTANT)
    Q_PROPERTY(QString exampleCall READ exampleCall CONSTANT)
    Q_PROPERTY(QString exampleReply READ exampleReply CONSTANT)

public:
    Tool() : QObject(nullptr) {}
    virtual ~Tool() {}

    virtual QString run(const QList<ToolParam> &params, qint64 timeout = 2000) = 0;

    // Tools should set these if they encounter errors. For instance, a tool depending upon the network
    // might set these error variables if the network is not available.
    virtual ToolEnums::Error error() const { return ToolEnums::Error::NoError; }
    virtual QString errorString() const { return QString(); }

    // [Required] Human readable name of the tool.
    virtual QString name() const = 0;

    // [Required] Human readable description of what the tool does. Use this tool to: {{description}}
    virtual QString description() const = 0;

    // [Required] Must be unique. Name of the function to invoke. Must be a-z, A-Z, 0-9, or contain underscores and dashes, with a maximum length of 64.
    virtual QString function() const = 0;

    // [Optional] List describing the tool's parameters. An empty list specifies no parameters.
    virtual QList<ToolParamInfo> parameters() const { return {}; }

    // [Optional] The symbolic format of the toolcall.
    virtual QString symbolicFormat() const { return QString(); }

    // [Optional] A human generated example of a prompt that could result in this tool being called.
    virtual QString examplePrompt() const { return QString(); }

    // [Optional] An example of this tool call that pairs with the example query. It should be the
    // complete string that the model must generate.
    virtual QString exampleCall() const { return QString(); }

    // [Optional] An example of the reply the model might generate given the result of the tool call.
    virtual QString exampleReply() const { return QString(); }

    bool operator==(const Tool &other) const { return function() == other.function(); }

    jinja2::Value jinjaValue() const;
};

#endif // TOOL_H
