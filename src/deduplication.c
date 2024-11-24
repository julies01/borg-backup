#include "deduplication.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/md5.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>


/** 
 * @brief Une procédure qui vérifie si un paquet est installé et donne la commande pour l'installer dans le cas échéant
 * 
 * @param nom_pckg le nom du paquet à vérifier
 * @param commande_installer_pckg la commande pour installer la bibliothèque contenant le paquet
 */

void check_pckg(char *nom_pckg, char *commande_installer_pckg){
     #ifndef nom_pckg
        printf("%s n'est pas disponible.\n");
        printf("Pour l'installer, saissisez la commande suivant dans le terminal : \n");
        printf("%s\n",commande_installer_pckg);
    #endif
}


typedef struct FileChunk {
    unsigned char hash[HASH_SIZE];
    char *filename;
    unsigned int id_chunk;
    unsigned int version;
    struct FileChunk *next; // Pointeur vers le prochain bloc de données
} FileChunk;

FileChunk *table_hashage[TAILLE_TABLE];


/**
 * @brief Une fonction qui détermine l'index d'un hachage dans la table de hachage
 * 
 * @param hash une partie de fichier de 16 bytes
 * @return unsigned int, l'index du hachage dans la table de hachage
 */
unsigned int determine_index_hash(unsigned char *hash) {
    //Le principe de la fonction est de répartir les hachages de manière uniforme dans la table de hachage
    unsigned int hash_value = 0;
    for (int i = 0; i < HASH_SIZE; i++) {
        hash_value = (hash_value * 31) + hash[i]; //On multiplie par 31 car 31 est un nombre premier
    }
    return hash_value % TAILLE_TABLE;
}



/**
 * @brief Une procédure qui insère un bloc de données dans la table de hachage
 * 
 * @param table la table de hachage
 * @param hash 
 * @param filename 
 * @param id_chunk 
 * @param version 
 */
void insere_file_chunk(FileChunk **table, unsigned char *hash, const char *filename, unsigned int id_chunk, unsigned int version) {
    unsigned int index = determine_index_hash(hash);

    //Crée un nouveau file chunk
    FileChunk *nouveau_chunk = (FileChunk *)malloc(sizeof(FileChunk));
    memcpy(nouveau_chunk->hash, hash, HASH_SIZE);
    nouveau_chunk->filename = strdup(filename);
    nouveau_chunk->id_chunk = id_chunk;
    nouveau_chunk->version = version;
    nouveau_chunk->next = table[index];

    table[index] = nouveau_chunk;
}



/**
 * @brief Une fonction qui vérifie si un bloc de données est un doublon est renvoie l'original si oui
 * 
 * @param table 
 * @param hash 
 * @return FileChunk* 
 */
FileChunk *trouver_double(FileChunk **table, unsigned char *hash) {
    unsigned int index = determine_index_hash(hash);

    // Traverse la liste chaînée à l'indice donné pour voir si il ya un doublon
    FileChunk *courant = table[index];
    while (courant) {
        if (memcmp(courant->hash, hash, HASH_SIZE) == 0) {
            return courant; // renvoie l'orginal si celui en doublon a été trouvé
        }
        courant = courant->next;
    }

    return NULL; // pas de doublon trouvé
}



/**
 * @brief Une procédure qui calcule le hachage MD5 d'un bloc de données
 * 
 * @param chunk_data le bloc de données
 * @param chunk_size la taille du bloc de données
 * @param hash la taille du hachage MD5
 */
void hash_chunk(const unsigned char *chunk_data, size_t chunk_size, unsigned char hash[HASH_SIZE]) {
    MD5(chunk_data, chunk_size, hash);
}



/**
 * @brief Une fonction qui stocke un bloc de données dans un fichier
 * 
 * @param chunk_data 
 * @param chunk_size 
 * @param version 
 * @param filename 
 * @param destination_dir 
 * @return unsigned int l'identifiant du bloc de données
 */
