#include "deduplication.h"
#include "file_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/md5.h>
#include <dirent.h>

/**
 * @brief Fonction de hachage MD5 pour l'indexation dans la table de hachage
 * 
 * @param md5 la somme MD5 du chunk
 * @return unsigned int l'index de hachage
 */
unsigned int hash_md5(unsigned char *md5) {
    unsigned int hash = 0;
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        hash = (hash << 5) + hash + md5[i];
    }
    return hash % HASH_TABLE_SIZE;
}

/**
 * @brief Fonction pour calculer le MD5 d'un chunk
 * 
 * @param data le contenu du chunk
 * @param len la taille du chunk
 * @param md5_out la somme MD5 du chunk en sortie
 */
void compute_md5(void *data, size_t len, unsigned char *md5_out) {
    MD5(data, len, md5_out);// Fonction de la librairie openssl qui permet de calculer le MD5 d'un chunk

}


/**
 * @brief Fonction permettant de chercher un MD5 dans la table de hachage
 * 
 * @param hash_table le tableau de hachage qui contient les MD5 et l'index des chunks unique
 * @param md5 md5 est le md5 du chunk dont on veut déterminer l'unicité
 * @return int l'index s'il trouve le md5 dans le tableau et -1 sinon
 */
int find_md5(Md5Entry **hash_table, unsigned char *md5) {
    int i;
    for (i = 0; i < HASH_TABLE_SIZE; i++){ // Parcours de la table de hachage
        Md5Entry *current = hash_table[i];
        while (current != NULL){
            if (memcmp(current->md5, md5, MD5_DIGEST_LENGTH) == 0){
                return current->index; // Retourne l'index si le md5 est trouvé
            }
            current = current->next;
        }
        i++;
    }
        return -1;
}

/**
 * @brief 
 * 
 * @param chunk 
 * @param md5 
 * @return Chunk* 
 */
Chunk * find_md5_in_Chunk_list(Chunk_list chunk, unsigned char *md5){
    Chunk *current = chunk;
    while (current != NULL){
        if (memcmp(current->md5, md5, MD5_DIGEST_LENGTH) == 0){
            return current;
        }
        current = current->next;
    }
    return NULL;
}


/**
 * @brief Fonction pour ajouter un MD5 dans la table de hachage
 * 
 * @param hash_table le tableau de hachage qui contient les MD5 et l'index des chunks unique
 * @param md5 le md5 du chunk à ajouter
 * @param index l'index du chunk
 */

void add_md5(Md5Entry **hash_table, unsigned char *md5, int index) {
        Md5Entry *new_el = (Md5Entry *)malloc(sizeof(Md5Entry));
        memcpy(new_el->md5, md5, MD5_DIGEST_LENGTH); // memcpy est une fonction qui copie un certain nombre de bytes d'un espace mémoire à un autre
        new_el->index = index;
        new_el->next = NULL;

        Md5Entry *current = hash_table[index];        
        if (current == NULL){
            hash_table[index] = new_el;
            return ; 
        } else {
            while (current->next != NULL){
                current = current->next;
        }
        current->next = new_el;
        }
}

/**
 * @brief 
 * 
 * @param chunk 
 * @param md5 
 * @param tampon 
 * @return Chunk_list 
 */
Chunk_list add_unique_chunk(Chunk_list chunk,unsigned char *md5, unsigned char *tampon){
    Chunk *new_el = (Chunk *)malloc(sizeof(Chunk));
    memcpy(new_el->md5, md5, MD5_DIGEST_LENGTH);
    new_el->data = malloc(CHUNK_SIZE);
    memcpy(new_el->data, tampon, CHUNK_SIZE);

    Chunk *current = chunk;
    if (current == NULL){
        chunk = new_el;
        chunk->prev = NULL;
        return chunk;

    } else {
        while (current->next != NULL){
            current = current->next;
        }
        current->next = new_el;
        new_el->prev = current;
        return chunk;
    }
}

//pas encore vérifié
Chunk_list add_seen_chunk(Chunk_list chunk, unsigned char *md5){
    Chunk *new_el = (Chunk *)malloc(sizeof(Chunk));
    memcpy(new_el->md5, md5, MD5_DIGEST_LENGTH);
    new_el->data = NULL;

    Chunk *current = chunk;
    if (current == NULL){
        chunk = new_el;
        chunk->prev = NULL;
        return chunk;

    } else {
        while (current->next != NULL){
            current = current->next;
        }
        current->next = new_el;
        new_el->prev = current;
        return chunk;
    }
}

