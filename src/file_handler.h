#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <openssl/md5.h>

// fonctions de l'ancienne version du sujet

char *list_files(const char *path);
char *read_file(const char *filepath, size_t *size);
void write_file(const char *filepath, const void *data, size_t size);

// types du nouveau sujet

// Structure pour une ligne du fichier log
typedef struct log_element{
    const char *path; // Chemin du fichier/dossier
    unsigned char md5[MD5_DIGEST_LENGTH]; // MD5 du fichier dédupliqué
    char *date; // Date de dernière modification
    struct log_element *next;
    struct log_element *prev;
} log_element;

// Structure pour une liste de log représentant le contenu du fichier backup_log
typedef struct {
    log_element *head; // Début de la liste de log 
    log_element *tail; // Fin de la liste de log
} log_t;

// fonctions de la nouvelle version du sujet

log_t read_backup_log(const char *logfile);
void update_backup_log(const char *logfile, log_t *logs);
void write_log_element(log_element *elt, FILE *logfile);
void list_files(const char *path);
void copy_file(const char *src, const char *dest);

#endif // FILE_HANDLER_H
