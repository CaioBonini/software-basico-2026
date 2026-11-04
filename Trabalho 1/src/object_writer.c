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

char *object_path_for(const char *source_path)
{
    const char *slash = strrchr(source_path, '/');
    const char *dot = strrchr(source_path, '.');
    size_t base = (dot != NULL && (slash == NULL || dot > slash))
                      ? (size_t)(dot - source_path)
                      : strlen(source_path);
    char *path = malloc(base + sizeof OBJECT_EXTENSION);
    if (path == NULL)
        return NULL;
    memcpy(path, source_path, base);
    strcpy(path + base, OBJECT_EXTENSION);
    return path;
}
