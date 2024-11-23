#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H


char *list_files(const char *path);
char *read_file(const char *filepath, size_t *size);
void write_file(const char *filepath, const void *data, size_t size);

#endif // FILE_HANDLER_H
