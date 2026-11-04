/* Testes isolados do lexer/normalizador (sem montador).
 * Uso: make unit (roda a partir de Trabalho 1/, por causa dos .asm reais) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errors.h"
#include "lexer.h"

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

static LineList lines;
static ErrorList errors;

static void lex(const char *source)
{
    lines_free(&lines);
    errors_free(&errors);
    lexer_run(source, &lines, &errors);
}

/* Linha lógica idx, já normalizada, com a linha original esperada. */
static int line_is(size_t idx, const char *text, int original_line)
{
    char buf[256];
    if (idx >= lines.count)
        return 0;
    line_format(&lines.items[idx], buf, sizeof buf);
    if (strcmp(buf, text) != 0 || lines.items[idx].line != original_line) {
        printf("        linha %zu: obtido \"%s\" (linha %d), esperado \"%s\" "
               "(linha %d)\n",
               idx, buf, lines.items[idx].line, text, original_line);
        return 0;
    }
    return 1;
}

/* O único erro reportado tem esse tipo e essa linha. */
static int single_error(ErrorType type, int line)
{
    if (errors.count != 1) {
        printf("        esperava 1 erro, veio %zu\n", errors.count);
        for (size_t i = 0; i < errors.count; i++)
            printf("          linha %d %s: %s\n", errors.items[i].line,
                   error_type_name(errors.items[i].type),
                   errors.items[i].message);
        return 0;
    }
    return errors.items[0].type == type && errors.items[0].line == line;
}

static void test_comments(void)
{
    printf("lexer: comentarios\n");
    lex("ADD X ; soma X\n"
        "; linha so de comentario\n"
        "   ;indentado\n"
        "SUB Y;colado\n");
    CHECK(lines.count == 2);
    CHECK(line_is(0, "ADD X", 1));
    CHECK(line_is(1, "SUB Y", 4));
    CHECK(errors.count == 0);
    /* Caracteres inválidos dentro do comentário não são erro. */
    lex("STOP ; $#@! ç\n");
    CHECK(line_is(0, "STOP", 1));
    CHECK(errors.count == 0);
}

static void test_case(void)
{
    printf("lexer: maiusculas/minusculas\n");
    lex("add x\nPublic Old_Data\nl1: Stop\n");
    CHECK(line_is(0, "ADD X", 1));
    CHECK(line_is(1, "PUBLIC OLD_DATA", 2));
    CHECK(line_is(2, "L1: STOP", 3));
    CHECK(strcmp(lines.items[2].label, "L1") == 0);
}

static void test_spacing(void)
{
    printf("lexer: espacos, tabs, linhas em branco, \\r\\n\n");
    lex("\n"
        "   LOAD\t\t OLD_DATA   \n"
        "\t\n"
        "L1:DIV DOIS\r\n"
        "L2 :  MULT\tDOIS\r\n"
        "\n");
    CHECK(lines.count == 3);
    CHECK(line_is(0, "LOAD OLD_DATA", 2));
    CHECK(line_is(1, "L1: DIV DOIS", 4));
    CHECK(line_is(2, "L2: MULT DOIS", 5));
    CHECK(errors.count == 0);
    /* Arquivo sem '\n' no fim. */
    lex("STOP");
    CHECK(lines.count == 1 && line_is(0, "STOP", 1));
}

static void test_sections(void)
{
    printf("lexer: SECTION/SECAO\n");
    lex("SECTION TEXT\nsecao data\nSecao Text\n");
    CHECK(line_is(0, "SECTION TEXT", 1));
    CHECK(line_is(1, "SECTION DATA", 2));
    CHECK(line_is(2, "SECTION TEXT", 3));
}

static void test_label_alone(void)
{
    printf("lexer: rotulo sozinho na linha\n");
    lex("L1:\n"
        "\n"
        "; comentario no meio\n"
        "DIV DOIS\n"
        "STOP\n");
    CHECK(lines.count == 2);
    CHECK(line_is(0, "L1: DIV DOIS", 4)); /* linha da instrução */
    CHECK(lines.items[0].label_line == 1); /* linha do rótulo */
    CHECK(line_is(1, "STOP", 5));
    CHECK(errors.count == 0);

    lex("FIM:\n"); /* rótulo no fim do arquivo continua existindo */
    CHECK(lines.count == 1 && line_is(0, "FIM:", 1));

    lex("A:\nB:\nSTOP\n");
    CHECK(single_error(ERR_SINTATICO, 2));
    lex("A:\nB: STOP\n");
    CHECK(single_error(ERR_SINTATICO, 2));
    lex("A: B: STOP\n");
    CHECK(single_error(ERR_SINTATICO, 1));
}

