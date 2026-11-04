/* Escrita do .obj (uma linha, inteiros separados por espaço). */
#ifndef OBJECT_WRITER_H
#define OBJECT_WRITER_H

#include <stddef.h>

#define OBJECT_EXTENSION ".obj"

/* Escreve count palavras numa linha só. count == 0 gera um arquivo vazio
 * (usado quando a montagem teve erro). Devolve 0 se deu certo. */
int write_object_file(const char *path, const int *words, size_t count);

/* "dir/prog.asm" -> "dir/prog.obj" (troca a extensão; acrescenta se não
 * houver). O chamador libera a string. */
char *object_path_for(const char *source_path);

#endif
