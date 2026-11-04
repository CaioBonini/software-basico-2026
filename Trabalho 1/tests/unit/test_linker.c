/* Testes isolados do modo ligador do montador e do ligador.
 * Uso: make unit (roda a partir de Trabalho 1/, por causa dos .asm reais) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assembler.h"
#include "errors.h"
#include "linker.h"
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

static void dump_errors(const ErrorList *errors)
{
    for (size_t i = 0; i < errors->count; i++)
        printf("          linha %d %s: %s\n", errors->items[i].line,
               error_type_name(errors->items[i].type),
               errors->items[i].message);
}

static int words_are(const ObjectCode *code, const char *expected)
{
    char buf[1024] = "";
    size_t used = 0;
    for (size_t i = 0; i < code->count && used < sizeof buf; i++)
        used += (size_t)snprintf(buf + used, sizeof buf - used,
                                 i ? " %d" : "%d", code->words[i]);
    if (strcmp(buf, expected) != 0) {
        printf("        obtido \"%s\", esperado \"%s\"\n", buf, expected);
        return 0;
    }
    return 1;
}

static int relative_is(const ObjectCode *code, const char *expected)
{
    char buf[512] = "";
    size_t used = 0;
    for (size_t i = 0; i < code->count && used < sizeof buf; i++)
        used += (size_t)snprintf(buf + used, sizeof buf - used,
                                 i ? " %d" : "%d", code->relative[i]);
    if (strcmp(buf, expected) != 0) {
        printf("        R obtido \"%s\", esperado \"%s\"\n", buf, expected);
        return 0;
    }
    return 1;
}

static int has_def(const ObjectCode *code, const char *name, int value)
{
    for (size_t i = 0; i < code->n_defs; i++)
        if (strcmp(code->defs[i].name, name) == 0)
            return code->defs[i].value == value;
    return 0;
}

static int has_use(const ObjectCode *code, const char *name, int position)
{
    for (size_t i = 0; i < code->n_uses; i++)
        if (strcmp(code->uses[i].name, name) == 0 &&
            code->uses[i].position == position)
            return 1;
    return 0;
}

static size_t link_sources(const char **sources, int n, ObjectCode *out,
                           ErrorList *errors);

/* Monta source no modo ligador; devolve a quantidade de erros. */
static size_t module_from(const char *source, ObjectCode *out)
{
    ErrorList errors;
    errors_init(&errors);
    assemble_module(source, out, &errors);
    size_t n = errors.count;
    if (n > 0)
        dump_errors(&errors);
    errors_free(&errors);
    return n;
}

static size_t module_from_file(const char *path, ObjectCode *out)
{
    char *src = read_source_file(path);
    if (src == NULL) {
        printf("        nao abriu %s\n", path);
        memset(out, 0, sizeof *out);
        return 1;
    }
    size_t n = module_from(src, out);
    free(src);
    return n;
}

/* Erros de assemble_module sem imprimir (para os casos que devem falhar). */
static ErrorList module_errors(const char *source)
{
    ErrorList errors;
    errors_init(&errors);
    ObjectCode code;
    assemble_module(source, &code, &errors);
    object_code_free(&code);
    return errors;
}

static int single_error(ErrorList *errors, ErrorType type, int line)
{
    int ok = errors->count == 1 && errors->items[0].type == type &&
             errors->items[0].line == line;
    if (!ok) {
        printf("        esperava 1 erro %s na linha %d, veio:\n",
               error_type_name(type), line);
        dump_errors(errors);
    }
    errors_free(errors);
    return ok;
}

static void test_module_mod1(void)
{
    printf("modo ligador: mod1.asm (exemplo do professor)\n");
    ObjectCode m;
    CHECK(module_from_file("tests/com_ligacao/mod1.asm", &m) == 0);
    CHECK(words_are(&m, "12 8 10 8 5 0 14 2 0 0 0"));
    CHECK(relative_is(&m, "0 1 0 1 0 1 0 0 0 0 0"));
    /* O exemplo do professor diz L2 7, mas pelo código L2 é 6. */
    CHECK(m.n_defs == 4 && has_def(&m, "L2", 6) &&
          has_def(&m, "OLD_DATA", 8) && has_def(&m, "NEW_DATA", 9) &&
          has_def(&m, "TMP_DATA", 10));
    CHECK(m.n_uses == 1 && has_use(&m, "MOD2", 5));
    object_code_free(&m);
}

