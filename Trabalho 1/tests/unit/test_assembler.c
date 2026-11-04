/* Testes isolados do montador (assemble) e do object_writer.
 * Uso: make unit (roda a partir de Trabalho 1/, por causa dos .asm reais) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assembler.h"
#include "errors.h"
#include "object_writer.h"

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

static ObjectCode code;
static ErrorList errors;

static void run(const char *source)
{
    object_code_free(&code);
    errors_free(&errors);
    assemble(source, &code, &errors);
}

static void dump_errors(void)
{
    for (size_t i = 0; i < errors.count; i++)
        printf("          linha %d %s: %s\n", errors.items[i].line,
               error_type_name(errors.items[i].type), errors.items[i].message);
}

/* Montou sem erro e gerou exatamente essas palavras. */
static int words_are(const char *expected)
{
    char buf[1024] = "";
    size_t used = 0;
    for (size_t i = 0; i < code.count && used < sizeof buf; i++)
        used += (size_t)snprintf(buf + used, sizeof buf - used,
                                 i ? " %d" : "%d", code.words[i]);
    if (errors.count != 0 || strcmp(buf, expected) != 0) {
        printf("        obtido \"%s\", esperado \"%s\"\n", buf, expected);
        dump_errors();
        return 0;
    }
    return 1;
}

/* Exatamente um erro, com esse tipo e essa linha. */
static int single_error(ErrorType type, int line)
{
    if (errors.count == 1 && errors.items[0].type == type &&
        errors.items[0].line == line)
        return 1;
    printf("        esperava 1 erro %s na linha %d, veio:\n",
           error_type_name(type), line);
    dump_errors();
    return 0;
}

/* Existe um erro com esse tipo e essa linha (entre outros). */
static int has_error(ErrorType type, int line)
{
    for (size_t i = 0; i < errors.count; i++)
        if (errors.items[i].type == type && errors.items[i].line == line)
            return 1;
    printf("        esperava erro %s na linha %d, veio:\n",
           error_type_name(type), line);
    dump_errors();
    return 0;
}

static void test_real_file(void)
{
    printf("montador: old_data.asm == old_data.obj.esperado\n");
    char *src = read_source_file("tests/sem_ligacao/old_data.asm");
    CHECK(src != NULL);
    if (src == NULL)
        return;
    run(src);
    CHECK(words_are("12 29 10 29 4 28 11 30 3 28 11 31 10 29 2 31 11 31 13 "
                    "31 9 30 29 10 29 7 4 14 2 0 0 0"));
    free(src);
}

static void test_references(void)
{
    printf("montador: referencias para tras, para frente e com + n\n");
    run("SECTION TEXT\n"
        "JMP FIM\n"       /* 0 1: rótulo de texto ainda não visto */
        "L: LOAD V + 1\n" /* 2 3: dado, com deslocamento */
        "JMP L\n"         /* 4 5: rótulo de texto já visto */
        "FIM: STOP\n"     /* 6 */
        "SECTION DATA\n"
        "V: SPACE 2\n");  /* 7 8 */
    CHECK(words_are("5 6 10 8 5 2 14 0 0"));

    printf("montador: SECTION DATA antes de SECTION TEXT\n");
    run("SECTION DATA\n"
        "N: CONST -3\n"
        "M: SPACE\n"
        "SECTION TEXT\n"
        "LOAD N\n"
        "STORE M\n"
        "OUTPUT N\n"
        "STOP\n");
    CHECK(words_are("10 7 11 8 13 7 14 -3 0"));

    printf("montador: COPY, MULT e MUL\n");
    run("SECTION TEXT\nCOPY A B\nMULT A\nMUL B\nSTOP\n"
        "SECTION DATA\nA: CONST 4\nB: SPACE\n");
    CHECK(words_are("9 8 9 3 8 3 9 14 4 0"));
}

