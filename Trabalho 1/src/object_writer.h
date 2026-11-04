/* Escrita do .obj (uma linha, inteiros separados por espaço). */
#ifndef OBJECT_WRITER_H
#define OBJECT_WRITER_H

#include <stddef.h>

#include "assembler.h"

#define OBJECT_EXTENSION ".obj"

/* Escreve count palavras numa linha só. count == 0 gera um arquivo vazio
 * (usado quando a montagem teve erro). Devolve 0 se deu certo. */
int write_object_file(const char *path, const int *words, size_t count);

/* Objeto intermediário de um módulo (gerado pelo montador quando chamado
 * pelo ligador; formato livre pelo enunciado). Uma linha por registro:
 *   D, NOME VALOR    um por PUBLIC (TD)
 *   U, NOME POSICAO  um por uso de EXTERN (TU)
 *   R, b0 b1 ...     1 = palavra é operando de endereço
 *   w0 w1 ...        código (texto + dados)
 * count == 0 gera um arquivo vazio. Devolve 0 se deu certo. */
int write_module_file(const char *path, const ObjectCode *module);

/* Troca a extensão de path por ext (acrescenta se não houver):
 * ("dir/prog.asm", ".obj") -> "dir/prog.obj". O chamador libera a string. */
char *path_with_extension(const char *path, const char *ext);

/* path_with_extension(source_path, OBJECT_EXTENSION). */
char *object_path_for(const char *source_path);

#endif