static void test_module_mod_a(void)
{
    printf("modo ligador: mod_a.asm (EXTERN com + n)\n");
    ObjectCode m;
    CHECK(module_from_file("tests/com_ligacao/mod_a.asm", &m) == 0);
    /* STORE Y + 2 com Y externo grava 2; ADD Y grava 0. */
    CHECK(words_are(&m, "12 0 10 11 1 0 11 2 7 0 14 5"));
    CHECK(relative_is(&m, "0 1 0 1 0 1 0 1 0 1 0 0"));
    CHECK(m.n_uses == 4 && has_use(&m, "Y", 1) && has_use(&m, "Y", 5) &&
          has_use(&m, "Y", 7) && has_use(&m, "MOD_B", 9));
    CHECK(m.n_defs == 2 && has_def(&m, "VAL", 11) && has_def(&m, "L1", 10));
    object_code_free(&m);

    printf("modo ligador: uso antes do EXTERN vai para a TU\n");
    CHECK(module_from("SECTION TEXT\nM: BEGIN\nJMP X + 1\nX: EXTERN\nEND\n",
                      &m) == 0);
    CHECK(words_are(&m, "5 1") && m.n_uses == 1 && has_use(&m, "X", 1));
    object_code_free(&m);

    printf("modo ligador: cadeia de pendencias inteira vira TU no EXTERN\n");
    const char *late_extern = "SECTION TEXT\n"
                              "M: BEGIN\n"
                              "LOAD X\n"      /* 0 1: pendente, + 0 */
                              "STORE X + 2\n" /* 2 3: pendente, + 2 */
                              "JMP L\n"       /* 4 5: local, para frente */
                              "X: EXTERN\n"
                              "ADD X + 1\n"   /* 6 7: depois do EXTERN */
                              "L: STOP\n"     /* 8 */
                              "END\n";
    CHECK(module_from(late_extern, &m) == 0); /* sem "X nao declarado" */
    CHECK(words_are(&m, "10 0 11 2 5 8 1 1 14"));
    CHECK(relative_is(&m, "0 1 0 1 0 1 0 1 0"));
    CHECK(m.n_uses == 3 && has_use(&m, "X", 1) && has_use(&m, "X", 3) &&
          has_use(&m, "X", 7));
    object_code_free(&m);

    printf("ligador: modulo com EXTERN depois do uso\n");
    const char *late_pair[] = {
        late_extern,
        "SECTION TEXT\nN: BEGIN\nPUBLIC X\nSTOP\nEND\n"
        "SECTION DATA\nX: SPACE 3\n",
    };
    ObjectCode out;
    ErrorList errors;
    errors_init(&errors);
    CHECK(link_sources(late_pair, 2, &out, &errors) == 0);
    /* Fator do 2º módulo = 9; TGD[X] = 1 + 9 = 10. */
    CHECK(words_are(&out, "10 10 11 12 5 8 1 11 14 14 0 0 0"));
    object_code_free(&out);
    errors_free(&errors);

    printf("modo ligador: EQU nao e endereco (R = 0)\n");
    CHECK(module_from("N: EQU 4\nSECTION TEXT\nM: BEGIN\nLOAD N\nSTOP\nEND\n",
                      &m) == 0);
    CHECK(words_are(&m, "10 4 14") && relative_is(&m, "0 0 0"));
    object_code_free(&m);
}

static void test_module_errors(void)
{
    printf("modo ligador: erros do modulo\n");
    ErrorList e;
    e = module_errors("SECTION TEXT\nM: BEGIN\nPUBLIC NADA\nSTOP\nEND\n");
    CHECK(single_error(&e, ERR_SEMANTICO, 3)); /* PUBLIC nao definido */
    e = module_errors("SECTION TEXT\nM: BEGIN\nX: EXTERN\nPUBLIC X\nEND\n");
    CHECK(single_error(&e, ERR_SEMANTICO, 4)); /* EXTERN e PUBLIC */
    e = module_errors("SECTION TEXT\nM: BEGIN\nX: EXTERN\nX: STOP\nEND\n");
    CHECK(single_error(&e, ERR_SEMANTICO, 4)); /* externo e rotulo local */
    e = module_errors("SECTION TEXT\nSTOP\n");
    CHECK(single_error(&e, ERR_SEMANTICO, 0)); /* sem BEGIN */
    /* SECTION é obrigatória também em módulo (versão antiga do mod_a). */
    e = module_errors("MOD_A: BEGIN\nY: EXTERN\nINPUT Y\nEND\n");
    CHECK(e.count >= 1);
    errors_free(&e);
}

static void test_roundtrip(void)
{
    printf("objeto intermediario: grava e le de volta\n");
    ObjectCode m, back;
    CHECK(module_from_file("tests/com_ligacao/mod_a.asm", &m) == 0);
    const char *path = "build/test_roundtrip.obj";
    CHECK(write_module_file(path, &m) == 0);
    CHECK(read_module_file(path, &back) == 0);
    CHECK(back.count == m.count &&
          memcmp(back.words, m.words, m.count * sizeof(int)) == 0 &&
          memcmp(back.relative, m.relative, m.count) == 0);
    CHECK(back.n_defs == m.n_defs && back.n_uses == m.n_uses);
    for (size_t i = 0; i < m.n_uses; i++)
        CHECK(has_use(&back, m.uses[i].name, m.uses[i].position));
    for (size_t i = 0; i < m.n_defs; i++)
        CHECK(has_def(&back, m.defs[i].name, m.defs[i].value));
    object_code_free(&back);
    object_code_free(&m);

    FILE *f = fopen(path, "w");
    fputs("R, 0 1\n12 0 10\n", f); /* R e código de tamanhos diferentes */
    fclose(f);
    CHECK(read_module_file(path, &back) == -2);
    CHECK(read_module_file("build/nao_existe.obj", &back) == -1);
    remove(path);
}

