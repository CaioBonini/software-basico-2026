#include "errors.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void errors_init(ErrorList *errors)
{
    errors->items = NULL;
    errors->count = 0;
    errors->capacity = 0;
}

void errors_free(ErrorList *errors)
{
    free(errors->items);
    errors_init(errors);
}

void report_error(ErrorList *errors, int line, ErrorType type,
                  const char *fmt, ...)
{
    if (errors->count == errors->capacity) {
        size_t capacity = errors->capacity ? errors->capacity * 2 : 16;
        Error *items = realloc(errors->items, capacity * sizeof *items);
        if (items == NULL) {
            fprintf(stderr, "erro: memoria insuficiente\n");
            exit(1);
        }
        errors->items = items;
        errors->capacity = capacity;
    }
    Error *err = &errors->items[errors->count++];
    err->line = line;
    err->type = type;
    va_list args;
    va_start(args, fmt);
    vsnprintf(err->message, sizeof err->message, fmt, args);
    va_end(args);
}

const char *error_type_name(ErrorType type)
{
    switch (type) {
    case ERR_LEXICO:
        return "LEXICO";
    case ERR_SINTATICO:
        return "SINTATICO";
    case ERR_SEMANTICO:
        return "SEMANTICO";
    }
    return "?";
}
