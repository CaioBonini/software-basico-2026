#include "assembler.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "opcode_table.h"
#include "symbol_table.h"

typedef enum {
    SECTION_NONE,
    SECTION_TEXT,
    SECTION_DATA
} Section;

typedef struct {
    int *items;
    size_t count;
    size_t capacity;
} IntVec;

/* Constante de montagem (EQU): nunca tem endereço, só valor. */
typedef struct {
    char name[IDENT_MAX_LEN + 1];
    int value;
} Equ;

/* Rótulo da seção de dados, definido na tabela só no fim da passagem. */
typedef struct {
    char name[IDENT_MAX_LEN + 1];
    int offset; /* posição dentro da seção de dados */
} DataLabel;

/* PUBLIC pendente: o valor só é conhecido no fim da passagem. */
typedef struct {
    char name[IDENT_MAX_LEN + 1];
    int line;
} Public;

typedef struct {
    bool linker_mode;
    IntVec text;          /* código da seção de texto */
    IntVec text_offset;   /* pending_offset da symbol_table, paralelo a text */
    IntVec text_line;     /* linha original de cada palavra de text */
    IntVec text_relative; /* 1 se a palavra é operando de endereço */
    IntVec data;          /* valores da seção de dados */
    Use *uses; /* TU */
    size_t n_uses, uses_capacity;
    Public *publics;
    size_t n_publics, publics_capacity;
    SymbolTable symbols;
    Equ *equs;
    size_t n_equs, equs_capacity;
    DataLabel *data_labels;
    size_t n_data_labels, data_labels_capacity;
    Section section;
    bool seen_text;
    int begin_line; /* 0 se não houve BEGIN */
    bool seen_end;
    ErrorList *errors;
} Assembler;

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

static void vec_push(IntVec *v, int value)
{
    v->items = grow(v->items, &v->capacity, v->count, sizeof *v->items);
    v->items[v->count++] = value;
}

/* Acrescenta uma palavra ao texto e devolve sua posição. */
static int emit_text(Assembler *as, int value, int line)
{
    vec_push(&as->text, value);
    vec_push(&as->text_offset, 0);
    vec_push(&as->text_line, line);
    vec_push(&as->text_relative, 0);
    return (int)as->text.count - 1;
}

static void add_use(Assembler *as, const char *name, int pos)
{
    as->uses = grow(as->uses, &as->uses_capacity, as->n_uses, sizeof *as->uses);
    strcpy(as->uses[as->n_uses].name, name);
    as->uses[as->n_uses].position = pos;
    as->n_uses++;
}

static const Equ *find_equ(const Assembler *as, const char *name)
{
    for (size_t i = 0; i < as->n_equs; i++)
        if (strcmp(as->equs[i].name, name) == 0)
            return &as->equs[i];
    return NULL;
}

static bool find_data_label(const Assembler *as, const char *name)
{
    for (size_t i = 0; i < as->n_data_labels; i++)
        if (strcmp(as->data_labels[i].name, name) == 0)
            return true;
    return false;
}

/* Já existe definição (rótulo de texto, de dados ou EQU) com esse nome? */
static bool is_declared(Assembler *as, const char *name)
{
    const Symbol *sym = symtab_find(&as->symbols, name);
    return (sym != NULL && sym->defined) || find_data_label(as, name) ||
           find_equ(as, name) != NULL;
}

static void define_label(Assembler *as, const char *name, int line)
{
    if (is_declared(as, name)) {
        report_error(as->errors, line, ERR_SEMANTICO,
                     "rotulo '%s' declarado mais de uma vez", name);
        return;
    }
    if (as->section == SECTION_TEXT) {
        symtab_define(&as->symbols, name, (int)as->text.count,
                      as->text.items, as->text_offset.items);
    } else {
        as->data_labels = grow(as->data_labels, &as->data_labels_capacity,
                               as->n_data_labels, sizeof *as->data_labels);
        DataLabel *dl = &as->data_labels[as->n_data_labels++];
        strcpy(dl->name, name);
        dl->offset = (int)as->data.count;
    }
}

