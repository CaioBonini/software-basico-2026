/* Tabela de mnemônicos/diretivas (enum + struct). */
#ifndef OPCODE_TABLE_H
#define OPCODE_TABLE_H

typedef enum {
    OP_ADD = 1,
    OP_SUB = 2,
    OP_MULT = 3,
    OP_DIV = 4,
    OP_JMP = 5,
    OP_JMPN = 6,
    OP_JMPP = 7,
    OP_JMPZ = 8,
    OP_COPY = 9,
    OP_LOAD = 10,
    OP_STORE = 11,
    OP_INPUT = 12,
    OP_OUTPUT = 13,
    OP_STOP = 14
} Opcode;

typedef struct {
    const char *mnemonic;
    Opcode opcode;
    int n_operands;
    int n_words; /* opcode + operandos */
} Instruction;

/* Procura uma instrução pelo mnemônico, já em maiúsculas (o lexer normaliza
 * a caixa). Aceita MULT (oficial) e MUL (sinônimo). NULL se não existir. */
const Instruction *opcode_lookup(const char *mnemonic);

#endif
