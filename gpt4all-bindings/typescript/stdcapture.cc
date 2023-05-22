
#include "stdcapture.h"

CoutRedirect::CoutRedirect() {
    old = std::cout.rdbuf(buffer.rdbuf());  // redirect cout to buffer stream
}

std::string CoutRedirect::getString() {
    return buffer.str();  // get string
}

CoutRedirect::~CoutRedirect() {
    std::cout.rdbuf(old);  // reverse redirect
}
