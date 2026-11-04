/* Reporte de erro padronizado: linha original + tipo (LEXICO/SINTATICO/SEMANTICO).
 *
 * Os erros são acumulados numa ErrorList (não impressos na hora): o montador
 * monta o programa inteiro, e só no fim decide o que fazer com eles. */
#ifndef ERRORS_H
#define ERRORS_H

#include <stddef.h>

/* Os 6 erros do enunciado, classificados como na Aula 3. */
typedef enum {
    ERR_LEXICO,
    ERR_SINTATICO,
    ERR_SEMANTICO
} ErrorType;

#define ERROR_MSG_MAX 160

typedef struct {
    int line; /* linha do arquivo original */
    ErrorType type;
    char message[ERROR_MSG_MAX];
} Error;

typedef struct {
    Error *items;
    size_t count;
    size_t capacity;
} ErrorList;

void errors_init(ErrorList *errors);
void errors_free(ErrorList *errors);

void report_error(ErrorList *errors, int line, ErrorType type,
                  const char *fmt, ...)
    __attribute__((format(printf, 4, 5)));

const char *error_type_name(ErrorType type);

#endif