/* Valor de um operando numérico: número literal ou símbolo EQU (+ n). */
static bool numeric_value(const Assembler *as, const Operand *op, int *value)
{
    if (op->kind == OPERAND_NUMBER) {
        *value = op->value;
        return true;
    }
    if (op->kind == OPERAND_SYMBOL) {
        const Equ *equ = find_equ(as, op->name);
        if (equ != NULL) {
            *value = equ->value + op->value;
            return true;
        }
    }
    return false;
}

static bool check_count(Assembler *as, const Line *l, int min, int max)
{
    if (l->n_operands >= min && l->n_operands <= max)
        return true;
    if (l->has_error)
        return false; /* contagem não confiável: o lexer já acusou a linha */
    if (min == max)
        report_error(as->errors, l->line, ERR_SINTATICO,
                     "'%s' espera %d operando(s), recebeu %d", l->op, min,
                     l->n_operands);
    else
        report_error(as->errors, l->line, ERR_SINTATICO,
                     "'%s' espera de %d a %d operandos, recebeu %d", l->op,
                     min, max, l->n_operands);
    return false;
}

/* Operando de instrução: endereço de um rótulo (+ n). */
static void emit_address(Assembler *as, const Operand *op, int line)
{
    int pos = emit_text(as, 0, line);
    if (op->kind == OPERAND_INVALID)
        return; /* erro léxico já reportado */
    if (op->kind != OPERAND_SYMBOL) {
        report_error(as->errors, line, ERR_SINTATICO,
                     "operando de instrucao deve ser um rotulo, nao o "
                     "numero %d",
                     op->value);
        return;
    }
    const Equ *equ = find_equ(as, op->name);
    if (equ != NULL) {
        /* Valor absoluto (não relocável), mas a instrução o usa como
         * endereço: já dá para checar a faixa aqui. */
        as->text.items[pos] = equ->value + op->value;
        if (!address_in_memory(as->text.items[pos]))
            report_error(as->errors, line, ERR_SEMANTICO,
                         "endereco %d fora da memoria (0 a %d)",
                         as->text.items[pos], MEMORY_SIZE - 1);
        return;
    }
    as->text_relative.items[pos] = 1;
    const Symbol *sym = symtab_find(&as->symbols, op->name);
    if (sym != NULL && sym->external) {
        as->text.items[pos] = op->value; /* 0 + deslocamento */
        add_use(as, op->name, pos);
        return;
    }
    symtab_reference(&as->symbols, op->name, op->value, pos, as->text.items,
                     as->text_offset.items);
}

/* EXTERN no modo ligador: o símbolo entra na TS com valor 0, marcado como
 * externo. Usos anteriores (ainda na cadeia de pendências) vão para a TU. */
static void declare_extern(Assembler *as, const char *name, int line)
{
    if (is_declared(as, name)) {
        report_error(as->errors, line, ERR_SEMANTICO,
                     "rotulo '%s' declarado mais de uma vez", name);
        return;
    }
    const Symbol *sym = symtab_find(&as->symbols, name);
    for (int pos = sym ? sym->list : LIST_END; pos != LIST_END;
         pos = as->text.items[pos])
        add_use(as, name, pos);
    symtab_define(&as->symbols, name, 0, as->text.items,
                  as->text_offset.items);
    symtab_find(&as->symbols, name)->external = true;
}

static void declare_public(Assembler *as, const Line *l)
{
    const Operand *op = &l->operands[0];
    if (op->kind != OPERAND_SYMBOL || op->value != 0) {
        report_error(as->errors, l->line, ERR_SINTATICO,
                     "PUBLIC espera o nome de um rotulo");
        return;
    }
    for (size_t i = 0; i < as->n_publics; i++)
        if (strcmp(as->publics[i].name, op->name) == 0) {
            report_error(as->errors, l->line, ERR_SEMANTICO,
                         "'%s' declarado PUBLIC mais de uma vez", op->name);
            return;
        }
    as->publics = grow(as->publics, &as->publics_capacity, as->n_publics,
                       sizeof *as->publics);
    strcpy(as->publics[as->n_publics].name, op->name);
    as->publics[as->n_publics].line = l->line;
    as->n_publics++;
}

