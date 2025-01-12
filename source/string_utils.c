#include "string_utils.h"

#include <stdlib.h>
#include <string.h>

sstring sfromcstr(const char* cstr)
{
    sstring newstring = (sstring)malloc(sizeof(struct sstring_s));
    if (newstring == NULL)
        return NULL;

    int cstrlen = strlen(cstr) + 1;
    newstring->cstr = (char*)malloc(sizeof(char)*(cstrlen));
    if (newstring->cstr == NULL)
    {
        free(newstring);
        return NULL;
    }

    newstring->lenght = cstrlen;
    strcpy(newstring->cstr, cstr);
    return newstring;
}

void sassigncstr(sstring string, const char* cstr)
{
    int cstrlen = strlen(cstr) + 1;
    if (string->cstr != NULL && cstrlen != string->lenght)
    {
        free(string->cstr);
        string->cstr = (char*)malloc(sizeof(char)*(cstrlen));
    }
    else if (string->cstr == NULL)
        string->cstr = (char*)malloc(sizeof(char)*(cstrlen));

    if (string->cstr == NULL)
        return;

    string->lenght = cstrlen;
    strcpy(string->cstr, cstr);
    return;
}

void sassignformat(sstring string, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    // Calculate string size
    int size = vsnprintf(NULL, 0, format, args) + 1;

    if (string->cstr != NULL && size != string->lenght)
    {
        free(string->cstr);
        string->cstr = (char*)malloc(sizeof(char)*(size));
    }
    else if (string->cstr == NULL)
        string->cstr = (char*)malloc(sizeof(char)*(size));

    va_end(args);
    if (string->cstr == NULL)
        return;

    // Reset args list
    va_start(args, format);

    // Write formated string to buffer
    vsnprintf(string->cstr, size, format, args);
    string->lenght = size;

    // End args list
    va_end(args);
    return;
}

sstring sformat(const char* format, ...)
{
    sstring newstring = (sstring)malloc(sizeof(struct sstring_s));
    if (newstring == NULL)
        return NULL;

    va_list args;
    va_start(args, format);

    // Calculate string size
    int size = vsnprintf(NULL, 0, format, args) + 1;

    newstring->cstr = (char*)malloc(sizeof(char)*(size));

    va_end(args);
    if (newstring->cstr == NULL)
    {
        free(newstring);
        return NULL;
    }

    // Reset args list
    va_start(args, format);

    // Write formated string to buffer
    vsnprintf(newstring->cstr, size, format, args);
    newstring->lenght = size;

    // End args list
    va_end(args);
    return newstring;
}

void scatcstr(sstring string, const char* cstr)
{
    if (string->cstr != NULL)
    {
        int size = string->lenght + strlen(cstr); // string->lenght already counts '\0' character

        string->cstr = (char*)realloc(string->cstr, sizeof(char) * size);
        if (string->cstr != NULL)
        {
            strcat(string->cstr, cstr);
            string->lenght = size;
        }
    }
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