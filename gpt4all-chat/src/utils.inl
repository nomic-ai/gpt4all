#include <QJsonObject>

inline QJsonObject makeJsonObject(std::initializer_list<std::pair<QLatin1StringView, QJsonValue>> args)
{
    QJsonObject obj;
    for (auto &arg : args)
        obj.insert(arg.first, arg.second);
    return obj;
}