static void do_instruction(Assembler *as, const Line *l, const Instruction *ins)
{
    if (as->section != SECTION_TEXT) {
        report_error(as->errors, l->line, ERR_SEMANTICO,
                     "instrucao '%s' fora da secao TEXT", l->op);
        return;
    }
    bool count_ok = check_count(as, l, ins->n_operands, ins->n_operands);
    emit_text(as, ins->opcode, l->line);
    /* Com a quantidade errada, ainda reserva o tamanho da instrução para
     * não deslocar os endereços seguintes. */
    for (int i = 0; i < ins->n_operands; i++) {
        if (count_ok)
            emit_address(as, &l->operands[i], l->line);
        else
            emit_text(as, 0, l->line);
    }
}

static void do_const(Assembler *as, const Line *l)
{
    if (as->section != SECTION_DATA) {
        report_error(as->errors, l->line, ERR_SEMANTICO,
                     "CONST fora da secao DATA");
        return;
    }
    int value = 0;
    if (check_count(as, l, 1, 1) && l->operands[0].kind != OPERAND_INVALID &&
        !numeric_value(as, &l->operands[0], &value))
        report_error(as->errors, l->line, ERR_SINTATICO,
                     "CONST espera um numero decimal");
    vec_push(&as->data, value);
}

static void do_space(Assembler *as, const Line *l)
{
    if (as->section != SECTION_DATA) {
        report_error(as->errors, l->line, ERR_SEMANTICO,
                     "SPACE fora da secao DATA");
        return;
    }
    int n = 1;
    if (check_count(as, l, 0, 1) && l->n_operands == 1 &&
        l->operands[0].kind != OPERAND_INVALID) {
        if (!numeric_value(as, &l->operands[0], &n) || n < 1) {
            report_error(as->errors, l->line, ERR_SINTATICO,
                         "SPACE espera um numero positivo de palavras");
            n = 1;
        }
    }
    for (int i = 0; i < n; i++)
        vec_push(&as->data, 0);
}

static void do_section(Assembler *as, const Line *l)
{
    if (l->has_label)
        report_error(as->errors, l->label_line, ERR_SINTATICO,
                     "rotulo '%s' sem instrucao", l->label);
    const Operand *op = &l->operands[0];
    bool simple = l->n_operands == 1 && op->kind == OPERAND_SYMBOL &&
                  op->value == 0;
    if (simple && strcmp(op->name, "TEXT") == 0) {
        as->section = SECTION_TEXT;
        as->seen_text = true;
    } else if (simple && strcmp(op->name, "DATA") == 0) {
        as->section = SECTION_DATA;
    } else {
        report_error(as->errors, l->line, ERR_SINTATICO,
                     "secao invalida (esperava SECTION TEXT ou SECTION DATA)");
    }
}

static void do_equ(Assembler *as, const Line *l)
{
    if (as->section != SECTION_NONE)
        report_error(as->errors, l->line, ERR_SEMANTICO,
                     "EQU deve vir antes de qualquer secao");
    if (!l->has_label) {
        report_error(as->errors, l->line, ERR_SINTATICO, "EQU sem rotulo");
        return;
    }
    int value = 0;
    if (check_count(as, l, 1, 1) && l->operands[0].kind != OPERAND_INVALID &&
        !numeric_value(as, &l->operands[0], &value))
        report_error(as->errors, l->line, ERR_SINTATICO,
                     "EQU espera um numero decimal ou outro simbolo EQU");
    if (is_declared(as, l->label)) {
        report_error(as->errors, l->label_line, ERR_SEMANTICO,
                     "rotulo '%s' declarado mais de uma vez", l->label);
        return;
    }
    as->equs = grow(as->equs, &as->equs_capacity, as->n_equs,
                    sizeof *as->equs);
    strcpy(as->equs[as->n_equs].name, l->label);
    as->equs[as->n_equs].value = value;
    as->n_equs++;
}

/* Devolve true se a próxima linha lógica deve ser pulada. */
static bool do_if(Assembler *as, const Line *l)
{
    if (as->section != SECTION_TEXT) {
        report_error(as->errors, l->line, ERR_SEMANTICO,
                     "IF fora da secao TEXT");
        return false;
    }
    if (!check_count(as, l, 1, 1) || l->operands[0].kind == OPERAND_INVALID)
        return false;
    int value;
    if (!numeric_value(as, &l->operands[0], &value)) {
        report_error(as->errors, l->line, ERR_SEMANTICO,
                     "IF espera um simbolo definido por EQU ('%s' nao e)",
                     l->operands[0].name);
        return false;
    }
    return value == 0;
}

