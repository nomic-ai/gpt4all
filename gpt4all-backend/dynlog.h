// can be included in dlopen()ed libraries that do not directly link against dynlog.c 
// provided the linker is configured to allow the `dynlog_vlog` symbol to be resolved dynamically
#ifndef _DYNLOG_H
#define _DYNLOG_H
#include <stdarg.h>
#include <stdlib.h>

#define DYNLOG_DEBUG 4
#define DYNLOG_INFO 3
#define DYNLOG_WARNING 1
#define DYNLOG_ERROR 0

#ifdef __cplusplus
#define DYNLOG_IMPORT extern "C"
#else
#define DYNLOG_IMPORT extern
#endif

DYNLOG_IMPORT void dynlog_vlog(int level, const char* fmt, va_list args);

inline void dynlog_log(int level, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  dynlog_vlog(level, fmt, args);
  va_end(args);
}

#define dlog_debug(fmt, ...) dynlog_log(DYNLOG_DEBUG, "[DEBUG] %s:%d " fmt "\n", __FILE__, __LINE__, __VA_ARGS__)
#define dlog_info(fmt, ...) dynlog_log(DYNLOG_INFO, "[INFO] %s:%d " fmt "\n", __FILE__, __LINE__, __VA_ARGS__)
#define dlog_warn(fmt, ...) dynlog_log(DYNLOG_WARNING, "[WARNING] %s:%d " fmt "\n", __FILE__, __LINE__, __VA_ARGS__)
#define dlog_error(fmt, ...) dynlog_log(DYNLOG_ERROR, "[ERROR] %s:%d " fmt "\n", __FILE__, __LINE__, __VA_ARGS__)

#endif
