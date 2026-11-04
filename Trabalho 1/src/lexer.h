/* Normalização do fonte em linhas lógicas + tokenização de uma linha.
 *
 * lexer_run() lê o fonte inteiro (já em memória) e gera uma linha lógica por
 * instrução/diretiva, já estruturada em rótulo / operação / operandos:
 *   - um BOM UTF-8 no início do arquivo é ignorado;
 *   - comentários (';' até o fim da linha) e linhas em branco somem;
 *   - espaços, tabs e '\r' são só separadores (':' e '+' também separam,
 *     então "L1:DIV" == "L1 : DIV" e "Y+2" == "Y + 2");
 *   - identificadores saem em maiúsculas;
 *   - SECAO vira SECTION;
 *   - "BEGIN: NOME" / "EXTERN: NOME" viram "NOME: BEGIN" / "NOME: EXTERN";
 *   - rótulo sozinho numa linha é grudado na próxima linha com conteúdo.
 * Cada linha lógica guarda o número da linha ORIGINAL da operação e do
 * rótulo (podem diferir quando o rótulo estava sozinho).
 *
 * Erros léxicos (token malformado) e de estrutura da linha (':' ou '+' fora
 * de lugar, dois rótulos) vão para a ErrorList. Validar o mnemônico e a
 * quantidade de operandos é papel do montador. */
#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>
#include <stddef.h>

#include "errors.h"

#define IDENT_MAX_LEN 30 /* regra do enunciado; igual a SYMBOL_MAX_LEN */
#define LINE_MAX_OPERANDS 4

typedef enum {
    OPERAND_SYMBOL, /* ROTULO ou ROTULO + n */
    OPERAND_NUMBER, /* constante decimal, ex.: CONST -5, SPACE 3 */
    OPERAND_INVALID /* token malformado (erro léxico já reportado) */
} OperandKind;

typedef struct {
    OperandKind kind;
    char name[IDENT_MAX_LEN + 1]; /* só OPERAND_SYMBOL */
    int value; /* OPERAND_SYMBOL: deslocamento (0 sem "+ n"); NUMBER: valor */
} Operand;

typedef struct {
    int line; /* linha original da operação (ou do rótulo, se não há operação) */
    bool has_label;
    char label[IDENT_MAX_LEN + 1];
    int label_line;
    bool has_op;
    char op[IDENT_MAX_LEN + 1];
    /* Total de operandos encontrados; só os LINE_MAX_OPERANDS primeiros são
     * guardados (o montador só precisa do total para acusar excesso). */
    int n_operands;
    Operand operands[LINE_MAX_OPERANDS];
    bool has_error; /* houve erro léxico/sintático nesta linha */
} Line;

typedef struct {
    Line *items;
    size_t count;
    size_t capacity;
} LineList;

void lexer_run(const char *source, LineList *lines, ErrorList *errors);
void lines_free(LineList *lines);

/* Forma normalizada da linha em texto, ex.: "L1: STORE Y + 2". */
void line_format(const Line *line, char *buf, size_t size);

#endif