/**
 * @brief Fonction pour afficher la table de hachage
 * 
 * @param hash_table le tableau de hachage qui contient les MD5 et l'index des chunks unique
 */

void see_hash_table(Md5Entry **hash_table){
    for (int i = 0; i < HASH_TABLE_SIZE; i++){
        Md5Entry *current = hash_table[i];
        if (current != NULL) {
            printf("Index : %d\n", i);
            while (current != NULL) {
                for (int j = 0; j < MD5_DIGEST_LENGTH; j++){
                    printf("%02x", current->md5[j]);
                }
                printf(" -> ");
                current = current->next;
            }
            printf("NULL\n");
        }
    }
}

/**
 * @brief Fonction pour afficher la liste de chunks
 * 
 * @param chunk la liste de chunks
 */
void see_chunk_list(Chunk_list chunk){
    Chunk *current = chunk;
    while (current != NULL){
        for (int j = 0; j < MD5_DIGEST_LENGTH; j++){
            printf("%02x et", current->md5[j]);
            if (current->data == NULL){
                printf(" data nulle \n");
            }
            else
            printf(" la data à l'adresse : %p\n", current->data);
        }
        printf("\n\n");
        current = current->next;
    }
}

/**
 * @brief Fonction pour convertir un fichier non dédupliqué en tableau de chunks
 * 
 * @param file le fichier qui sera dédupliqué
 * @param chunks le tableau de chunks initialisés qui contiendra les chunks issu du fichier
 * @param hash_table le tableau de hachage qui contient les MD5 et l'index des chunks unique
 */
void deduplicate_file(FILE *file, Chunk_list chunks, Md5Entry **hash_table){
    unsigned char tampon[CHUNK_SIZE];
    unsigned char hash[MD5_DIGEST_LENGTH];
    size_t bytes_lus;

    while ((bytes_lus = fread(tampon, 1, CHUNK_SIZE, file)) > 0) { // Lecture du fichier en chunks
        compute_md5(tampon, bytes_lus, hash);
        int index = find_md5(hash_table, hash);
        if (index == -1){ // Si le chunk n'est pas déjà présent dans la table de hachage 
            index = hash_md5(hash);
            add_md5(hash_table, hash, index); // Ajout du chunk dans la table de hachage
        }
        if (find_md5_in_Chunk_list(chunks, hash) == NULL) {
            chunks = add_unique_chunk(chunks, hash, tampon);
        } else {
            chunks = add_seen_chunk(chunks, hash);
                
        }

        
    }
    see_chunk_list(chunks);
    printf("____________________________________________________________________________________\n");
    //see_hash_table(hash_table);
}


/**
 * @brief Fonction permettant de charger un fichier dédupliqué en table de chunks en remplaçant les références par les données correspondantes
 * 
 * @param file le nom du fichier dédupliqué
 * @param chunks représente le tableau de chunk qui contiendra les chunks restauré depuis filename
 * @param chunk_count est un compteur du nombre de chunk restauré depuis le fichier filename
 */
/*void undeduplicate_file(FILE *file, Chunk *chunks, int *chunk_count) {
    unsigned char hash[MD5_DIGEST_LENGTH];
    unsigned char tampon[CHUNK_SIZE];
    size_t bytes_lus;

    while ((bytes_lus = fread(hash, 1, MD5_DIGEST_LENGTH, file)) > 0) { // Lecture des sommes MD5 des chunks du fichier 
        int i;
        int index = find_md5(hash_table, hash); // Recherche de la somme MD5 dans la table de hachage
        if (index == -1) {
            fprintf(stderr, "Erreur: chunk introuvable dans la table de hachage\n");
            return;
        }

        int trouve = 0;
        for (i = 0; i < *chunk_count; i++) {
            if (memcmp(chunks[i].md5, hash,MD5_DIGEST_LENGTH) == 0) {
                memcpy(tampon, chunks[index].data, CHUNK_SIZE); //On copie le chunk dans le tampon
                fwrite(tampon, 1, CHUNK_SIZE, file);
                (*chunk_count)++;
                trouve = 1;
                break;
            }
        }
        if (!trouve) {
            fprintf(stderr, "Erreur: chunk correspondant au hash introuvable\n");
            return;
        }
    }
   
}*/

