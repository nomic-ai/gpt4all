#pragma once

#include <cassert>

#ifdef NDEBUG
#   ifdef __has_builtin
#       if __has_builtin(__builtin_unreachable)
#           define UNREACHABLE() __builtin_unreachable()
#       else
#           define UNREACHABLE() do {} while (0)
#       endif
#   else
#       define UNREACHABLE() do {} while (0)
#   endif
#else
#   define UNREACHABLE() assert(!"Unreachable statement was reached")
#endif
