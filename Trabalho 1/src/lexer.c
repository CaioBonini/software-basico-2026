#include "lexer.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_MAX_TOKENS 32

typedef enum {
    TOK_IDENT,
    TOK_NUMBER,
    TOK_COLON,
    TOK_PLUS,
    TOK_INVALID /* já reportado como LEXICO; mantido para não gerar erros em
                   cascata na análise da estrutura da linha */
} TokenKind;

typedef struct {
    TokenKind kind;
    char text[IDENT_MAX_LEN + 1]; /* TOK_IDENT, em maiúsculas */
    int value;                    /* TOK_NUMBER */
} Token;

static bool is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f';
}

static bool is_ident_char(char c)
{
    return isalnum((unsigned char)c) || c == '_';
}

/* [-]dígitos, em decimal, cabendo num int. */
static bool parse_number(const char *s, size_t len, int *value,
                         bool *out_of_range)
{
    size_t i = (s[0] == '-') ? 1 : 0;
    if (i == len)
        return false;
    long long v = 0;
    *out_of_range = false;
    for (; i < len; i++) {
        if (!isdigit((unsigned char)s[i]))
            return false;
        if (v <= INT_MAX)
            v = v * 10 + (s[i] - '0');
    }
    if (s[0] == '-')
        v = -v;
    if (v > INT_MAX || v < INT_MIN) {
        *out_of_range = true;
        return false;
    }
    *value = (int)v;
    return true;
}

/* Quebra [p, end) em tokens, parando no ';' de comentário. */
static int tokenize(const char *p, const char *end, int line, Token *tokens,
                    ErrorList *errors, bool *has_error)
{
    int n = 0;
    while (p < end && *p != ';') {
        char c = *p;
        if (is_space(c)) {
            p++;
            continue;
        }
        const char *start = p;
        Token tok;
        if (c == ':' || c == '+') {
            tok.kind = (c == ':') ? TOK_COLON : TOK_PLUS;
            p++;
        } else if (is_ident_char(c) ||
                   (c == '-' && p + 1 < end && isdigit((unsigned char)p[1]))) {
            p++;
            while (p < end && is_ident_char(*p))
                p++;
            int len = (int)(p - start);
            if (isdigit((unsigned char)c) || c == '-') {
                bool out_of_range;
                if (!parse_number(start, (size_t)len, &tok.value,
                                  &out_of_range)) {
                    if (out_of_range)
                        report_error(errors, line, ERR_LEXICO,
                                     "numero fora do intervalo '%.*s'", len,
                                     start);
                    else
                        report_error(errors, line, ERR_LEXICO,
                                     "token invalido '%.*s' (identificador "
                                     "nao pode comecar com digito)",
                                     len, start);
                    *has_error = true;
                    tok.kind = TOK_INVALID;
                } else {
                    tok.kind = TOK_NUMBER;
                    /* texto guardado só para mensagens (ex.: "123:") */
                    int n_chars = len < IDENT_MAX_LEN ? len : IDENT_MAX_LEN;
                    memcpy(tok.text, start, (size_t)n_chars);
                    tok.text[n_chars] = '\0';
                }
            } else if (len > IDENT_MAX_LEN) {
                report_error(errors, line, ERR_LEXICO,
                             "identificador com mais de %d caracteres '%.*s'",
                             IDENT_MAX_LEN, len, start);
                *has_error = true;
                tok.kind = TOK_INVALID;
            } else {
                tok.kind = TOK_IDENT;
                for (int i = 0; i < len; i++)
                    tok.text[i] = (char)toupper((unsigned char)start[i]);
                tok.text[len] = '\0';
            }
        } else {
            while (p < end && !is_space(*p) && *p != ';' && *p != ':' &&
                   *p != '+' && !is_ident_char(*p))
                p++;
            report_error(errors, line, ERR_LEXICO, "caractere invalido '%.*s'",
                         (int)(p - start), start);
            *has_error = true;
            tok.kind = TOK_INVALID;
        }
        if (n == LINE_MAX_TOKENS) {
            report_error(errors, line, ERR_SINTATICO, "linha longa demais");
            *has_error = true;
            break;
        }
        tokens[n++] = tok;
    }
    return n;
}

