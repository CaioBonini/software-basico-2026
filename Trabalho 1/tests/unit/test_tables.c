/* Testes isolados de opcode_table e symbol_table (sem lexer/montador).
 * Uso: make unit */
#include <stdio.h>
#include <string.h>

#include "opcode_table.h"
#include "symbol_table.h"

static int checks = 0;
static int failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        checks++;                                                            \
        if (!(cond)) {                                                       \
            failures++;                                                      \
            printf("  FALHA %s:%d: %s\n", __FILE__, __LINE__, #cond);        \
        }                                                                    \
    } while (0)

static void test_opcodes(void)
{
    static const struct {
        const char *mnemonic;
        Opcode opcode;
        int n_operands;
        int n_words;
    } expected[] = {
        {"ADD", 1, 1, 2},    {"SUB", 2, 1, 2},    {"MULT", 3, 1, 2},
        {"MUL", 3, 1, 2},    {"DIV", 4, 1, 2},    {"JMP", 5, 1, 2},
        {"JMPN", 6, 1, 2},   {"JMPP", 7, 1, 2},   {"JMPZ", 8, 1, 2},
        {"COPY", 9, 2, 3},   {"LOAD", 10, 1, 2},  {"STORE", 11, 1, 2},
        {"INPUT", 12, 1, 2}, {"OUTPUT", 13, 1, 2}, {"STOP", 14, 0, 1},
    };

    printf("opcode_table\n");
    for (size_t i = 0; i < sizeof expected / sizeof expected[0]; i++) {
        const Instruction *ins = opcode_lookup(expected[i].mnemonic);
        CHECK(ins != NULL);
        if (ins == NULL)
            continue;
        CHECK(ins->opcode == expected[i].opcode);
        CHECK(ins->n_operands == expected[i].n_operands);
        CHECK(ins->n_words == expected[i].n_words);
    }
    CHECK(opcode_lookup("MOV") == NULL);
    CHECK(opcode_lookup("") == NULL);
    /* A caixa é normalizada pelo lexer; a tabela só conhece maiúsculas. */
    CHECK(opcode_lookup("add") == NULL);
}

/* Confere se a cadeia de pendências de sym, lida no vetor de código, passa
 * exatamente pelas posições em expected (da cabeça até o -1). */
static int chain_is(const Symbol *sym, const int *code, const int *expected,
                    int n)
{
    int pos = sym->list;
    for (int i = 0; i < n; i++) {
        if (pos != expected[i])
            return 0;
        pos = code[pos];
    }
    return pos == LIST_END;
}

static void test_symbol_table(void)
{
    enum { CODE_SIZE = 20, UNSET = 999 };
    int code[CODE_SIZE];
    int pending_offset[CODE_SIZE] = {0};
    for (int i = 0; i < CODE_SIZE; i++)
        code[i] = UNSET;

    SymbolTable table;
    symtab_init(&table);

    printf("symbol_table: 1) X referenciado em 3 posicoes antes de definido\n");
    symtab_reference(&table, "X", 0, 1, code, pending_offset);
    symtab_reference(&table, "Y", 0, 3, code, pending_offset); /* intercalado */
    symtab_reference(&table, "X", 2, 5, code, pending_offset); /* X + 2 */
    symtab_reference(&table, "X", 0, 9, code, pending_offset);

    Symbol *x = symtab_find(&table, "X");
    CHECK(x != NULL);
    CHECK(!x->defined);
    CHECK(x->list == 9);
    CHECK(code[9] == 5);
    CHECK(pending_offset[5] == 2); /* X + 2 guardado ao lado da cadeia */
    CHECK(code[5] == 1);
    CHECK(code[1] == LIST_END);
    CHECK(chain_is(x, code, (const int[]){9, 5, 1}, 3));
    Symbol *y = symtab_find(&table, "Y");
    CHECK(y != NULL && !y->defined);
    CHECK(chain_is(y, code, (const int[]){3}, 1));

    printf("symbol_table: 2) X definido no endereco 12\n");
    CHECK(symtab_define(&table, "X", 12, code, pending_offset) == SYM_OK);
    x = symtab_find(&table, "X");
    CHECK(x->defined);
    CHECK(x->value == 12);
    CHECK(x->list == LIST_END);
    CHECK(code[1] == 12);
    CHECK(code[5] == 14); /* 12 + 2 */
    CHECK(code[9] == 12);
    CHECK(pending_offset[5] == 0); /* limpo depois de resolvido */
    CHECK(code[3] == LIST_END); /* cadeia de Y intacta */
    /* Referência depois da definição grava o endereço direto. */
    symtab_reference(&table, "X", 1, 15, code, pending_offset);
    CHECK(code[15] == 13);
    CHECK(x->list == LIST_END);

    printf("symbol_table: 3) X redefinido\n");
    CHECK(symtab_define(&table, "X", 17, code, pending_offset) ==
          SYM_REDEFINED);
    x = symtab_find(&table, "X");
    CHECK(x->value == 12);
    CHECK(code[1] == 12 && code[5] == 14 && code[9] == 12 && code[15] == 13);
    /* Rótulo definido sem referência prévia. */
    CHECK(symtab_define(&table, "Z", 18, code, pending_offset) == SYM_OK);

    printf("symbol_table: 4) simbolo nunca declarado\n");
    int undefined = 0;
    const char *undefined_name = NULL;
    for (size_t i = 0; i < table.count; i++) {
        if (!table.items[i].defined) {
            undefined++;
            undefined_name = table.items[i].name;
        }
    }
    CHECK(undefined == 1);
    CHECK(undefined_name != NULL && strcmp(undefined_name, "Y") == 0);
    CHECK(symtab_find(&table, "NUNCA") == NULL);

    symtab_free(&table);
}

int main(void)
{
    test_opcodes();
    test_symbol_table();
    printf("\n%d verificacoes, %d falharam\n", checks, failures);
    return failures != 0;
}
