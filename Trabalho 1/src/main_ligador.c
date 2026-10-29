/* Ponto de entrada: ./ligador a.asm b.asm [c.asm] */
#include <stdio.h>

#include "linker.h"

int main(int argc, char **argv)
{
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "uso: %s prog1.asm prog2.asm [prog3.asm]\n", argv[0]);
        return 1;
    }
    fprintf(stderr, "ligador: ainda nao implementado\n");
    return 1;
}
