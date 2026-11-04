/* Núcleo do montador: passagem única sobre as linhas lógicas.
 *
 * Código e dados são gerados em vetores separados (as seções podem vir em
 * qualquer ordem no fonte) e concatenados no fim: texto primeiro, dados
 * depois. Rótulos de texto são definidos na hora (endereço = contador de
 * posição do texto); rótulos de dados só no fim, quando o tamanho do texto
 * é conhecido (endereço = tamanho do texto + posição nos dados). Em ambos
 * os casos as referências anteriores à definição são resolvidas pela lista
 * de backpatch da symbol_table. */
#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stddef.h>

#include "errors.h"

typedef struct {
    int *words;
    size_t count;
} ObjectCode;

/* Monta o fonte inteiro (não para no primeiro erro). Se errors->count > 0
 * ao final, o conteúdo de out não deve ser usado. */
void assemble(const char *source, ObjectCode *out, ErrorList *errors);
void object_code_free(ObjectCode *code);

/* Lê um arquivo texto inteiro para a memória (NULL se não conseguir). */
char *read_source_file(const char *path);

#endif
