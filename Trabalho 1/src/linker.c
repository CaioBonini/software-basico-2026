#include "linker.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Garante espaço para mais um item no vetor. */
static void *grow(void *items, size_t *capacity, size_t count, size_t size)
{
    if (count < *capacity)
        return items;
    *capacity = *capacity ? *capacity * 2 : 64;
    items = realloc(items, *capacity * size);
    if (items == NULL) {
        fprintf(stderr, "erro: memoria insuficiente\n");
        exit(1);
    }
    return items;
}

/* Lê os inteiros de s, em ordem, para *items. */
static size_t parse_ints(const char *s, int **items)
{
    size_t count = 0, capacity = 0;
    char *end;
    for (long v = strtol(s, &end, 10); end != s; v = strtol(s, &end, 10)) {
        *items = grow(*items, &capacity, count, sizeof **items);
        (*items)[count++] = (int)v;
        s = end;
    }
    return count;
}

int read_module_file(const char *path, ObjectCode *out)
{
    memset(out, 0, sizeof *out);
    char *text = read_source_file(path);
    if (text == NULL)
        return -1;

    size_t defs_capacity = 0, uses_capacity = 0, n_relative = 0;
    int *relative = NULL;
    int ok = 1;
    for (char *line = strtok(text, "\n"); line != NULL;
         line = strtok(NULL, "\n")) {
        char name[SYMBOL_MAX_LEN + 1];
        int value;
        if (strncmp(line, "D,", 2) == 0) {
            if (!(ok = sscanf(line + 2, "%30s %d", name, &value) == 2))
                break;
            out->defs = grow(out->defs, &defs_capacity, out->n_defs,
                             sizeof *out->defs);
            strcpy(out->defs[out->n_defs].name, name);
            out->defs[out->n_defs++].value = value;
        } else if (strncmp(line, "U,", 2) == 0) {
            if (!(ok = sscanf(line + 2, "%30s %d", name, &value) == 2))
                break;
            out->uses = grow(out->uses, &uses_capacity, out->n_uses,
                             sizeof *out->uses);
            strcpy(out->uses[out->n_uses].name, name);
            out->uses[out->n_uses++].position = value;
        } else if (strncmp(line, "R,", 2) == 0) {
            n_relative = parse_ints(line + 2, &relative);
        } else {
            out->count = parse_ints(line, &out->words);
        }
    }
    free(text);

    ok = ok && out->count > 0 && n_relative == out->count;
    for (size_t i = 0; ok && i < out->n_uses; i++)
        ok = out->uses[i].position >= 0 &&
             (size_t)out->uses[i].position < out->count;
    if (ok) {
        out->relative = malloc(out->count);
        if (out->relative == NULL) {
            fprintf(stderr, "erro: memoria insuficiente\n");
            exit(1);
        }
        for (size_t i = 0; i < out->count; i++)
            out->relative[i] = relative[i] != 0;
    }
    free(relative);
    if (!ok) {
        object_code_free(out);
        return -2;
    }
    return 0;
}

/* Entrada da Tabela Global de Definições. */
typedef struct {
    const char *name;
    int value;
    int module;
} Global;

static const Global *find_global(const Global *tgd, size_t n, const char *name)
{
    for (size_t i = 0; i < n; i++)
        if (strcmp(tgd[i].name, name) == 0)
            return &tgd[i];
    return NULL;
}

void link_modules(const ObjectCode *modules, const char *const *names, int n,
                  ObjectCode *out, ErrorList *errors)
{
    memset(out, 0, sizeof *out);

    /* 1 e 2: tamanho total e fator de correção de cada módulo. */
    int factor[LINKER_MAX_MODULES];
    size_t total = 0;
    for (int i = 0; i < n; i++) {
        factor[i] = (int)total;
        total += modules[i].count;
    }

    /* 3: TGD. */
    size_t n_globals = 0, globals_capacity = 0;
    Global *tgd = NULL;
    for (int i = 0; i < n; i++) {
        for (size_t d = 0; d < modules[i].n_defs; d++) {
            const Definition *def = &modules[i].defs[d];
            const Global *prev = find_global(tgd, n_globals, def->name);
            if (prev != NULL) {
                report_error(errors, 0, ERR_SEMANTICO,
                             "simbolo '%s' definido em mais de um modulo "
                             "(%s e %s)",
                             def->name, names[prev->module], names[i]);
                continue;
            }
            tgd = grow(tgd, &globals_capacity, n_globals, sizeof *tgd);
            tgd[n_globals++] = (Global){def->name, def->value + factor[i], i};
        }
    }

    out->count = total;
    out->words = malloc((total ? total : 1) * sizeof *out->words);
    unsigned char *in_use_table = calloc(total ? total : 1, 1);
    if (out->words == NULL || in_use_table == NULL) {
        fprintf(stderr, "erro: memoria insuficiente\n");
        exit(1);
    }
    for (int i = 0; i < n; i++)
        memcpy(out->words + factor[i], modules[i].words,
               modules[i].count * sizeof *out->words);

    for (int i = 0; i < n; i++) {
        const ObjectCode *mod = &modules[i];
        /* 4.1: posições da TU recebem TGD[s] + o que já estava gravado. */
        for (size_t u = 0; u < mod->n_uses; u++) {
            const Use *use = &mod->uses[u];
            size_t pos = (size_t)factor[i] + (size_t)use->position;
            in_use_table[pos] = 1;
            const Global *g = find_global(tgd, n_globals, use->name);
            if (g != NULL) {
                out->words[pos] += g->value;
                continue;
            }
            /* Uma mensagem por símbolo e módulo, não por uso. */
            size_t first = 0;
            while (strcmp(mod->uses[first].name, use->name) != 0)
                first++;
            if (first == u)
                report_error(errors, 0, ERR_SEMANTICO,
                             "simbolo externo '%s' (usado em %s) nao e "
                             "PUBLIC em nenhum modulo",
                             use->name, names[i]);
        }
        /* 4.2: os demais operandos de endereço recebem o fator do módulo. */
        for (size_t p = 0; p < mod->count; p++) {
            size_t pos = (size_t)factor[i] + p;
            if (!mod->relative[p])
                continue;
            if (!in_use_table[pos])
                out->words[pos] += factor[i];
            if (out->words[pos] < 0 || out->words[pos] >= MEMORY_SIZE)
                report_error(errors, 0, ERR_SEMANTICO,
                             "endereco %d fora da memoria (posicao %zu de %s)",
                             out->words[pos], p, names[i]);
        }
    }
    free(in_use_table);
    free(tgd);
}