static void test_original_lines(void)
{
    printf("lexer: numero da linha original\n");
    lex("; cabecalho\n"     /* 1 */
        "SECTION TEXT\n"    /* 2 */
        "\n"                /* 3 */
        "INPUT N ; le\n"    /* 4 */
        "  \n"              /* 5 */
        "LACO:\n"           /* 6 */
        "  OUTPUT N\n"      /* 7 */
        "STOP\n"            /* 8 */
        "SECTION DATA\n"    /* 9 */
        "N: SPACE\n");      /* 10 */
    CHECK(lines.count == 6);
    CHECK(line_is(0, "SECTION TEXT", 2));
    CHECK(line_is(1, "INPUT N", 4));
    CHECK(line_is(2, "LACO: OUTPUT N", 7));
    CHECK(lines.items[2].label_line == 6);
    CHECK(line_is(3, "STOP", 8));
    CHECK(line_is(4, "SECTION DATA", 9));
    CHECK(line_is(5, "N: SPACE", 10));
    CHECK(lines.items[5].label_line == 10);
}

static void test_begin_extern(void)
{
    printf("lexer: BEGIN/EXTERN nas duas ordens\n");
    lex("MOD_A: BEGIN\nBEGIN: MOD1\nY: EXTERN\nEXTERN: MOD2\nbegin:mod3\n");
    CHECK(line_is(0, "MOD_A: BEGIN", 1));
    CHECK(line_is(1, "MOD1: BEGIN", 2));
    CHECK(line_is(2, "Y: EXTERN", 3));
    CHECK(line_is(3, "MOD2: EXTERN", 4));
    CHECK(line_is(4, "MOD3: BEGIN", 5));
    CHECK(strcmp(lines.items[1].label, "MOD1") == 0);
    CHECK(strcmp(lines.items[1].op, "BEGIN") == 0);
    CHECK(errors.count == 0);
    /* PUBLIC tem a forma normal (diretiva + operando), não é invertido. */
    lex("PUBLIC L1\n");
    CHECK(line_is(0, "PUBLIC L1", 1));
    CHECK(!lines.items[0].has_label);
}

static void test_operands(void)
{
    printf("lexer: operandos IDENT (+ NUMERO)?\n");
    lex("STORE Y + 2\n"
        "STORE Y+2\n"
        "LOAD XY\n"
        "COPY A + 1 B\n"
        "CONST -5\n"
        "SPACE 3\n"
        "CONST +7\n");
    CHECK(errors.count == 0);
    CHECK(line_is(0, "STORE Y + 2", 1));
    const Line *l = &lines.items[0];
    CHECK(l->n_operands == 1 && l->operands[0].kind == OPERAND_SYMBOL &&
          strcmp(l->operands[0].name, "Y") == 0 && l->operands[0].value == 2);
    CHECK(line_is(1, "STORE Y + 2", 2));
    l = &lines.items[2];
    CHECK(l->n_operands == 1 && l->operands[0].kind == OPERAND_SYMBOL &&
          l->operands[0].value == 0); /* referência simples: deslocamento 0 */
    l = &lines.items[3];
    CHECK(line_is(3, "COPY A + 1 B", 4));
    CHECK(l->n_operands == 2 && l->operands[0].value == 1 &&
          strcmp(l->operands[1].name, "B") == 0);
    l = &lines.items[4];
    CHECK(l->n_operands == 1 && l->operands[0].kind == OPERAND_NUMBER &&
          l->operands[0].value == -5);
    CHECK(lines.items[5].operands[0].value == 3);
    CHECK(lines.items[6].operands[0].kind == OPERAND_NUMBER &&
          lines.items[6].operands[0].value == 7);

    /* Operandos demais ainda são contados (o montador acusa o excesso). */
    lex("ADD A B C D E F\n");
    CHECK(lines.items[0].n_operands == 6);
    CHECK(errors.count == 0);

    lex("STORE Y +\n");
    CHECK(single_error(ERR_SINTATICO, 1) && lines.items[0].has_error);
    lex("STORE Y + Z\n");
    CHECK(errors.count >= 1 && errors.items[0].type == ERR_SINTATICO);
    lex("ADD X :\n");
    CHECK(single_error(ERR_SINTATICO, 1));
    /* ':' também separa: "ADD : X" é rótulo ADD + operação X (acusar rótulo
     * com nome de instrução é papel do montador). */
    lex("ADD : X\n");
    CHECK(errors.count == 0 && line_is(0, "ADD: X", 1));
    lex("5 ADD X\n");
    CHECK(single_error(ERR_SINTATICO, 1));
}