static void add_operand(Line *out, Operand operand)
{
    if (out->n_operands < LINE_MAX_OPERANDS)
        out->operands[out->n_operands] = operand;
    out->n_operands++;
}

/* rótulo:* operação operando*, com operando = IDENT [+ NUMERO] | NUMERO. */
static void parse_tokens(const Token *tokens, int n, int line, Line *out,
                         ErrorList *errors)
{
    int i = 0;
    while (i + 1 < n && tokens[i].kind != TOK_COLON &&
           tokens[i].kind != TOK_PLUS && tokens[i + 1].kind == TOK_COLON) {
        if (tokens[i].kind == TOK_INVALID) {
            /* rótulo malformado: já reportado, só consome "X:" */
        } else if (tokens[i].kind == TOK_NUMBER) {
            /* "123:" é um número bem formado, mas como rótulo viola a mesma
             * regra de identificador que "1ABC:" */
            report_error(errors, line, ERR_LEXICO,
                         "rotulo invalido '%s' (identificador nao pode "
                         "comecar com digito)",
                         tokens[i].text);
            out->has_error = true;
        } else if (out->has_label) {
            report_error(errors, line, ERR_SINTATICO,
                         "mais de um rotulo na mesma linha ('%s' e '%s')",
                         out->label, tokens[i].text);
            out->has_error = true;
        } else {
            out->has_label = true;
            strcpy(out->label, tokens[i].text);
            out->label_line = line;
        }
        i += 2;
    }
    if (i == n)
        return;

    if (tokens[i].kind == TOK_INVALID)
        return; /* operação malformada: já reportada */
    if (tokens[i].kind != TOK_IDENT) {
        report_error(errors, line, ERR_SINTATICO,
                     "esperava instrucao ou diretiva no inicio da linha");
        out->has_error = true;
        return;
    }
    out->has_op = true;
    strcpy(out->op, tokens[i].text);
    i++;

    while (i < n) {
        Operand operand = {0};
        if (tokens[i].kind == TOK_IDENT) {
            operand.kind = OPERAND_SYMBOL;
            strcpy(operand.name, tokens[i].text);
            i++;
            if (i < n && tokens[i].kind == TOK_PLUS) {
                if (i + 1 < n && tokens[i + 1].kind == TOK_NUMBER) {
                    operand.value = tokens[i + 1].value;
                    i += 2;
                } else {
                    report_error(errors, line, ERR_SINTATICO,
                                 "esperava numero depois de '%s +'",
                                 operand.name);
                    out->has_error = true;
                    i++;
                }
            }
        } else if (tokens[i].kind == TOK_INVALID) {
            operand.kind = OPERAND_INVALID; /* conta para a quantidade */
            i++;
        } else if (tokens[i].kind == TOK_NUMBER) {
            operand.kind = OPERAND_NUMBER;
            operand.value = tokens[i].value;
            i++;
        } else if (tokens[i].kind == TOK_PLUS && i + 1 < n &&
                   tokens[i + 1].kind == TOK_NUMBER) {
            operand.kind = OPERAND_NUMBER; /* sinal explícito: CONST +5 */
            operand.value = tokens[i + 1].value;
            i += 2;
        } else {
            report_error(errors, line, ERR_SINTATICO, "'%c' fora de lugar",
                         tokens[i].kind == TOK_COLON ? ':' : '+');
            out->has_error = true;
            i++;
            continue;
        }
        add_operand(out, operand);
    }
}

/* SECAO -> SECTION; BEGIN: NOME -> NOME: BEGIN (idem EXTERN). */
static void canonicalize(Line *line)
{
    if (line->has_op && strcmp(line->op, "SECAO") == 0)
        strcpy(line->op, "SECTION");

    if (line->has_label && line->has_op && line->n_operands == 0 &&
        (strcmp(line->label, "BEGIN") == 0 ||
         strcmp(line->label, "EXTERN") == 0)) {
        char name[IDENT_MAX_LEN + 1];
        strcpy(name, line->op);
        strcpy(line->op, line->label);
        strcpy(line->label, name);
    }
}

