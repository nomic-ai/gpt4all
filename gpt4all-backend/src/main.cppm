module;

#include <string>

#include <QString>

export module gpt4all.backend.main;


export class LLMProvider {
    static std::string qstringToSTL(const QString &s);
};
