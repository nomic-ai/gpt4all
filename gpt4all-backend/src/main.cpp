module;

#include <string>

#include <QString>

module gpt4all.backend.main;


std::string LLMProvider::qstringToSTL(const QString &s)
{
    return s.toStdString();
}
