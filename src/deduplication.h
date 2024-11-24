#ifndef DEDUPLICATION_H
#define DEDUPLICATION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/md5.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <libgen.h>
#include <unistd.h>

#define HASH_SIZE MD5_DIGEST_LENGTH // Taille du hachage MD5
#define CHUNK_SIZE 4096            // Taille d'un bloc de donnée 4Ko
#define TAILLE_TABLE 1024
#define MAX_NOM_FICHIER 1024

typedef struct FileChunk {
    unsigned char hash[HASH_SIZE];
    char *filename;
    unsigned int id_chunk;
    unsigned int version;
    struct FileChunk *next; // Pointeur vers le prochain bloc de données
} FileChunk;

char *strdup(const char *s);
void check_pckg(char *nom_pckg, char *commande_installer_pckg);
unsigned int determine_index_hash(unsigned char *hash);
void insere_file_chunk(FileChunk **table, unsigned char *hash, const char *filename, unsigned int id_chunk, unsigned int version);
FileChunk *trouver_double(FileChunk **table, unsigned char *hash);
void hash_chunk(const unsigned char *chunk_data, size_t chunk_size, unsigned char hash[HASH_SIZE]);
unsigned int stocker_chunk_data(const unsigned char *chunk_data, size_t chunk_size, unsigned int version, const char *filename, const char *destination);
void traitement_fichier(const char *filename, const char *destination);
void deduplicate_files(const char *source, const char *destination);

#endif // DEDUPLICATION_H