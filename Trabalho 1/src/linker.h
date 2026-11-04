/* Ligador: TD/TU por módulo, fatores de correção, TGD e executável final.
 *
 * Algoritmo dos slides da Aula 7:
 *   1. concatena os códigos na ordem dos argumentos;
 *   2. fator de correção do módulo i = soma dos tamanhos dos anteriores;
 *   3. TGD[s] = valor local de s + fator do módulo que o definiu (PUBLIC);
 *   4. em cada posição da TU, soma TGD[s] ao valor já gravado (o
 *      deslocamento da expressão); em todo outro operando de endereço do
 *      módulo, soma só o fator do próprio módulo. Opcodes e valores de
 *      CONST/SPACE não são tocados. */
#ifndef LINKER_H
#define LINKER_H

#include "assembler.h"
#include "errors.h"

#define LINKER_MAX_MODULES 3
#define LINKED_EXTENSION ".lig" /* extensão do executável (provisória) */

/* Lê um objeto intermediário gravado por write_module_file. Devolve 0 se
 * deu certo, -1 se não abriu e -2 se o formato está errado. */
int read_module_file(const char *path, ObjectCode *out);

/* Liga n módulos (names[i] só aparece nas mensagens de erro). Erros de
 * ligação vão para errors sem número de linha. Só out->words/out->count
 * são preenchidos. */
void link_modules(const ObjectCode *modules, const char *const *names, int n,
                  ObjectCode *out, ErrorList *errors);

#endif
