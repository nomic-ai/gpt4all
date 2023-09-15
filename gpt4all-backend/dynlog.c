#include "dynlog.h"
#include <stdio.h>
#include <strings.h>

#ifndef DYNLOG_DEFAULT_OUTPUT
#define DYNLOG_DEFAULT_OUTPUT stderr
#endif

static int dynlog_level = DYNLOG_DEBUG;
static FILE* dynlog_outfile = NULL;

__attribute__((constructor)) static void dynlog_init(void) {
  char* logfilename = getenv("GPT4ALL_LOG_FILE");
  if (logfilename == NULL) {
    dynlog_outfile = DYNLOG_DEFAULT_OUTPUT;
  } else if (logfilename[0] == 0) {
    dynlog_outfile = NULL;
  } else {
    dynlog_outfile = fopen(logfilename, "a");
  }
  char *loglevel = getenv("GPT4ALL_LOG_LEVEL");
  if (loglevel == NULL) {
    return;
  } else if (strcasecmp("debug", loglevel) == 0) {
    dynlog_level = DYNLOG_DEBUG;
  } else if (strcasecmp("info", loglevel) == 0) {
    dynlog_level = DYNLOG_INFO;
  } else if (strcasecmp("error", loglevel) == 0) {
    dynlog_level = DYNLOG_ERROR;
  } else if (strncasecmp("warn", loglevel, 4) == 0) {
    dynlog_level = DYNLOG_WARNING;
  }
}

void dynlog_set_level(int level) {
  dynlog_level = level;
}

void dynlog_set_output_file(FILE* outfile) {
  if (dynlog_outfile != NULL && dynlog_outfile != stderr) {
    fclose(dynlog_outfile);
  }
  dynlog_outfile = outfile;
}

void dynlog_vlog(int level, const char* fmt, va_list args) {
  if (level > dynlog_level) return;
  if (dynlog_outfile == NULL) return;
  vfprintf(dynlog_outfile, fmt, args);
  fflush(dynlog_outfile);
}