static void do_line(Assembler *as, const Line *l, bool *skip_next)
{
    if (!l->has_op) {
        report_error(as->errors, l->label_line, ERR_SINTATICO,
                     "rotulo '%s' sem instrucao", l->label);
        return;
    }
    if (strcmp(l->op, "SECTION") == 0) {
        do_section(as, l);
        return;
    }
    if (strcmp(l->op, "EQU") == 0) {
        do_equ(as, l);
        return;
    }
    if (as->section == SECTION_NONE) {
        report_error(as->errors, l->line, ERR_SEMANTICO,
                     "'%s' fora de qualquer secao", l->op);
        return;
    }

    bool is_extern = strcmp(l->op, "EXTERN") == 0;
    if (l->has_label && !is_extern)
        define_label(as, l->label, l->label_line);

    const Instruction *ins = opcode_lookup(l->op);
    if (ins != NULL) {
        do_instruction(as, l, ins);
    } else if (strcmp(l->op, "CONST") == 0) {
        do_const(as, l);
    } else if (strcmp(l->op, "SPACE") == 0) {
        do_space(as, l);
    } else if (strcmp(l->op, "IF") == 0) {
        *skip_next = do_if(as, l);
    } else if (strcmp(l->op, "BEGIN") == 0) {
        /* Ligação completa fica para a etapa 7. */
        check_count(as, l, 0, 0);
        as->begin_line = l->line;
    } else if (strcmp(l->op, "END") == 0) {
        check_count(as, l, 0, 0);
        if (as->begin_line == 0)
            report_error(as->errors, l->line, ERR_SEMANTICO, "END sem BEGIN");
        as->seen_end = true;
    } else if (strcmp(l->op, "PUBLIC") == 0) {
        if (check_count(as, l, 1, 1) && as->linker_mode)
            declare_public(as, l);
    } else if (is_extern) {
        check_count(as, l, 0, 0);
        if (!l->has_label)
            report_error(as->errors, l->line, ERR_SINTATICO,
                         "EXTERN sem rotulo");
        else if (as->linker_mode)
            declare_extern(as, l->label, l->label_line);
    } else {
        report_error(as->errors, l->line, ERR_SINTATICO,
                     "instrucao ou diretiva invalida '%s'", l->op);
    }
}

/* Fim da passagem: define os rótulos de dados (agora que o tamanho do texto
 * é conhecido) e acusa os rótulos usados e nunca declarados. */
static void finish(Assembler *as)
{
    int text_size = (int)as->text.count;
    for (size_t i = 0; i < as->n_data_labels; i++)
        symtab_define(&as->symbols, as->data_labels[i].name,
                      text_size + as->data_labels[i].offset, as->text.items,
                      as->text_offset.items);

    for (size_t i = 0; i < as->symbols.count; i++) {
        Symbol *sym = &as->symbols.items[i];
        if (sym->defined)
            continue;
        /* Uma mensagem por uso, percorrendo a cadeia de pendências. */
        for (int pos = sym->list; pos != LIST_END;) {
            int next = as->text.items[pos];
            report_error(as->errors, as->text_line.items[pos], ERR_SEMANTICO,
                         "rotulo '%s' nao declarado", sym->name);
            as->text.items[pos] = 0;
            pos = next;
        }
        sym->list = LIST_END;
    }

    /* Endereços de rótulo só ficam conhecidos agora. Operandos externos
     * guardam só o deslocamento: quem checa esses é o ligador. */
    unsigned char *external = calloc(as->text.count ? as->text.count : 1, 1);
    if (external == NULL) {
        fprintf(stderr, "erro: memoria insuficiente\n");
        exit(1);
    }
    for (size_t i = 0; i < as->n_uses; i++)
        external[as->uses[i].position] = 1;
    for (size_t pos = 0; pos < as->text.count; pos++)
        if (as->text_relative.items[pos] && !external[pos] &&
            !address_in_memory(as->text.items[pos]))
            report_error(as->errors, as->text_line.items[pos], ERR_SEMANTICO,
                         "endereco %d fora da memoria (0 a %d)",
                         as->text.items[pos], MEMORY_SIZE - 1);
    free(external);

    if (!as->seen_text)
        report_error(as->errors, 0, ERR_SEMANTICO, "secao TEXT ausente");
    if (as->begin_line != 0 && !as->seen_end)
        report_error(as->errors, as->begin_line, ERR_SEMANTICO,
                     "BEGIN sem END");
    if (as->linker_mode && as->begin_line == 0)
        report_error(as->errors, 0, ERR_SEMANTICO,
                     "modulo sem BEGIN (obrigatorio para o ligador)");
}

