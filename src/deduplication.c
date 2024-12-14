#include "deduplication.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <openssl/md5.h>
#include <dirent.h>

/**
 * @brief Une fonction de hachage MD5 pour l'indexation dans la table de hachage
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
 * @brief Une procédure pour calculer la somme MD5 d'un chunk
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
 * @brief Une fonction permettant de chercher une somme MD5 dans la table de hachage
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
 * @brief Une procédure pour ajouter une somme MD5 dans la table de hachage
 * 
 * @param hash_table le tableau de hachage qui contient les MD5 et l'index des chunks unique
 * @param md5 le md5 du chunk à ajouter
 * @param index l'index du chunk
 */

void add_md5(Md5Entry **hash_table, unsigned char *md5, int index) {
        Md5Entry *new_el = (Md5Entry *)malloc(sizeof(Md5Entry));
        memcpy(new_el->md5, md5, MD5_DIGEST_LENGTH); // memcpy est une fonction qui copie un certain nombre de bytes d'un espace mémoire à un autre
        new_el->index = index;//Stockage de l'index du chunk dans le chunk
        new_el->next = NULL;

        Md5Entry *current = hash_table[index]; //Parcours de la liste chaînée à l'indice index       
        if (current == NULL){ // Si la liste est vide
            hash_table[index] = new_el; // On ajoute le chunk à la liste
            return ; 
        } else {
            while (current->next != NULL){ //Sinon on parcourt la liste jusqu'à la fin
                current = current->next;
            }
            current->next = new_el; // On ajoute le chunk à la fin de la liste
        }
}

/**
 * @brief Une fonction pour ajouter un chunk avec une somme md5 unique dans la liste de chunks
 * 
 * @param chunk La liste doublement chaînée de chunks
 * @param md5 la somme MD5 du chunk
 * @param tampon la donnée du chunk
 * @return Chunk_list 
 */
Chunk_list add_unique_chunk(Chunk_list chunk,unsigned char *md5, unsigned char *tampon){
    Chunk *new_el = (Chunk *)malloc(sizeof(Chunk));
    new_el->is_unique = 0;
    memcpy(new_el->md5, md5, MD5_DIGEST_LENGTH); // memcpy est une fonction qui copie un certain nombre de bytes d'un espace mémoire à un autre
    new_el->data = malloc(CHUNK_SIZE);
    if (new_el->data == NULL) { //Gestion des erreurs
        perror("Impossible d'allouer de la mémoire");
        exit(EXIT_FAILURE);
    }
    memcpy(new_el->data, tampon, CHUNK_SIZE); //Copie de la data du tampon dans l'attribut data du chunk
    new_el->next = NULL;

    Chunk *current = chunk;
    if (current == NULL){ // Si la liste est vide
        chunk = new_el; // On ajoute le chunk à la liste
        chunk->prev = NULL;
        return chunk;

    } else {
        while (current->next != NULL){ //Sinon on parcourt la liste jusqu'à la fin
            current = current->next;
        }
        current->next = new_el;// On ajoute le chunk à la fin de la liste
        new_el->prev = current;
        return chunk; // On retourne la liste avec le nouveau chunk ajouté
    }
}

/**
 * @brief Une fonction permettant de trouver l'index d'un chunk dans la table de chunks à l'aide de la somme MD5
 * 
 * @param chunk la liste de chunks
 * @param md5 la somme MD5 du chunk déjà présent dont on cherche l'index
 * @return int l'index du chunk
 */

