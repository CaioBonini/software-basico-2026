#include "object_writer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int write_object_file(const char *path, const int *words, size_t count)
{
    FILE *f = fopen(path, "w");
    if (f == NULL)
        return -1;
    for (size_t i = 0; i < count; i++)
        fprintf(f, i == 0 ? "%d" : " %d", words[i]);
    if (count > 0)
        fputc('\n', f);
    return fclose(f) == 0 ? 0 : -1;
}

int write_module_file(const char *path, const ObjectCode *module)
{
    FILE *f = fopen(path, "w");
    if (f == NULL)
        return -1;
    if (module->count > 0) {
        for (size_t i = 0; i < module->n_defs; i++)
            fprintf(f, "D, %s %d\n", module->defs[i].name,
                    module->defs[i].value);
        for (size_t i = 0; i < module->n_uses; i++)
            fprintf(f, "U, %s %d\n", module->uses[i].name,
                    module->uses[i].position);
        fputs("R,", f);
        for (size_t i = 0; i < module->count; i++)
            fprintf(f, " %d", module->relative[i]);
        fputc('\n', f);
        for (size_t i = 0; i < module->count; i++)
            fprintf(f, i == 0 ? "%d" : " %d", module->words[i]);
        fputc('\n', f);
    }
    return fclose(f) == 0 ? 0 : -1;
}

char *path_with_extension(const char *path, const char *ext)
{
    const char *slash = strrchr(path, '/');
    const char *dot = strrchr(path, '.');
    size_t base = (dot != NULL && (slash == NULL || dot > slash))
                      ? (size_t)(dot - path)
                      : strlen(path);
    char *out = malloc(base + strlen(ext) + 1);
    if (out == NULL)
        return NULL;
    memcpy(out, path, base);
    strcpy(out + base, ext);
    return out;
}

char *object_path_for(const char *source_path)
{
    return path_with_extension(source_path, OBJECT_EXTENSION);
}
