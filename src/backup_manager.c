#include "backup_manager.h"
#include "deduplication.h"
#include "file_handler.h"
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
#include <unistd.h>
#include <fcntl.h>
#include <sys/sendfile.h>

#define PATH_MAX 4096

// Fonction pour convertir un nom de dossier en structure tm (date)
int parse_folder_date(const char *folder_name, struct tm *result) {
    return sscanf(folder_name, "%4d-%2d-%2d-%2d:%2d:%2d.%3d",
                  &result->tm_year, &result->tm_mon, &result->tm_mday,
                  &result->tm_hour, &result->tm_min, &result->tm_sec, &(int){0}) == 7;
}

// Fonction principale
char *find_most_recent_folder(const char *base_path) {
    DIR *dir = opendir(base_path);
    if (!dir) {
        perror("Impossible d'ouvrir le répertoire");
        return NULL;
    }

    struct dirent *entry;
    struct tm most_recent_tm = {0};
    char *most_recent_folder = NULL;

    while ((entry = readdir(dir)) != NULL) {
        // Ignore les dossiers "." et ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Vérifie si c'est un dossier
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", base_path, entry->d_name);

        struct stat statbuf;
        if (stat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
            struct tm folder_tm = {0};
            if (parse_folder_date(entry->d_name, &folder_tm)) {
                folder_tm.tm_year -= 1900; // tm_year est compté depuis 1900
                folder_tm.tm_mon -= 1;    // tm_mon est compté de 0 à 11

                // Comparer la date actuelle avec la plus récente
                time_t folder_time = mktime(&folder_tm);
                time_t most_recent_time = mktime(&most_recent_tm);
                if (most_recent_folder == NULL || difftime(folder_time, most_recent_time) > 0) {
                    free(most_recent_folder); // Libère la mémoire de la précédente si nécessaire
                    most_recent_folder = strdup(entry->d_name);
                    most_recent_tm = folder_tm;
                }
            }
        }
    }

    closedir(dir);
    return most_recent_folder;
}

void copy_file_link(const char *source, const char *destination) {
    int source_fd, dest_fd;
    struct stat stat_buf;
    off_t offset = 0;
    source_fd = open(source, O_RDONLY);
    if (source_fd == -1) {
        perror("open source");
        exit(EXIT_FAILURE);
    }
    if (fstat(source_fd, &stat_buf) == -1) {
        perror("fstat");
        close(source_fd);
        exit(EXIT_FAILURE);
    }
    dest_fd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, stat_buf.st_mode);
    if (dest_fd == -1) {
        perror("open destination");
        close(source_fd);
        exit(EXIT_FAILURE);
    }
    if (sendfile(dest_fd, source_fd, &offset, stat_buf.st_size) == -1) {
        perror("sendfile");
        close(source_fd);
        close(dest_fd);
        exit(EXIT_FAILURE);
    }
    close(source_fd);
    close(dest_fd);
}

/**
 * @brief Une procédure permettant de copier un répertoire avec les liens dur
 * 
 * @param source_dir Le chemin du répertoire source
 * @param dest_dir Le chemin du répertoire de destination
 */
void copy_directory_link(const char *source, const char *destination) {
    DIR *dir;
    struct dirent *entry;
    char source_path[PATH_MAX];
    char dest_path[PATH_MAX];
    struct stat stat_buf;
    if (mkdir(destination, 0755) == -1 && errno != EEXIST) {
        perror("mkdir");
        exit(EXIT_FAILURE);
    }
    dir = opendir(source);
    if (!dir) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(source_path, PATH_MAX, "%s/%s", source, entry->d_name);
        snprintf(dest_path, PATH_MAX, "%s/%s", destination, entry->d_name);

        if (stat(source_path, &stat_buf) == -1) {
            perror("stat");
            continue;
        }

        if (S_ISDIR(stat_buf.st_mode)) {
            copy_directory_link(source_path, dest_path);
        } else {
            copy_file_link(source_path, dest_path);
            if (unlink(source_path) == -1) {
                perror("unlink");
                exit(EXIT_FAILURE);
            }
            if (link(dest_path, source_path) == -1) {
                perror("link");
                exit(EXIT_FAILURE);
            }
        }
    }
    closedir(dir);
}

