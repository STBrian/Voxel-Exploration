#pragma once

#include <stdio.h>
#include <stdarg.h>

struct sstring_s {
    char* cstr;
    uint32_t lenght;
};

typedef struct sstring_s* sstring;

sstring sfromcstr(const char* cstr);

void sassignformat(sstring string, const char* format, ...);

sstring sformat(const char* format, ...);

void scatcstr(sstring string, const char* cstr);

void sdestroy(sstring string);