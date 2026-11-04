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

#include <stdbool.h>
#include <stddef.h>

#include "errors.h"
#include "symbol_table.h"

#define MEMORY_SIZE 65536 /* palavras da máquina hipotética (2^16) */

/* address cabe na memória (0 a MEMORY_SIZE - 1)? Usado pelo montador e
 * pelo ligador. */
bool address_in_memory(int address);

/* Entrada da Tabela de Definições (TD): símbolo PUBLIC e seu endereço
 * local ao módulo. */
typedef struct {
    char name[SYMBOL_MAX_LEN + 1];
    int value;
} Definition;

/* Entrada da Tabela de Uso (TU): uso de símbolo EXTERN na posição position
 * do código (a palavra guarda o deslocamento da expressão, ex. o 2 de Y + 2). */
typedef struct {
    char name[SYMBOL_MAX_LEN + 1];
    int position;
} Use;

typedef struct {
    int *words;
    size_t count;
    /* relative[i] = 1 se words[i] é operando de endereço (relativo ao início
     * do módulo, ou externo); opcodes, CONST/SPACE e valores EQU ficam 0. */
    unsigned char *relative;
    Definition *defs; /* TD; vazia fora do modo ligador */
    size_t n_defs;
    Use *uses; /* TU; vazia fora do modo ligador */
    size_t n_uses;
} ObjectCode;

/* Monta o fonte inteiro (não para no primeiro erro). Se errors->count > 0
 * ao final, o conteúdo de out não deve ser usado. */
void assemble(const char *source, ObjectCode *out, ErrorList *errors);

/* Igual a assemble(), no modo "chamado pelo ligador": EXTERN entra na TS com
 * valor 0 e seus usos vão para a TU; PUBLIC vai para a TD; BEGIN é
 * obrigatório. */
void assemble_module(const char *source, ObjectCode *out, ErrorList *errors);

void object_code_free(ObjectCode *code);

/* Lê um arquivo texto inteiro para a memória (NULL se não conseguir). */
char *read_source_file(const char *path);

#endif