void copy_directory(const char *source_dir, const char *dest_dir) {
    DIR *dir = opendir(source_dir);
    if (!dir) {
        perror("Erreur lors de l'ouverture du répertoire source");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    struct stat statbuf;

    mkdir(dest_dir, 0755);

    while ((entry = readdir(dir)) != NULL) {
        printf("Copie de %s/%s\n", source_dir, entry->d_name);
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char src_path[PATH_MAX];
        char dest_path[PATH_MAX];
        snprintf(src_path, sizeof(src_path), "%s/%s", source_dir, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, entry->d_name);

        if (stat(src_path, &statbuf) == -1) {
            perror("Erreur lors de la récupération des informations sur un fichier");
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            copy_directory(src_path, dest_path);
        } else {
            backup_file(src_path, dest_path);
        }
    }

    closedir(dir);
}

/**
 * @brief Une procédure permettant obtenir le timestamp actuel
 * 
 * @param buffer le tampon pour stocker le timestamp
 * @param size la taille du tampon
 */
void get_current_timestamp(char *buffer, size_t size) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm *local_time = localtime(&tv.tv_sec);
    char temp_buffer[64];
    strftime(temp_buffer, sizeof(temp_buffer), "%Y-%m-%d-%H:%M:%S", local_time);
    snprintf(buffer, size, "%s.%03ld", temp_buffer, tv.tv_usec / 1000);
}

/**
 * @brief Une procédure pour créer un nouveau backup incrémental
 * 
 * @param source_dir le répertoire source
 * @param backup_dir le répertoire de destination
 */
void create_backup(const char *source_dir, const char *backup_dir) {
    char date_str[64];
    char fichierlog[128];
    get_current_timestamp(date_str, sizeof(date_str));
    mkdir(backup_dir, 0755);
    char new_backup_dir[PATH_MAX];
    snprintf(new_backup_dir, sizeof(new_backup_dir), "%s/%s", backup_dir, date_str);
    strcat(fichierlog, backup_dir);
    strcat(fichierlog, "/");
    strcat(fichierlog, "backup_log");
    FILE *log = fopen(fichierlog, "r");
    if(log == NULL){
        log = fopen(fichierlog, "w");
        printf("Copie des fichier de : %s dans : %s\n", source_dir, new_backup_dir);
        copy_directory(source_dir, new_backup_dir);
    }else{
        char *closest_backup = find_most_recent_folder(backup_dir);
        strcat(backup_dir, "/");
        strcat(backup_dir, closest_backup);
        printf("Restauration de la sauvegarde la plus proche : %s\n", closest_backup);
        copy_directory_link(backup_dir, new_backup_dir);
        return;
    }
    if (mkdir(new_backup_dir, 0755) == -1 && errno != EEXIST) {
        perror("Erreur lors de la création du répertoire de sauvegarde");
        exit(EXIT_FAILURE);
    }
    fclose(log);

    printf("Sauvegarde terminée dans : %s\n", new_backup_dir);
}

/**
 * @brief Une procédure permettant d'enregistrer dans fichier le tableau de chunk dédupliqué

 * 
 * @param output_filename le fichier de sortie
 * @param chunks le tableau de chunks
 */