static void test_equ_if(void)
{
    printf("montador: EQU e IF\n");
    run("TAM: EQU 3\n"
        "UM: EQU 1\n"
        "ZERO: EQU 0\n"
        "SECTION TEXT\n"
        "IF UM\n"
        "OUTPUT V\n" /* incluída */
        "IF ZERO\n"
        "OUTPUT V\n" /* pulada */
        "STOP\n"
        "SECTION DATA\n"
        "V: CONST TAM\n"
        "W: SPACE TAM\n");
    CHECK(words_are("13 3 14 3 0 0 0"));

    run("SECTION TEXT\nIF NADA\nSTOP\n");
    CHECK(single_error(ERR_SEMANTICO, 2));
    run("FLAG: EQU 1\nSECTION TEXT\nSTOP\nSECTION DATA\nIF FLAG\n");
    CHECK(single_error(ERR_SEMANTICO, 5));
    run("SECTION TEXT\nX: EQU 1\nSTOP\n"); /* EQU depois de seção */
    CHECK(single_error(ERR_SEMANTICO, 2));
    run("EQU 1\nSECTION TEXT\nSTOP\n");
    CHECK(single_error(ERR_SINTATICO, 1));
}

static void test_linking_directives(void)
{
    printf("montador: BEGIN/PUBLIC/END nao quebram\n");
    run("SECTION TEXT\n"
        "PROG: BEGIN\n"
        "PUBLIC L1\n"
        "L1: STOP\n"
        "END\n"
        "SECTION DATA\n");
    CHECK(words_are("14"));
    run("SECTION TEXT\nBEGIN: PROG\nSTOP\nEND\n");
    CHECK(words_are("14"));
    run("SECTION TEXT\nSTOP\nEND\n");
    CHECK(single_error(ERR_SEMANTICO, 3)); /* END sem BEGIN */
    run("SECTION TEXT\nPROG: BEGIN\nSTOP\n");
    CHECK(single_error(ERR_SEMANTICO, 2)); /* BEGIN sem END */
}