int find_index_Chunklist (Chunk_list chunk, unsigned char *md5){
    int index = 1;
    Chunk *current = chunk;
    while (current != NULL){
        if (memcmp(current->md5, md5, MD5_DIGEST_LENGTH) == 0){ // Si le md5 du chunk est égal au md5 du chunk déjà présent
            return index; // On retourne l'index du chunk
        }
        index++; // Sinon on incrémente l'index
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
    memcpy(new_el->md5, md5, MD5_DIGEST_LENGTH); // memcpy est une fonction qui copie un certain nombre de bytes d'un espace mémoire à un autre
    new_el->is_unique = 1;
    new_el->data = malloc(sizeof(int));
    memcpy(new_el->data, &index, sizeof(int)); //Copie de l'index du chunk auquel celui-ci fait référence dans l'attribut data du chunk
    new_el->next = NULL;

    Chunk *current = chunk; 
    if (current == NULL){ //Si la liste est vide
        chunk = new_el; // On ajoute le chunk à la liste
        chunk->prev = NULL;
        return chunk;

    } else {
        while (current->next != NULL){ //Sinon on parcourt la liste jusqu'à la fin
            current = current->next;
        }
        current->next = new_el;
        new_el->prev = current; // On ajoute le chunk à la fin de la liste
        return chunk;
    }
}


/**
 * @brief Fonction pour afficher la table de hachage (plutôt utile pour le débuggage)
 * 
 * @param hash_table le tableau de hachage qui contient les MD5 et l'index des chunks unique
 */

void see_hash_table(Md5Entry **hash_table){
    for (int i = 0; i < HASH_TABLE_SIZE; i++){ //Parcours des 1000 listes chaînées
        Md5Entry *current = hash_table[i]; //Parcours de chaque liste chaînée
        if (current != NULL) { 
            printf("Index : %d\n", i);
            while (current != NULL) {
                for (int j = 0; j < MD5_DIGEST_LENGTH; j++){
                    printf("%02x", current->md5[j]); // Affichage de la somme MD5
                }
                printf(" -> ");
                current = current->next; // Passage au maillon suivant
            }
            printf("NULL\n");
        }
    }
}

/**
 * @brief Fonction pour afficher la liste de chunks (plutôt utile pour le débuggage)
 * 
 * @param chunk la liste de chunks
 */
void see_chunk_list(Chunk_list chunk) {
    Chunk *current = chunk;
    int compteur = 0;
    while (current != NULL) { // Parcours de la liste de chunks jusqu'à la fin
        compteur++;
        printf("Chunk %d : ", compteur); // Affichage de l'index du chunk
        for (int j = 0; j < MD5_DIGEST_LENGTH; j++) {
            printf("%02x", current->md5[j]); // Affichage de la somme MD5
        }
        if (current->data == NULL) {
            printf(" et il n'y a pas de data\n");
        } else {
            if (current->is_unique == 1) { // Si le chunk est déjà présent dans la table de chunks
                int index;
                memcpy(&index, current->data, sizeof(int));
                printf(" et fait référence à %d de la table de Chunks\n", index); // Affichage de l'index du chunk auquel il fait référence
            } else {
                unsigned char *data_bytes = (unsigned char *)current->data; //Sinon on affiche les premiers bytes de la data
                printf(" les premiers cinq bytes de data : ");
                for (int k = 0; k < 5; k++) {
                    printf("%02x", data_bytes[k]); //affichage des 5 premiers bytes de la data
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
        if (index_h == -1) { // Si la somme MD5 du chunk n'est pas déjà présente dans la table de hachage (Chunk unique)
            index_h = hash_md5(hash);
            add_md5(hash_table, hash, index_h); // Ajout de la somme MD5 du chunk dans la table de hachage
            *chunks = add_unique_chunk(*chunks, hash, tampon); // Ajout du chunk dans la liste de chunks
            nb_chunks++;
        } else { //(Chunk doublon)
            int index_c = find_index_Chunklist(*chunks, hash); // Recherche de l'index du chunk déjà présent dans la liste de chunks
            *chunks = add_seen_chunk(*chunks, hash, index_c); // Ajout du chunk dans la liste de chunks
            nb_chunks++;
        }
    }
    printf("Nombre de chunks : %d\n", nb_chunks);
}

/**
 * @brief Une fonction qui lit la ligne contenant l'identificateur et retourne l'index du chunk de référence ou 0
 * 
 * @param identificator la chaîne contenant l'index du chunk et l'index du chunk deb référence s'il y en a un ou 0 sinon 
 * @return int l'index du chunk de référence ou 0
 */
int extract_second_number(const char *identificator) {
    const char *start = strstr(identificator, "[*(");
    int number;
    sscanf(start + 3, "%d", &number);
    return number;
}

/**
 * @brief Une procédure qui permet de récupérer la data d'un chunk à un indice donné
 * 
 * @param chunk La liste doublement chaînée de chunks
 * @param index L'index du chunk à trouver
 * @param data La donnée du chunk
 */
void find_data_in_chunklist(Chunk_list chunk, int index, void **data) {
    Chunk *current = chunk;
    int compteur = 1;
    while (compteur != index){ // Parcours de la liste de chunks jusqu'à l'index donné
        compteur++;
        current = current->next;
    }
    if (current->data == NULL){ //Gestion des erreurs
        printf("Erreur, il n'y a pas de data\n");
    } else {
        *data = current->data; //On récupère la data du chunk dans l'attribut data
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
    char *line = (char*)malloc(30 * sizeof(char)); // Allocation de la mémoire pour l'identificateur
    if (line == NULL) { //Gestion des erreurs
        fprintf(stderr, "Memory allocation failed for line\n");
        return;
    }
    size_t bytes_lus;

    fseek(file, 0, SEEK_SET); //On place le curseur au début du fichier

    while (!feof(file)) {
        if (fgets(line, 30, file) != NULL) {
            if (strstr(line, "!/(") != NULL) { // Si la ligne contient un identificateur
                int index = extract_second_number(line);
                if (index != 0) { // Si le chunk contient une référence à un autre chunk
                    void *data = NULL;
                    find_data_in_chunklist(*chunks, index, &data); //On récupère la data du chunk de référence
                    if (data == NULL) { //Gestion des erreurs
                        fprintf(stderr, "Data not found for index %d\n", index);
                        continue;
                    }
                    compute_md5(data, CHUNK_SIZE, hash);//Calcul de la somme MD5 de la data
                    *chunks = add_unique_chunk(*chunks, hash, data); //Ajout du chunk dans la liste de chunks
                } else { // Si le chunk est unique
                    bytes_lus = fread(tampon, 1, CHUNK_SIZE, file);
                    if (bytes_lus > 0) { 
                        compute_md5(tampon, bytes_lus, hash);//Calcul de la somme MD5 du chunk
                        *chunks = add_unique_chunk(*chunks, hash, tampon); //Ajout du chunk dans la liste de chunks
                    } else { //Gestion des erreurs
                        fprintf(stderr, "Failed to read chunk from file\n");
                    }
                }
            }
        }
    }
    free(line);
}