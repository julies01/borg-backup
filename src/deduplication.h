#ifndef DEDUPLICATION_H
#define DEDUPLICATION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <openssl/md5.h>
#include <dirent.h>

// Taille d'un chunk (4096 octets)
#define CHUNK_SIZE 4096

// Taille de la table de hachage qui contiendra les chunks
// dont on a déjà calculé le MD5 pour effectuer les comparaisons
#define HASH_TABLE_SIZE 1000

// Structure pour un chunk
typedef struct Chunk{
    unsigned char md5[MD5_DIGEST_LENGTH]; // MD5 du chunk
    int is_unique; // 1 si le chunk est unique, 0 sinon
    void *data; // Données du chunk
    struct Chunk *prev; // Pointeur vers le chunk précédent
    struct Chunk *next; // Pointeur vers le prochain chunk
} Chunk;

typedef struct Chunk *Chunk_list;

// Table de hachage pour stocker les MD5 et leurs index

typedef struct Md5Entry {
    unsigned char md5[MD5_DIGEST_LENGTH];
    int index;
    struct Md5Entry *prev;
    struct Md5Entry *next;
} Md5Entry;


// Fonction de hachage MD5 pour l'indexation dans la table de hachage
unsigned int hash_md5(unsigned char *md5);
// Fonction pour calculer le MD5 d'un chunk
void compute_md5(void *data, size_t len, unsigned char *md5_out);
// Fonction permettant de chercher un MD5 dans la table de hachage
int find_md5(Md5Entry **hash_table, unsigned char *md5);
// Fonction pour ajouter un MD5 dans la table de hachage
void add_md5(Md5Entry **hash_table, unsigned char *md5,int index);
// Fonction pour afficher la table de hachage
void see_hash_table(Md5Entry **hash_table);
// Fonction pour ajouter un chunk unique à la liste de chunks
Chunk_list add_unique_chunk(Chunk_list chunk,unsigned char *md5, unsigned char *tampon);
// Fonction pour ajouter un chunk déjà vu à la liste de chunks
Chunk_list add_seen_chunk(Chunk_list chunk,unsigned char *md5,int index);
// Fonction pour afficher la liste de chunks
void see_chunk_list(Chunk_list chunk);

/**
 * @brief Fonction pour convertir un fichier non dédupliqué en tableau de chunks
 * 
 * @param file le fichier qui sera dédupliqué
 * @param chunks le tableau de chunks initialisés qui contiendra les chunks issu du fichier
 * @param hash_table le tableau de hachage qui contient les MD5 et l'index des chunks unique
 */
void deduplicate_file(FILE *file, Chunk_list *chunks, Md5Entry **hash_table);

/*
 * @brief Fonction permettant de charger un fichier dédupliqué en table de chunks en remplaçant les références par les données correspondantes
 * 
 * @param file le nom du fichier dédupliqué
 * @param chunks représente le tableau de chunk qui contiendra les chunks restauré depuis filename
 * @param chunk_count est un compteur du nombre de chunk restauré depuis le fichier filename
void undeduplicate_file(FILE *file, Chunk_list chunks, int *chunk_count);*/

#endif // DEDUPLICATION_H