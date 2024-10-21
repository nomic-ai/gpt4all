#include "utils.h"


constexpr uint32_t hash(const char* data) noexcept
{
    uint32_t hash = 5381;

    for(const char *c = data; *c; ++c)
        hash = ((hash << 5) + hash) + (unsigned char) *c;

    return hash;
}