/* Liga os módulos dados por fonte; devolve a quantidade de erros. */
static size_t link_sources(const char **sources, int n, ObjectCode *out,
                           ErrorList *errors)
{
    ObjectCode mods[LINKER_MAX_MODULES];
    const char *names[] = {"m0", "m1", "m2"};
    for (int i = 0; i < n; i++)
        CHECK(module_from(sources[i], &mods[i]) == 0);
    link_modules(mods, names, n, out, errors);
    for (int i = 0; i < n; i++)
        object_code_free(&mods[i]);
    return errors->count;
}

static void test_link(void)
{
    printf("ligador: mod_a + mod_b == mod_a_mod_b.lig.esperado\n");
    ObjectCode a, b, out;
    ErrorList errors;
    errors_init(&errors);
    CHECK(module_from_file("tests/com_ligacao/mod_a.asm", &a) == 0);
    CHECK(module_from_file("tests/com_ligacao/mod_b.asm", &b) == 0);
    const char *names[] = {"mod_a.asm", "mod_b.asm"};

    ObjectCode ab[] = {a, b};
    link_modules(ab, names, 2, &out, &errors);
    CHECK(errors.count == 0);
    CHECK(words_are(&out, "12 20 10 11 1 20 11 22 7 12 14 5 13 20 13 11 13 "
                          "22 5 10 0 0 0"));
    object_code_free(&out);

    printf("ligador: ordem invertida (mod_b + mod_a)\n");
    ObjectCode ba[] = {b, a};
    const char *names_ba[] = {"mod_b.asm", "mod_a.asm"};
    link_modules(ba, names_ba, 2, &out, &errors);
    CHECK(errors.count == 0);
    /* mod_b: fator 0, 11 palavras; mod_a: fator 11. TGD: Y=8, MOD_B=0,
     * VAL=11+11=22, L1=10+11=21. */
    CHECK(words_are(&out, "13 8 13 22 13 10 5 21 0 0 0 12 8 10 22 1 8 11 10 "
                          "7 0 14 5"));
    object_code_free(&out);
    object_code_free(&a);
    object_code_free(&b);
    errors_free(&errors);

    printf("ligador: tres modulos\n");
    const char *three[] = {
        "SECTION TEXT\nA: BEGIN\nX: EXTERN\nZ: EXTERN\nJMP Z\nOUTPUT X\n"
        "END\n",
        "SECTION TEXT\nB: BEGIN\nPUBLIC X\nSTOP\nEND\nSECTION DATA\n"
        "X: CONST 9\n",
        "SECTION TEXT\nC: BEGIN\nPUBLIC Z\nZ: JMP Z\nEND\n",
    };
    errors_init(&errors);
    CHECK(link_sources(three, 3, &out, &errors) == 0);
    /* A: 0-3 (fator 0); B: 4-5 (fator 4, X=1+4=5); C: 6-7 (fator 6, Z=6). */
    CHECK(words_are(&out, "5 6 13 5 14 9 5 6"));
    object_code_free(&out);
    errors_free(&errors);
}

static void test_link_errors(void)
{
    printf("ligador: EXTERN nunca resolvido (um erro por simbolo)\n");
    const char *unresolved[] = {
        "SECTION TEXT\nA: BEGIN\nY: EXTERN\nLOAD Y\nSTORE Y + 1\nSTOP\nEND\n",
        "SECTION TEXT\nB: BEGIN\nSTOP\nEND\n",
    };
    ObjectCode out;
    ErrorList errors;
    errors_init(&errors);
    CHECK(link_sources(unresolved, 2, &out, &errors) == 1);
    CHECK(errors.count == 1 && strstr(errors.items[0].message, "'Y'") &&
          errors.items[0].type == ERR_SEMANTICO);
    object_code_free(&out);
    errors_free(&errors);

    printf("ligador: PUBLIC definido em mais de um modulo\n");
    const char *duplicate[] = {
        "SECTION TEXT\nA: BEGIN\nPUBLIC X\nX: STOP\nEND\n",
        "SECTION TEXT\nB: BEGIN\nPUBLIC X\nX: STOP\nEND\n",
    };
    errors_init(&errors);
    CHECK(link_sources(duplicate, 2, &out, &errors) == 1);
    CHECK(errors.count == 1 && strstr(errors.items[0].message, "'X'"));
    object_code_free(&out);
    errors_free(&errors);

    printf("ligador: endereco fora da memoria\n");
    const char *far[] = {
        "SECTION TEXT\nA: BEGIN\nLOAD X + 70000\nSTOP\nEND\n"
        "SECTION DATA\nX: SPACE\n",
        "SECTION TEXT\nB: BEGIN\nSTOP\nEND\n",
    };
    errors_init(&errors);
    CHECK(link_sources(far, 2, &out, &errors) == 1);
    object_code_free(&out);
    errors_free(&errors);
}

int main(void)
{
    test_module_mod1();
    test_module_mod_a();
    test_module_errors();
    test_roundtrip();
    test_link();
    test_link_errors();
    printf("\n%d verificacoes, %d falharam\n", checks, failures);
    return failures != 0;
}
