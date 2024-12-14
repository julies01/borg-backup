#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <openssl/md5.h>

// Structure pour une ligne du fichier log
typedef struct log_element {
    char *path; // Chemin du fichier/dossier
    char *md5; // MD5 du fichier dédupliqué, le considérer comme une chaine de caracter est suffisant
    char *date; // Date de dernière modification
    struct log_element *next;
    struct log_element *prev;
} log_element;

// Structure pour une liste de log représentant le contenu du fichier backup_log
typedef struct {
    log_element *head; // Début de la liste de log 
    log_element *tail; // Fin de la liste de log
} log_t;

char **list_files(const char *path, int *count);
void copy_file(const char *src, const char *dest);
log_t read_backup_log(const char *logfile);
void update_backup_log(const char *logfile, log_t *logs);
void write_log_element(log_element *elt, FILE *logfile);

#endif // FILE_HANDLER_H
