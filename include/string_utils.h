#pragma once

#include <stdio.h>
#include <stdarg.h>

struct sstring_s {
    char* cstr;
};

typedef struct sstring_s* sstring;

sstring sfromcstr(const char* cstr);

void sassignformat(sstring string, const char* format, ...);

void sdestroy(sstring string);