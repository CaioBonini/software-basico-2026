/* Tabela de símbolos (Símbolo | Valor | Def | Lista) com lista de backpatch.
 *
 * Implementa o algoritmo de passagem única da seção 4.1 do CLAUDE.md: a lista
 * de referências pendentes de um símbolo é uma cadeia de índices guardada no
 * próprio vetor de código objeto (code[pos] = próxima posição da cadeia),
 * terminada em -1, com a cabeça na coluna Lista.
 *
 * Operandos com expressão (ROTULO + n) usados antes da definição guardam o
 * deslocamento n em pending_offset[pos], vetor paralelo a code, porque a
 * palavra em code[pos] está ocupada pelo elo da cadeia. Na definição, cada
 * posição pendente recebe endereço + pending_offset[pos], e pending_offset[pos]
 * volta a 0. Referência simples (sem + n) é só offset = 0. */
#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdbool.h>
#include <stddef.h>

#define SYMBOL_MAX_LEN 30
#define LIST_END (-1)

typedef struct {
    char name[SYMBOL_MAX_LEN + 1];
    int value;    /* endereço; só válido se defined */
    bool defined; /* coluna Def */
    int list;     /* coluna Lista: cabeça da cadeia em code, ou LIST_END */
} Symbol;

typedef struct {
    Symbol *items;
    size_t count;
    size_t capacity;
} SymbolTable;

typedef enum {
    SYM_OK,
    SYM_REDEFINED /* rótulo já definido: erro semântico, valor não muda */
} SymbolResult;

void symtab_init(SymbolTable *table);
void symtab_free(SymbolTable *table);

/* NULL se o símbolo nunca apareceu. */
Symbol *symtab_find(SymbolTable *table, const char *name);

/* Define name como rótulo no endereço address. Se havia referências
 * pendentes, percorre a cadeia e escreve address + deslocamento em cada
 * posição. */
SymbolResult symtab_define(SymbolTable *table, const char *name, int address,
                           int *code, int *pending_offset);

/* Registra o uso de name + offset como operando na posição pos do código.
 * Se name já está definido, escreve o endereço final em code[pos]; senão,
 * encadeia pos na lista de pendências de name. */
void symtab_reference(SymbolTable *table, const char *name, int offset,
                      int pos, int *code, int *pending_offset);

#endif