unsigned int stocker_chunk_data(const unsigned char *chunk_data, size_t chunk_size, unsigned int version, const char *filename, const char *destination) {
    static unsigned int chunk_id = 0;
    char *chunk_filename;

    //Cherche l'extension du fichier
    const char *ext = strrchr(filename, '.');
    if (ext == NULL) {
        //Pas d'extension
        snprintf(chunk_filename, sizeof(chunk_filename), "%s/chunk_%u_v%u.bin", destination, chunk_id, version);
    } else {
        snprintf(chunk_filename, sizeof(chunk_filename), "%s/chunk_%u_v%u%s", destination, chunk_id, version, ext);
    }

    FILE *chunk_file = fopen(chunk_filename, "wb");
    if (!chunk_file) {
        perror("Impossible de stocker le chunk");
        exit(1);
    }
    fwrite(chunk_data, 1, chunk_size, chunk_file);
    fclose(chunk_file);

    return chunk_id++;
}



/**
 * @brief Une procédure qui traite un fichier en le découpant en blocs de données et en les stockant
 * 
 * @param filename le nom du fichier
 * @param destination le répertoire de destination 
 */
void traitement_fichier(const char *filename, const char *destination) {
    FILE *fichier = fopen(filename, "rb"); // Ouvre le fichier en mode lecture binaire
    if (!fichier) {
        perror("Erreur lors de l'ouverture du fichier");
        return;
    }

    unsigned char tampon[CHUNK_SIZE];
    unsigned char hash[HASH_SIZE];
    size_t bytes_lus;
    int index_chunk = 0;

    //Découpe du fichier en blocs de données (chunks)
    while ((bytes_lus = fread(tampon, 1, CHUNK_SIZE, fichier)) > 0) {
        
        hash_chunk(tampon, bytes_lus, hash);

        //On véririfie si le chunk est un doublon
        FileChunk *doublon = trouver_double(table_hashage, hash);
        if (doublon) {
            // If it is a duplicate, increment version number
            unsigned int new_version = doublon->version + 1;
            unsigned int new_chunk_id = stocker_chunk_data(tampon, bytes_lus, new_version, filename, destination);  // Store as new version
            insere_file_chunk(table_hashage, hash, filename, new_chunk_id, new_version);        // Insert the new version into the hash table
        } else {            
            unsigned int id_chunk = stocker_chunk_data(tampon, bytes_lus, 0, filename, destination);  //Stocke le nouveau chunk
            insere_file_chunk(table_hashage, hash, filename, id_chunk, 0); //Insère le chunk dans la table de hachage
        }
    }

    fclose(fichier);
}



/**
 * @brief Une procédure qui déduplique les fichiers d'un répertoire source et les stocke dans un répertoire de destination
 * 
 * @param source le chemin du répertoire source que l'on veut dédupliquer
 * @param destination le chemin du répertoire de destination de la déduplication
 */
void deduplicate_files(const char *source, const char *destination) {
  DIR *dir = opendir(source);
    if (!dir) {
        perror("Erreur lors de l'ouverture du répertoire source");
        return;
    }

    struct dirent *entree;
    struct stat stat_entree;
    char chemin_source[1024];

    //Parcours du répertoire source
    while ((entree = readdir(dir)) != NULL) {
        // Ignorer tous les fichiers cachés
        if (strcmp(entree->d_name, ".") == 0 || strcmp(entree->d_name, "..") == 0)
            continue;

        snprintf(chemin_source, sizeof(chemin_source), "%s/%s", source, entree->d_name); //Crée un nouveau chemin source
        if (stat(chemin_source, &stat_entree) == -1) {
            perror("Erreur lors de la récupération des informations sur le fichier");
            continue;
        }

        if (S_ISDIR(stat_entree.st_mode)) {
            //Appel récursif si c'est un répertoire
            deduplicate_files(chemin_source, destination);
        } else if (S_ISREG(stat_entree.st_mode)) {
            //Traiter le fichier si c'est un fichier normal
            traitement_fichier(chemin_source, destination);
        }
    }

    closedir(dir);
}
