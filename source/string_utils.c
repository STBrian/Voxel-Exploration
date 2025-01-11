#include "string_utils.h"

#include <stdlib.h>
#include <string.h>

sstring sfromcstr(const char* cstr)
{
    sstring newstring = (sstring)calloc(1, sizeof(struct sstring_s));
    if (newstring == NULL)
        return NULL;

    newstring->cstr = (char*)malloc(sizeof(char)*(strlen(cstr) + 1));
    if (newstring->cstr == NULL)
    {
        free(newstring);
        return NULL;
    }

    strcpy(newstring->cstr, cstr);

    return newstring;
}

void sassigncstr(sstring string, const char* cstr)
{
    if (string->cstr != NULL)
        free(string->cstr);
    
    string->cstr = (char*)malloc(sizeof(char)*(strlen(cstr) + 1));
    if (string->cstr == NULL)
        return;

    strcpy(string->cstr, cstr);
    return;
}

void sassignformat(sstring string, const char* format, ...)
{
    if (string->cstr != NULL)
        free(string->cstr);

    va_list args;
    va_start(args, format);

    // Calculate string size
    int size = vsnprintf(NULL, 0, format, args) + 1;

    // Reset args list
    va_end(args);
    va_start(args, format);

    string->cstr = (char*)malloc(size);
    if (string->cstr == NULL) {
        va_end(args);
        return;
    }

    // Write formated string to buffer
    vsnprintf(string->cstr, size, format, args);

    // End args list
    va_end(args);
    return;
}

void sdestroy(sstring string)
{
    if (string->cstr != NULL)
    {
        free(string->cstr);
        string->cstr = NULL;
    }
    free(string);
    return;
}