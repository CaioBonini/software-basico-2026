#include "opcode_table.h"

#include <stddef.h>
#include <string.h>

static const Instruction instructions[] = {
    {"ADD",    OP_ADD,    1, 2},
    {"SUB",    OP_SUB,    1, 2},
    {"MULT",   OP_MULT,   1, 2},
    {"MUL",    OP_MULT,   1, 2}, /* sinônimo defensivo (slide da aula) */
    {"DIV",    OP_DIV,    1, 2},
    {"JMP",    OP_JMP,    1, 2},
    {"JMPN",   OP_JMPN,   1, 2},
    {"JMPP",   OP_JMPP,   1, 2},
    {"JMPZ",   OP_JMPZ,   1, 2},
    {"COPY",   OP_COPY,   2, 3},
    {"LOAD",   OP_LOAD,   1, 2},
    {"STORE",  OP_STORE,  1, 2},
    {"INPUT",  OP_INPUT,  1, 2},
    {"OUTPUT", OP_OUTPUT, 1, 2},
    {"STOP",   OP_STOP,   0, 1},
};

const Instruction *opcode_lookup(const char *mnemonic)
{
    size_t n = sizeof instructions / sizeof instructions[0];
    for (size_t i = 0; i < n; i++)
        if (strcmp(instructions[i].mnemonic, mnemonic) == 0)
            return &instructions[i];
    return NULL;
}
