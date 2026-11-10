/* Ponto de entrada: ./ligador a.asm b.asm [c.asm]
 *
 * Monta cada módulo no modo ligador, grava o objeto intermediário de cada
 * um (a.obj, b.obj, ...), lê esses objetos de volta e liga, gerando o
 * executável com o nome do primeiro módulo e extensão LINKED_EXTENSION.
 * Se algum .asm não abre, nada é gravado. */
#include <stdio.h>
#include <stdlib.h>

#include "assembler.h"
#include "errors.h"
#include "linker.h"
#include "object_writer.h"

/* Monta source (lido de path) e grava seu objeto intermediário. Devolve 0
 * se não houve erro. */
static int assemble_to_file(const char *path, const char *source)
{
    ErrorList errors;
    errors_init(&errors);
    ObjectCode module;
    assemble_module(source, &module, &errors);

    char prefix[256];
    snprintf(prefix, sizeof prefix, "%s: ", path);
    errors_print_prefixed(&errors, prefix, stdout);

    int failed = errors.count > 0;
    if (failed)
        module.count = 0; /* com erro, o objeto sai vazio */
    char *obj_path = object_path_for(path);
    if (obj_path == NULL || write_module_file(obj_path, &module) != 0) {
        fprintf(stderr, "ligador: nao foi possivel escrever o objeto de '%s'\n",
                path);
        failed = 1;
    }
    free(obj_path);
    object_code_free(&module);
    errors_free(&errors);
    return failed;
}

int main(int argc, char **argv)
{
    int n = argc - 1;
    if (n > LINKER_MAX_MODULES) {
        fprintf(stderr, "ligador: no maximo %d modulos (recebeu %d)\n",
                LINKER_MAX_MODULES, n);
        return 1;
    }
    if (n < 2) {
        fprintf(stderr, "uso: %s prog1.asm prog2.asm [prog3.asm]\n", argv[0]);
        return 1;
    }
    const char *const *names = (const char *const *)&argv[1];

    /* Lê todas as entradas antes de gravar qualquer coisa: se alguma não
     * abre, sai sem criar arquivo nenhum (como o montador). */
    char *sources[LINKER_MAX_MODULES] = {0};
    int unreadable = 0;
    for (int i = 0; i < n; i++) {
        sources[i] = read_source_file(names[i]);
        if (sources[i] == NULL) {
            fprintf(stderr, "ligador: nao foi possivel ler '%s'\n", names[i]);
            unreadable = 1;
        }
    }
    if (unreadable) {
        for (int i = 0; i < n; i++)
            free(sources[i]);
        return 1;
    }

    int failed = 0;
    for (int i = 0; i < n; i++) {
        failed |= assemble_to_file(names[i], sources[i]);
        free(sources[i]);
    }

    ObjectCode modules[LINKER_MAX_MODULES] = {0};
    for (int i = 0; i < n && !failed; i++) {
        char *obj_path = object_path_for(names[i]);
        if (obj_path == NULL || read_module_file(obj_path, &modules[i]) != 0) {
            fprintf(stderr, "ligador: objeto de '%s' invalido\n", names[i]);
            failed = 1;
        }
        free(obj_path);
    }

    ObjectCode linked = {0};
    if (!failed) {
        ErrorList errors;
        errors_init(&errors);
        link_modules(modules, names, n, &linked, &errors);
        errors_print(&errors, stdout);
        failed = errors.count > 0;
        errors_free(&errors);
    }

    /* Com erro, o executável sai vazio, como o .obj do montador. */
    char *out_path = path_with_extension(names[0], LINKED_EXTENSION);
    if (out_path == NULL || write_object_file(out_path, linked.words,
                                              failed ? 0 : linked.count) != 0) {
        fprintf(stderr, "ligador: nao foi possivel escrever o executavel\n");
        failed = 1;
    }

    free(out_path);
    object_code_free(&linked);
    for (int i = 0; i < n; i++)
        object_code_free(&modules[i]);
    return failed;
}