/* TD: cada PUBLIC recebe o valor que o símbolo tem na TS no fim da passagem. */
static void build_definitions(Assembler *as, ObjectCode *out)
{
    out->defs = malloc((as->n_publics ? as->n_publics : 1) * sizeof *out->defs);
    if (out->defs == NULL) {
        fprintf(stderr, "erro: memoria insuficiente\n");
        exit(1);
    }
    out->n_defs = 0;
    for (size_t i = 0; i < as->n_publics; i++) {
        const Public *pub = &as->publics[i];
        const Symbol *sym = symtab_find(&as->symbols, pub->name);
        if (sym == NULL || !sym->defined) {
            report_error(as->errors, pub->line, ERR_SEMANTICO,
                         "simbolo PUBLIC '%s' nao definido no modulo",
                         pub->name);
        } else if (sym->external) {
            report_error(as->errors, pub->line, ERR_SEMANTICO,
                         "simbolo '%s' nao pode ser EXTERN e PUBLIC",
                         pub->name);
        } else {
            strcpy(out->defs[out->n_defs].name, pub->name);
            out->defs[out->n_defs].value = sym->value;
            out->n_defs++;
        }
    }
}

static void assemble_mode(const char *source, bool linker_mode,
                          ObjectCode *out, ErrorList *errors)
{
    Assembler as = {0};
    as.linker_mode = linker_mode;
    as.errors = errors;
    symtab_init(&as.symbols);

    LineList lines;
    lexer_run(source, &lines, errors);

    bool skip_next = false;
    for (size_t i = 0; i < lines.count; i++) {
        if (skip_next) {
            skip_next = false; /* IF falso: a linha não existe */
            continue;
        }
        do_line(&as, &lines.items[i], &skip_next);
    }
    finish(&as);
    build_definitions(&as, out);

    out->count = as.text.count + as.data.count;
    size_t n = out->count ? out->count : 1;
    out->words = malloc(n * sizeof *out->words);
    out->relative = calloc(n, sizeof *out->relative);
    if (out->words == NULL || out->relative == NULL) {
        fprintf(stderr, "erro: memoria insuficiente\n");
        exit(1);
    }
    for (size_t i = 0; i < as.text.count; i++) {
        out->words[i] = as.text.items[i];
        out->relative[i] = (unsigned char)as.text_relative.items[i];
    }
    if (as.data.count > 0)
        memcpy(out->words + as.text.count, as.data.items,
               as.data.count * sizeof(int));
    out->uses = as.uses; /* posse passa para out */
    out->n_uses = as.n_uses;

    lines_free(&lines);
    symtab_free(&as.symbols);
    free(as.text.items);
    free(as.text_offset.items);
    free(as.text_line.items);
    free(as.text_relative.items);
    free(as.data.items);
    free(as.equs);
    free(as.data_labels);
    free(as.publics);
}

bool address_in_memory(int address)
{
    return address >= 0 && address < MEMORY_SIZE;
}

void assemble(const char *source, ObjectCode *out, ErrorList *errors)
{
    assemble_mode(source, false, out, errors);
}

void assemble_module(const char *source, ObjectCode *out, ErrorList *errors)
{
    assemble_mode(source, true, out, errors);
}

void object_code_free(ObjectCode *code)
{
    free(code->words);
    free(code->relative);
    free(code->defs);
    free(code->uses);
    code->words = NULL;
    code->relative = NULL;
    code->defs = NULL;
    code->uses = NULL;
    code->count = code->n_defs = code->n_uses = 0;
}

char *read_source_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL)
        return NULL;
    char *buf = NULL;
    if (fseek(f, 0, SEEK_END) == 0) {
        long size = ftell(f);
        if (size >= 0 && fseek(f, 0, SEEK_SET) == 0) {
            buf = malloc((size_t)size + 1);
            if (buf != NULL) {
                size_t got = fread(buf, 1, (size_t)size, f);
                buf[got] = '\0';
            }
        }
    }
    fclose(f);
    return buf;
}
