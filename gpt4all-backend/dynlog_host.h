// this file should only be included from files that are part of or link against 
// gpt4all-backend directly (i.e. not from model libraries that are dlopen'd)
#ifndef _DYNLOG_HOST_H
#define _DYNLOG_HOST_H
#include "dynlog.h"
#include <stdio.h>
void dynlog_set_level(int level);
void dynlog_set_output_file(FILE* outfile);
#endif