void write_backup_file(const char *output_filename, Chunk *chunks) {
    /*
    */
    FILE *file = fopen(output_filename, "wb");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier\n\n");
        return;
    }
    while(chunks->next != NULL){
        if(chunks->data != NULL){
            int size = 4096;
            size_t written_size = fwrite(chunks->data, size, 1, file);
            if (written_size != 1) {
                perror("Erreur lors de l'écriture dans le fichier");
            }
        }
        chunks = chunks->next;
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
    Md5Entry *hash_table[HASH_TABLE_SIZE] = {0};
    size_t max_chunks = 1000;
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
 * @brief Procédure  qui restaure une sauvegarde
 * 
 * @param backup_id chemin vers de répertoire de la sauvegarde que l'on veut restaurer
 * @param restore_dir répertoire ou sera restaurée la sauvegarde
 */
void restore_backup(const char *backup_id, const char *restore_dir) {
    DIR *dir;
    struct dirent *entry;
    char backup_path[PATH_MAX];
    char restore_path[PATH_MAX];
    struct stat st;

    dir = opendir(backup_id);
    if (!dir) {
        fprintf(stderr, "Erreur : impossible d'ouvrir le répertoire de sauvegarde %s : %s\n", backup_id, strerror(errno));
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(backup_path, sizeof(backup_path), "%s/%s", backup_id, entry->d_name);
        snprintf(restore_path, sizeof(restore_path), "%s/%s", restore_dir, entry->d_name);

        if (stat(backup_path, &st) == -1) {
            fprintf(stderr, "Erreur : impossible de récupérer les informations du fichier %s : %s\n", backup_path, strerror(errno));
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            mkdir(restore_path, 0755);
            restore_backup(backup_path, restore_path);
        } else if (S_ISREG(st.st_mode)) {
            int src_fd = open(backup_path, O_RDONLY);
            if (src_fd == -1) {
                fprintf(stderr, "Erreur : impossible d'ouvrir le fichier source %s : %s\n", backup_path, strerror(errno));
                continue;
            }

            int dest_fd = open(restore_path, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode);
            if (dest_fd == -1) {
                fprintf(stderr, "Erreur : impossible de créer le fichier de destination %s : %s\n", restore_path, strerror(errno));
                close(src_fd);
                continue;
            }

            off_t offset = 0;
            if (sendfile(dest_fd, src_fd, &offset, st.st_size) == -1) {
                fprintf(stderr, "Erreur : échec de la copie du fichier %s vers %s : %s\n", backup_path, restore_path, strerror(errno));
            }

            close(src_fd);
            close(dest_fd);
        }
    }

    closedir(dir);
}

/**
 * @brief Fonction qui calcule la taille d'un répertoire
 * 
 * @param directory le répertoire dont on veut connaître la taille/l'existence
 * @return entier, -1 si le répertoire n'existe pas, la taille du répertoire sinon
 */
int taille_dossier(const char *directory) {
    int total_size = 0;
    struct dirent *fichier;
    DIR *dir = opendir(directory);

    if (!dir) {
        printf("Le répertoire n'éxiste pas");
        return -1;
    }

    while ((fichier = readdir(dir)) != NULL) {
        if (strcmp(fichier->d_name, ".") == 0 || strcmp(fichier->d_name, "..") == 0) {
            continue;
        }

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", directory, fichier->d_name);
        struct stat st;
        if (stat(path, &st) == -1) {
            printf("Erreur lors de la récupération des informations du fichier");
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            int dir_size = taille_dossier(path);
            if (dir_size != -1) {
                total_size += dir_size;
            }
        } else if (S_ISREG(st.st_mode)) {
            total_size += st.st_size;
        }
    }

    closedir(dir);
    return total_size;
} 

/**
 * @brief Procédure listant les sauvegardes dans un répertoire, et
 *          donnant des info sur chaque sauvegarde (chaque sous répertoire dans ce répertoire enfaite)
 * 
 * @param directory chemin vers le répertoire avec les sauvegardes
 * @param verbose mode verbose activé ou non
 */
void list_backup(const char *directory, int verbose) {
    struct dirent *fichier;
    DIR *dir = opendir(directory);

    if (!dir) {
        printf("Le répertoire n'éxiste pas");
        return;
    }

    while ((fichier = readdir(dir)) != NULL) {
        
        if (strcmp(fichier->d_name, ".") == 0 || strcmp(fichier->d_name, "..") == 0) {
            continue;
        }
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", directory, fichier->d_name);
        char *chemin_absolue = realpath(path, NULL);
        struct stat st;
        if (stat(path, &st) == -1) {
            printf("Erreur lors de la récupération des informations du répertoire");
            continue;
        }
        if (S_ISDIR(st.st_mode)) {
            int dir_size = taille_dossier(path);
            if (verbose) {
                printf("Nom du répertoire: %s\n", fichier->d_name);
                printf("Chemin complet: %s\n", chemin_absolue);
                printf("Taille totale: %d octets\n", dir_size);
                printf("Date de création: %s", ctime(&st.st_ctime));
            } else {
                printf("Nom du répertoire: %s\n", fichier->d_name);
                printf("Taille totale: %d octets\n", dir_size);
            }
            printf("\n");
            free(chemin_absolue);
        }
    }

    closedir(dir);
}