static void test_lexical_errors(void)
{
    printf("lexer: erros lexicos\n");
    /* Rótulo malformado: um só erro (LEXICO), sem cascata, e o resto da
     * linha continua reconhecido. */
    lex("1ABC: ADD X\n");
    CHECK(single_error(ERR_LEXICO, 1));
    CHECK(line_is(0, "ADD X", 1) && !lines.items[0].has_label &&
          lines.items[0].has_error);
    lex("SECTION TEXT\nADD X$\n");
    CHECK(single_error(ERR_LEXICO, 2) && lines.items[1].has_error);
    lex("COPY A,B\n"); /* COPY é sem vírgula */
    CHECK(single_error(ERR_LEXICO, 1));
    /* O token inválido ainda conta como operando. */
    CHECK(line_is(0, "COPY A ? B", 1) && lines.items[0].n_operands == 3);
    lex("CONST 0X10\n"); /* só decimal */
    CHECK(single_error(ERR_LEXICO, 1));
    lex("CONST 99999999999\n");
    CHECK(single_error(ERR_LEXICO, 1));
    lex("SUB X - 2\n");
    CHECK(single_error(ERR_LEXICO, 1));

    /* 30 caracteres: ok; 31: erro. */
    lex("A23456789012345678901234567890: STOP\n");
    CHECK(errors.count == 0 && lines.items[0].has_label);
    lex("A234567890123456789012345678901: STOP\n");
    CHECK(single_error(ERR_LEXICO, 1));
    lex("_TMP: SPACE\nV_2: CONST 1\n");
    CHECK(errors.count == 0);

    /* O lexer não para no primeiro erro. */
    lex("ADD $\nSUB #\nSTOP\n");
    CHECK(errors.count == 2 && errors.items[0].line == 1 &&
          errors.items[1].line == 2);
    CHECK(line_is(lines.count - 1, "STOP", 3));
}

static char *read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL)
        return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char *buf = malloc((size_t)size + 1);
    size_t got = fread(buf, 1, (size_t)size, f);
    buf[got] = '\0';
    fclose(f);
    return buf;
}

static void test_real_files(void)
{
    printf("lexer: tests/sem_ligacao/old_data.asm\n");
    char *src = read_file("tests/sem_ligacao/old_data.asm");
    CHECK(src != NULL);
    if (src != NULL) {
        static const char *expected[] = {
            "SECTION TEXT",   "INPUT OLD_DATA",     "LOAD OLD_DATA",
            "L1: DIV DOIS",   "STORE NEW_DATA",     "MULT DOIS",
            "STORE TMP_DATA", "LOAD OLD_DATA",      "SUB TMP_DATA",
            "STORE TMP_DATA", "OUTPUT TMP_DATA",    "COPY NEW_DATA OLD_DATA",
            "LOAD OLD_DATA",  "JMPP L1",            "STOP",
            "SECTION DATA",   "DOIS: CONST 2",      "OLD_DATA: SPACE",
            "NEW_DATA: SPACE", "TMP_DATA: SPACE",
        };
        lex(src);
        CHECK(errors.count == 0);
        CHECK(lines.count == 20);
        for (size_t i = 0; i < 20; i++)
            CHECK(line_is(i, expected[i], (int)i + 1));
        free(src);
    }

    printf("lexer: tests/com_ligacao/mod1.asm\n");
    src = read_file("tests/com_ligacao/mod1.asm");
    CHECK(src != NULL);
    if (src != NULL) {
        lex(src);
        CHECK(errors.count == 0);
        CHECK(line_is(1, "MOD1: BEGIN", 2));
        CHECK(line_is(2, "MOD2: EXTERN", 3));
        CHECK(line_is(3, "PUBLIC L2", 4));
        CHECK(line_is(7, "INPUT OLD_DATA", 9)); /* depois da linha em branco */
        CHECK(line_is(9, "JMP MOD2", 11));
        CHECK(line_is(11, "END", 13));
        free(src);
    }
}

int main(void)
{
    errors_init(&errors);
    test_comments();
    test_case();
    test_spacing();
    test_sections();
    test_label_alone();
    test_original_lines();
    test_begin_extern();
    test_operands();
    test_lexical_errors();
    test_real_files();
    lines_free(&lines);
    errors_free(&errors);
    printf("\n%d verificacoes, %d falharam\n", checks, failures);
    return failures != 0;
}
