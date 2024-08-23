#include "llmodel.h"

#include <algorithm>
#include <cctype>
#include <string>


std::string remove_leading_whitespace(const std::string &input)
{
    auto first_non_whitespace = std::find_if(input.begin(), input.end(), [](unsigned char c) {
        return !std::isspace(c);
    });

    if (first_non_whitespace == input.end())
        return std::string();

    return std::string(first_non_whitespace, input.end());
}

std::string trim_whitespace(const std::string &input)
{
    auto first_non_whitespace = std::find_if(input.begin(), input.end(), [](unsigned char c) {
        return !std::isspace(c);
    });

    if (first_non_whitespace == input.end())
        return std::string();

    auto last_non_whitespace = std::find_if(input.rbegin(), input.rend(), [](unsigned char c) {
        return !std::isspace(c);
    }).base();

    return std::string(first_non_whitespace, last_non_whitespace);
}
