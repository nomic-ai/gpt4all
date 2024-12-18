#include "tool.h"

#include <jinja2cpp/value.h>

#include <string>

jinja2::Value Tool::jinjaValue() const
{
    jinja2::ValuesList paramList;
    const QList<ToolParamInfo> p = parameters();
    for (auto &info : p) {
        std::string typeStr;
        switch (info.type) {
        using enum ToolEnums::ParamType;
        case String:   typeStr = "string"; break;
        case Number:   typeStr = "number"; break;
        case Integer:  typeStr = "integer"; break;
        case Object:   typeStr = "object"; break;
        case Array:    typeStr = "array"; break;
        case Boolean:  typeStr = "boolean"; break;
        case Null:     typeStr = "null"; break;
        }
        jinja2::ValuesMap infoMap {
            { "name", info.name.toStdString() },
            { "type", typeStr},
            { "description", info.description.toStdString() },
            { "required", info.required }
        };
        paramList.push_back(infoMap);
    }

    jinja2::ValuesMap params {
        { "name", name().toStdString() },
        { "description", description().toStdString() },
        { "function", function().toStdString() },
        { "parameters", paramList },
        { "symbolicFormat", symbolicFormat().toStdString() },
        { "examplePrompt", examplePrompt().toStdString() },
        { "exampleCall", exampleCall().toStdString() },
        { "exampleReply", exampleReply().toStdString() }
    };
    return params;
}

void ToolCallInfo::serialize(QDataStream &stream, int version)
{
    stream << name;
    stream << params.size();
    for (auto param : params) {
        stream << param.name;
        stream << param.type;
        stream << param.value;
    }
    stream << result;
    stream << error;
    stream << errorString;
}

bool ToolCallInfo::deserialize(QDataStream &stream, int version)
{
    stream >> name;
    qsizetype count;
    stream >> count;
    for (int i = 0; i < count; ++i) {
        ToolParam p;
        stream >> p.name;
        stream >> p.type;
        stream >> p.value;
    }
    stream >> result;
    stream >> error;
    stream >> errorString;
    return true;
}