static void test_errors(void)
{
    printf("montador: erro 1 - rotulo nao declarado (uma vez por uso)\n");
    run("SECTION TEXT\nJMP NADA\nLOAD NADA\nSTOP\n");
    CHECK(errors.count == 2 && has_error(ERR_SEMANTICO, 2) &&
          has_error(ERR_SEMANTICO, 3));

    printf("montador: erro 2 - rotulo repetido\n");
    run("SECTION TEXT\nL: STOP\nL: STOP\n");
    CHECK(single_error(ERR_SEMANTICO, 3));
    run("SECTION DATA\nX: SPACE\nSECTION TEXT\nX: STOP\n");
    CHECK(single_error(ERR_SEMANTICO, 4));
    run("X: EQU 1\nSECTION TEXT\nX: STOP\n");
    CHECK(single_error(ERR_SEMANTICO, 3));

    printf("montador: erro 3 - instrucao/diretiva invalida\n");
    run("SECTION TEXT\nMOV X\nSTOP\n");
    CHECK(single_error(ERR_SINTATICO, 2));

    printf("montador: erro 4 - secao TEXT ausente\n");
    run("SECTION DATA\nX: SPACE\n");
    CHECK(single_error(ERR_SEMANTICO, 0));
    run("");
    CHECK(single_error(ERR_SEMANTICO, 0));

    printf("montador: erro 5 - lexico\n");
    run("SECTION TEXT\nADD X$\nSTOP\nSECTION DATA\nX: SPACE\n");
    CHECK(single_error(ERR_LEXICO, 2)); /* sem cascata na contagem */
    run("SECTION TEXT\nCOPY A,B\nSTOP\nSECTION DATA\nA: SPACE\nB: SPACE\n");
    CHECK(single_error(ERR_LEXICO, 2));

    printf("montador: erro 6 - quantidade de operandos\n");
    static const char *wrong_count[] = {
        "ADD", "ADD X X", "STOP X", "COPY X", "COPY X X X", "JMP",
    };
    for (size_t i = 0; i < sizeof wrong_count / sizeof *wrong_count; i++) {
        char src[128];
        snprintf(src, sizeof src,
                 "SECTION TEXT\n%s\nSTOP\nSECTION DATA\nX: SPACE\n",
                 wrong_count[i]);
        run(src);
        CHECK(single_error(ERR_SINTATICO, 2));
    }
    run("SECTION TEXT\nSTOP\nSECTION DATA\nX: SPACE 1 2\nY: CONST\n");
    CHECK(errors.count == 2 && has_error(ERR_SINTATICO, 4) &&
          has_error(ERR_SINTATICO, 5));
    /* Com a quantidade errada, a instrução ainda ocupa seu tamanho. */
    run("SECTION TEXT\nADD\nL: JMP L\n");
    CHECK(single_error(ERR_SINTATICO, 2) && code.count == 4 &&
          code.words[3] == 2);

    printf("montador: operandos do tipo errado\n");
    run("SECTION TEXT\nJMP 5\nSTOP\n");
    CHECK(single_error(ERR_SINTATICO, 2));
    run("SECTION TEXT\nSTOP\nSECTION DATA\nX: CONST Y\nY: SPACE 0\n");
    CHECK(errors.count == 2 && has_error(ERR_SINTATICO, 4) &&
          has_error(ERR_SINTATICO, 5));
    run("SECTION CODIGO\nSECTION TEXT\nSTOP\n");
    CHECK(single_error(ERR_SINTATICO, 1));

    printf("montador: endereco fora da memoria (0 a 65535)\n");
    /* Texto = LOAD (0 1) + STOP (2): X fica no endereço 3. */
    run("SECTION TEXT\nLOAD X + 65532\nSTOP\nSECTION DATA\nX: SPACE\n");
    CHECK(words_are("10 65535 14 0")); /* limite: ainda cabe */
    run("SECTION TEXT\nLOAD X + 65533\nSTOP\nSECTION DATA\nX: SPACE\n");
    CHECK(single_error(ERR_SEMANTICO, 2));
    run("SECTION TEXT\nLOAD X + -4\nSTOP\nSECTION DATA\nX: SPACE\n");
    CHECK(single_error(ERR_SEMANTICO, 2));
    /* Rótulo definido depois do uso: checado no fim, na linha do uso. */
    run("SECTION TEXT\nJMP L + 70000\nL: STOP\n");
    CHECK(single_error(ERR_SEMANTICO, 2));
    /* Valor de EQU usado como endereço também é checado. */
    run("N: EQU 70000\nSECTION TEXT\nLOAD N\nSTOP\n");
    CHECK(single_error(ERR_SEMANTICO, 3));
    /* Operando externo guarda só o deslocamento: quem checa é o ligador. */
    ErrorList module_errors;
    errors_init(&module_errors);
    ObjectCode module;
    assemble_module("SECTION TEXT\nM: BEGIN\nY: EXTERN\nLOAD Y + 70000\n"
                    "STOP\nEND\n",
                    &module, &module_errors);
    CHECK(module_errors.count == 0);
    object_code_free(&module);
    errors_free(&module_errors);

    printf("montador: rotulo sem instrucao\n");
    run("SECTION TEXT\nSTOP\nFIM:\nSECTION DATA\nX: SPACE\n");
    CHECK(single_error(ERR_SINTATICO, 3));
    run("SECTION TEXT\nSTOP\nFIM:\n");
    CHECK(single_error(ERR_SINTATICO, 3));

    printf("montador: linha na secao errada\n");
    run("SECTION TEXT\nX: CONST 1\nSTOP\n");
    CHECK(single_error(ERR_SEMANTICO, 2));
    run("SECTION TEXT\nSTOP\nSECTION DATA\nADD X\nX: SPACE\n");
    CHECK(single_error(ERR_SEMANTICO, 4));
    run("STOP\nSECTION TEXT\nSTOP\n");
    CHECK(single_error(ERR_SEMANTICO, 1));

    printf("montador: varios erros, sem parar no primeiro\n");
    run("SECTION TEXT\nMOV A\nADD\nJMP NADA\nL: STOP\nL: STOP\n");
    CHECK(errors.count == 4 && has_error(ERR_SINTATICO, 2) &&
          has_error(ERR_SINTATICO, 3) && has_error(ERR_SEMANTICO, 4) &&
          has_error(ERR_SEMANTICO, 6));
}

static void test_object_path(void)
{
    printf("object_writer: nome do .obj\n");
    static const char *cases[][2] = {
        {"prog.asm", "prog.obj"},
        {"tests/sem_ligacao/old_data.asm", "tests/sem_ligacao/old_data.obj"},
        {"PROG.ASM", "PROG.obj"},
        {"sem_extensao", "sem_extensao.obj"},
        {"dir.v2/prog", "dir.v2/prog.obj"},
    };
    for (size_t i = 0; i < sizeof cases / sizeof *cases; i++) {
        char *path = object_path_for(cases[i][0]);
        CHECK(path != NULL && strcmp(path, cases[i][1]) == 0);
        free(path);
    }
}

int main(void)
{
    errors_init(&errors);
    test_real_file();
    test_references();
    test_equ_if();
    test_linking_directives();
    test_errors();
    test_object_path();
    object_code_free(&code);
    errors_free(&errors);
    printf("\n%d verificacoes, %d falharam\n", checks, failures);
    return failures != 0;
}
