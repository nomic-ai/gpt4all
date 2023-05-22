//https://stackoverflow.com/questions/5419356/redirect-stdout-stderr-to-a-string
#ifndef COUTREDIRECT_H
#define COUTREDIRECT_H

#include <iostream>
#include <streambuf>
#include <string>
#include <sstream>

class CoutRedirect {
public:
    CoutRedirect();
    std::string getString();
    ~CoutRedirect();

private:
    std::stringstream buffer;
    std::streambuf* old;
};

#endif  // COUTREDIRECT_H
