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

void errors_print(const ErrorList *errors, FILE *out)
{
    /* Ordenação por inserção pela linha: estável e a lista é pequena. */
    size_t n = errors->count;
    const Error **sorted = malloc((n ? n : 1) * sizeof *sorted);
    if (sorted == NULL) {
        fprintf(stderr, "erro: memoria insuficiente\n");
        exit(1);
    }
    for (size_t i = 0; i < n; i++) {
        const Error *err = &errors->items[i];
        size_t j = i;
        for (; j > 0 && sorted[j - 1]->line > err->line; j--)
            sorted[j] = sorted[j - 1];
        sorted[j] = err;
    }
    for (size_t i = 0; i < n; i++) {
        if (sorted[i]->line > 0)
            fprintf(out, "Linha %d: erro %s: %s\n", sorted[i]->line,
                    error_type_name(sorted[i]->type), sorted[i]->message);
        else
            fprintf(out, "Erro %s: %s\n", error_type_name(sorted[i]->type),
                    sorted[i]->message);
    }
    free(sorted);
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
