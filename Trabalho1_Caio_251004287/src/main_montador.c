/* Ponto de entrada: ./montador arquivo.asm -> arquivo.obj */
#include <stdio.h>
#include <stdlib.h>

#include "assembler.h"
#include "errors.h"
#include "object_writer.h"

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "uso: %s arquivo.asm\n", argv[0]);
        return 1;
    }
    char *source = read_source_file(argv[1]);
    if (source == NULL) {
        fprintf(stderr, "montador: nao foi possivel ler '%s'\n", argv[1]);
        return 1;
    }

    ErrorList errors;
    errors_init(&errors);
    ObjectCode code;
    assemble(source, &code, &errors);
    errors_print(&errors, stdout);

    /* Decisão revisável: com erro, o .obj sai vazio (como no projeto de
     * referência), em vez de não ser gerado. */
    int failed = errors.count > 0;
    char *obj_path = object_path_for(argv[1]);
    if (obj_path == NULL ||
        write_object_file(obj_path, code.words, failed ? 0 : code.count) != 0) {
        fprintf(stderr, "montador: nao foi possivel escrever '%s'\n",
                obj_path ? obj_path : "(sem memoria)");
        failed = 1;
    }

    free(obj_path);
    object_code_free(&code);
    errors_free(&errors);
    free(source);
    return failed;
}