static void push_line(LineList *lines, const Line *line)
{
    if (lines->count == lines->capacity) {
        size_t capacity = lines->capacity ? lines->capacity * 2 : 64;
        Line *items = realloc(lines->items, capacity * sizeof *items);
        if (items == NULL) {
            fprintf(stderr, "erro: memoria insuficiente\n");
            exit(1);
        }
        lines->items = items;
        lines->capacity = capacity;
    }
    lines->items[lines->count++] = *line;
}

void lexer_run(const char *source, LineList *lines, ErrorList *errors)
{
    lines->items = NULL;
    lines->count = 0;
    lines->capacity = 0;

    Line pending = {0}; /* rótulo sozinho esperando a próxima linha */
    bool has_pending = false;
    int line_no = 0;
    const char *p = source;
    /* BOM UTF-8 (EF BB BF), gravado por alguns editores do Windows no
     * início do arquivo: não é conteúdo, só marca a codificação. */
    if (strncmp(p, "\xEF\xBB\xBF", 3) == 0)
        p += 3;

    while (*p != '\0') {
        const char *end = strchr(p, '\n');
        if (end == NULL)
            end = p + strlen(p);
        line_no++;

        Token tokens[LINE_MAX_TOKENS];
        Line line = {0};
        line.line = line_no;
        int n = tokenize(p, end, line_no, tokens, errors, &line.has_error);
        parse_tokens(tokens, n, line_no, &line, errors);
        p = (*end == '\n') ? end + 1 : end;

        if (!line.has_label && !line.has_op)
            continue; /* vazia, só comentário, ou só tokens inválidos */

        if (line.has_label && !line.has_op && line.n_operands == 0) {
            if (has_pending) {
                report_error(errors, line_no, ERR_SINTATICO,
                             "mais de um rotulo para a mesma linha ('%s' e "
                             "'%s')",
                             pending.label, line.label);
                pending.has_error = true;
            } else {
                pending = line;
                has_pending = true;
            }
            continue;
        }

        if (has_pending) {
            if (line.has_label) {
                report_error(errors, line.label_line, ERR_SINTATICO,
                             "mais de um rotulo para a mesma linha ('%s' e "
                             "'%s')",
                             pending.label, line.label);
                line.has_error = true;
            } else {
                line.has_label = true;
                strcpy(line.label, pending.label);
                line.label_line = pending.label_line;
                line.has_error = line.has_error || pending.has_error;
            }
            has_pending = false;
        }
        canonicalize(&line);
        push_line(lines, &line);
    }

    if (has_pending)
        push_line(lines, &pending); /* rótulo no fim do arquivo */
}

void lines_free(LineList *lines)
{
    free(lines->items);
    lines->items = NULL;
    lines->count = 0;
    lines->capacity = 0;
}

void line_format(const Line *line, char *buf, size_t size)
{
    size_t used = 0;
#define APPEND(...)                                                          \
    do {                                                                     \
        if (used < size)                                                     \
            used += (size_t)snprintf(buf + used, size - used, __VA_ARGS__); \
    } while (0)

    buf[0] = '\0';
    if (line->has_label)
        APPEND("%s:", line->label);
    if (line->has_op)
        APPEND("%s%s", line->has_label ? " " : "", line->op);
    int stored = line->n_operands < LINE_MAX_OPERANDS ? line->n_operands
                                                      : LINE_MAX_OPERANDS;
    for (int i = 0; i < stored; i++) {
        const Operand *op = &line->operands[i];
        if (op->kind == OPERAND_INVALID)
            APPEND(" ?");
        else if (op->kind == OPERAND_NUMBER)
            APPEND(" %d", op->value);
        else if (op->value != 0)
            APPEND(" %s + %d", op->name, op->value);
        else
            APPEND(" %s", op->name);
    }
#undef APPEND
}
