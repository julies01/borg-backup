#include "deduplication.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
        hash = (hash << 5) + hash + md5[i]; // Calcul de l'index de hachage
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
    if (md5_out == NULL) {
        fprintf(stderr, "md5_out buffer is not allocated\n");
        exit(EXIT_FAILURE);
    }
    MD5(data, len, md5_out); // Fonction de la librairie openssl qui permet de calculer le MD5 d'un chunk
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
    }
    return -1;
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
    new_el->is_unique = 0;
    memcpy(new_el->md5, md5, MD5_DIGEST_LENGTH);
    new_el->data = malloc(CHUNK_SIZE);
    if (new_el->data == NULL) {
        perror("Impossible d'allouer de la mémoire");
        exit(EXIT_FAILURE);
    }
    memcpy(new_el->data, tampon, CHUNK_SIZE);
    new_el->next = NULL;

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
 * @brief Create a index object
 * 
 * @param chunk la liste de chunks
 * @param md5 la somme MD5 du chunk déjà présent dont on cherche l'index
 * @return int l'index du chunk
 */

int create_index(Chunk_list chunk, unsigned char *md5){
    int index = 1;
    Chunk *current = chunk;
    while (current != NULL){
        if (memcmp(current->md5, md5, MD5_DIGEST_LENGTH) == 0){
            return index;
        }
        index++;
        current = current->next;
    }

    return index;
}

/**
 * @brief Une fonction pour ajouter un chunk déjà répertorié dans la table de hashage, dans le tableau de chunks
 * 
 * @param chunk le tableau de chunks
 * @param md5 la somme MD5 du chunk
 * @param index l'index du chunk dans le tableau de chunks
 * @return Chunk_list le tableau de chunks mis à jour
 */
Chunk_list add_seen_chunk(Chunk_list chunk, unsigned char *md5,int index){
    Chunk *new_el = (Chunk *)malloc(sizeof(Chunk));
    memcpy(new_el->md5, md5, MD5_DIGEST_LENGTH);
    new_el->is_unique = 1;
    new_el->data = malloc(sizeof(int));
    memcpy(new_el->data, &index, sizeof(int));
    new_el->next = NULL;

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
    int compteur = 0;
    while (current != NULL){
        compteur++;
        printf("Chunk %d : ", compteur);
        for (int j = 0; j < MD5_DIGEST_LENGTH; j++) {
            printf("%02x", current->md5[j]);
        }
        if (current->data == NULL){
            printf(" et fait référence à NULL de la table de hashage\n");
        } else {
            if (current->is_unique == 1) {
                int index;
                memcpy(&index, current->data, sizeof(int));
                printf(" et fait référence à %d de la table de hashage\n", index);
            } else {
                unsigned char *data_bytes = (unsigned char *)current->data;
                printf(" les premiers cinq bytes de data : ");
                for (int k = 0; k < 5; k++) {
                    printf("%02x", data_bytes[k]);
                }
                printf("\n");
            }
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
void deduplicate_file(FILE *file, Chunk_list *chunks, Md5Entry **hash_table) {
    unsigned char tampon[CHUNK_SIZE];
    unsigned char hash[MD5_DIGEST_LENGTH];
    size_t bytes_lus;
    int nb_chunks = 0;

    while ((bytes_lus = fread(tampon, 1, CHUNK_SIZE, file)) > 0) { // Lecture du fichier en chunks
        compute_md5(tampon, bytes_lus, hash);
        int index_h = find_md5(hash_table, hash);
        if (index_h == -1) { // Si le chunk n'est pas déjà présent dans la table de hachage 
            index_h = hash_md5(hash);
            add_md5(hash_table, hash, index_h); // Ajout du chunk dans la table de hachage
            *chunks = add_unique_chunk(*chunks, hash, tampon); // Ajout du chunk dans la liste de chunks
            nb_chunks++;
        } else {
            int index_c = create_index(*chunks, hash);
            *chunks = add_seen_chunk(*chunks, hash, index_c); // Ajout du chunk dans la liste de chunks
            nb_chunks++;
        }
    }
    printf("Nombre de chunks : %d\n", nb_chunks);
}


int extract_second_number(const char *identificator) {
    const char *start = strstr(identificator, "[*(");
    int number;
    sscanf(start + 3, "%d", &number);
    return number;
}

void find_data_in_chunklist(Chunk_list chunk, int index, void **data) {
    Chunk *current = chunk;
    int compteur = 1;
    while (compteur != index){
        compteur++;
        current = current->next;
    }
    if (current->data == NULL){
        printf("Erreur, il n'y a pas de data\n");
    } else {
        *data = current->data;
    }
}



/**
 * @brief Fonction permettant de charger un fichier dédupliqué en table de chunks en remplaçant les références par les données correspondantes
 * 
 * @param file le nom du fichier dédupliqué
 * @param chunks représente le tableau de chunk qui contiendra les chunks restauré depuis filename
 * @param chunk_count est un compteur du nombre de chunk restauré depuis le fichier filename
 */

void undeduplicate_file(FILE *file, Chunk_list *chunks) {
    unsigned char hash[MD5_DIGEST_LENGTH];
    unsigned char tampon[CHUNK_SIZE];
    char *line = (char*)malloc(30 * sizeof(char));
    if (line == NULL) {
        fprintf(stderr, "Memory allocation failed for line\n");
        return;
    }
    size_t bytes_lus;

    fseek(file, 0, SEEK_SET);

    while (!feof(file)) {
        if (fgets(line, 30, file) != NULL) {
            if (strstr(line, "!/(") != NULL) { // Si la ligne contient un identificateur
                int index = extract_second_number(line);
                if (index != 0) { // Si le chunk contient une référence à un autre chunk
                    void *data = NULL;
                    find_data_in_chunklist(*chunks, index, &data);
                    if (data == NULL) {
                        fprintf(stderr, "Data not found for index %d\n", index);
                        continue;
                    }
                    compute_md5(data, CHUNK_SIZE, hash);
                    *chunks = add_unique_chunk(*chunks, hash, data);
                } else {
                    bytes_lus = fread(tampon, 1, CHUNK_SIZE, file);
                    if (bytes_lus > 0) {
                        compute_md5(tampon, bytes_lus, hash);
                        *chunks = add_unique_chunk(*chunks, hash, tampon);
                    } else {
                        fprintf(stderr, "Failed to read chunk from file\n");
                    }
                }
            }
        }
    }

    free(line);
}