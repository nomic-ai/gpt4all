module;

#include <string>

#include <QString>

#include <ollama.hpp>

module gpt4all.backend.main;

import fmt;


std::string LLMProvider::qstringToSTL(const QString &s)
{
    fmt::format("{}", "foo");
    return s.toStdString();
}
