#include "symbol_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void symtab_init(SymbolTable *table)
{
    table->items = NULL;
    table->count = 0;
    table->capacity = 0;
}

void symtab_free(SymbolTable *table)
{
    free(table->items);
    symtab_init(table);
}

Symbol *symtab_find(SymbolTable *table, const char *name)
{
    for (size_t i = 0; i < table->count; i++)
        if (strcmp(table->items[i].name, name) == 0)
            return &table->items[i];
    return NULL;
}

static Symbol *insert(SymbolTable *table, const char *name)
{
    if (table->count == table->capacity) {
        size_t capacity = table->capacity ? table->capacity * 2 : 16;
        Symbol *items = realloc(table->items, capacity * sizeof *items);
        if (items == NULL) {
            fprintf(stderr, "erro: memoria insuficiente\n");
            exit(1);
        }
        table->items = items;
        table->capacity = capacity;
    }
    Symbol *sym = &table->items[table->count++];
    snprintf(sym->name, sizeof sym->name, "%s", name);
    sym->value = 0;
    sym->defined = false;
    sym->list = LIST_END;
    sym->external = false;
    return sym;
}

SymbolResult symtab_define(SymbolTable *table, const char *name, int address,
                           int *code, int *pending_offset)
{
    Symbol *sym = symtab_find(table, name);
    if (sym == NULL)
        sym = insert(table, name);
    else if (sym->defined)
        return SYM_REDEFINED;

    sym->defined = true;
    sym->value = address;
    for (int pos = sym->list; pos != LIST_END;) {
        int next = code[pos];
        code[pos] = address + pending_offset[pos];
        pending_offset[pos] = 0;
        pos = next;
    }
    sym->list = LIST_END;
    return SYM_OK;
}

void symtab_reference(SymbolTable *table, const char *name, int offset,
                      int pos, int *code, int *pending_offset)
{
    Symbol *sym = symtab_find(table, name);
    if (sym == NULL)
        sym = insert(table, name);

    if (sym->defined) {
        code[pos] = sym->value + offset;
        return;
    }
    code[pos] = sym->list;
    pending_offset[pos] = offset;
    sym->list = pos;
}
