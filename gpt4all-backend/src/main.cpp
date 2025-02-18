module;

#include <string>

#include <QString>

#include <ollama.hpp>

module gpt4all.backend.main;


std::string LLMProvider::qstringToSTL(const QString &s)
{
    return s.toStdString();
}
