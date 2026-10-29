/* Ponto de entrada: ./montador arquivo.asm -> arquivo.obj */
#include <stdio.h>

#include "assembler.h"

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "uso: %s arquivo.asm\n", argv[0]);
        return 1;
    }
    fprintf(stderr, "montador: ainda nao implementado\n");
    return 1;
}
