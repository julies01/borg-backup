#include "backup.h"
#include "deduplication.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <limits.h> 

#define PATH_MAX 4096


/**
 * @brief Une procédure permettant d'enregistrer dans fichier le tableau de chunk dédupliqué

 * 
 * @param output_filename le fichier de sortie
 * @param chunks le tableau de chunks
 */
void write_backup_file(const char *output_filename, Chunk *chunks) {
    FILE *file = fopen(output_filename, "wb");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier");
        return;
    }
    Chunk *current = chunks;
    int chunk_count = 0;
    while(current != NULL){
        chunk_count++;
        char* identificator = (char*)malloc(30*sizeof(char));
        if (current->data == NULL){
            printf("Erreur, il n'y a pas de data \n");
        } else {
            if(current->is_unique == 1){
                int index;
                memcpy(&index, current->data, sizeof(int));
                sprintf(identificator, "!/(%d)/![*(%d)*]", chunk_count,index);
                size_t ecriture = fwrite(identificator, strlen(identificator) * sizeof(char), 1, file);
                free(identificator);
                if (ecriture != 1) {
                    perror("Erreur lors de l'écriture de l'index et de la référence dans le fichier");
                }
            }
            else {
                int index = 0;
                sprintf(identificator, "!/(%d)/![*(%d)*]", chunk_count,index);
                size_t ecriture = fwrite(identificator, strlen(identificator) * sizeof(char), 1, file);
                if (ecriture != 1) {
                    perror("Erreur lors de l'écriture de l'index et de la référence dans le fichier");
                }
                free(identificator);
                fwrite("\n", sizeof(char), 1, file);
                size_t data = fwrite(current->data, CHUNK_SIZE, 1, file);
                if (data != 1) {
                    perror("Erreur lors de l'écriture de la data dans le fichier");
                }
            }
        }
        fwrite("\n", sizeof(char), 1, file);
        current = current->next;
    }
    fclose(file);
    return;
}

/**
 * @brief Une procédure implémentant la logique pour la sauvegarde d'un fichier
 * 
 * @param filename le nom du fichier à traiter
 * @param backup_dir le chemin du répertoire de sauvegarde
 */
void backup_file(const char *filename, const char *backup_dir) {
    printf("Sauvegarde du fichier : %s\n", filename);
    FILE *file = fopen(filename, "rb");
    Md5Entry *hash_table[HASH_TABLE_SIZE] = {NULL};
    Chunk_list chunks = NULL;

    deduplicate_file(file, &chunks, hash_table);
    write_backup_file(backup_dir, chunks);

    fclose(file);
    return;
}


/** 
 * @brief Une procédure permettant la restauration du fichier backup via le tableau de chunk
 * 
 * @param output_filename fichier de sortie avec les chunks restorés
 * @param chunks tableau de chunks
 * @param chunk_count nombre de chunks contenus dans le tableau
 */
void write_restored_file(const char *output_filename, Chunk *chunks, int chunk_count) {
    FILE *dest = fopen(output_filename, "wb");
    if (!dest){
        fprintf(stderr, "erreur : impossible de créer le fichier %s : %s\n", output_filename, strerror(errno));
        return;
    }
    for (int i = 0; i < chunk_count && chunks != NULL; i++, chunks = chunks->next) {
        if (chunks->data != NULL){
            size_t written_size = fwrite(chunks->data, CHUNK_SIZE, 1, dest);
            if (written_size != 1) {
                fprintf(stderr, "erreur : ecriture incomplète dans %s : %s\n", output_filename, strerror(errno));
                fclose(dest);
                return;
            }
        }
    }

    fclose(dest);
}
        
    
/**
 * @brief Une procédure qui permet de restaurer la dernière sauvegarde
 * 
 * @param backup_id le chemin vers le répertoire de la sauvegarde que l'on veut restaurer
 * @param restore_dir le répertoire de destination de la restauration
 */
void restore_backup(const char *backup_id, const char *restore_dir) {
    
}