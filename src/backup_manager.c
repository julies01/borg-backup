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

int parse_directory_name(const char *dir_name, struct tm *tm) {
    return sscanf(dir_name, "%4d-%2d-%2d-%2d:%2d:%2d",
                  &tm->tm_year, &tm->tm_mon, &tm->tm_mday,
                  &tm->tm_hour, &tm->tm_min, &tm->tm_sec) == 6;
}

double time_diff(struct tm *tm1, struct tm *tm2) {
    time_t t1 = mktime(tm1);
    time_t t2 = mktime(tm2);
    return difftime(t1, t2);
}

char* find_closest_backup(const char *backup_dir) {
    DIR *dir;
    struct dirent *entry;
    struct tm current_tm, entry_tm;
    struct timeval tv;
    double min_diff = -1;
    char *closest_dir = NULL;
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &current_tm);
    dir = opendir(backup_dir);
    if (!dir) {
        perror("opendir");
        return NULL;
    }

    while ((entry = readdir(dir)) != NULL) {
        struct stat entry_stat;
        char entry_path[PATH_MAX];
        snprintf(entry_path, sizeof(entry_path), "%s/%s", backup_dir, entry->d_name);
        if (stat(entry_path, &entry_stat) == 0 && S_ISDIR(entry_stat.st_mode) &&
            parse_directory_name(entry->d_name, &entry_tm)) {
            double diff = fabs(time_diff(&current_tm, &entry_tm));
            if (min_diff == -1 || diff < min_diff) {
                min_diff = diff;
                if (closest_dir) {
                    free(closest_dir);
                }
                closest_dir = strdup(entry->d_name);
            }
        }
    }

    closedir(dir);
    return closest_dir;
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
    strcat(fichierlog, ".backup_log");
    FILE *log = fopen(fichierlog, "r");
    if(log == NULL){
        log = fopen(fichierlog, "w");
        printf("Copie des fichier de : %s dans : %s\n", source_dir, new_backup_dir);
        copy_directory(source_dir, new_backup_dir);
    }else{
        char *closest_backup = find_closest_backup(backup_dir);
        strcat(backup_dir, closest_backup);
        printf("Restauration de la sauvegarde la plus proche : %s\n", fichierlog);
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
 * @brief Une procédure qui permet de restaurer la dernière sauvegarde
 * 
 * @param backup_id le chemin vers le répertoire de la sauvegarde que l'on veut restaurer
 * @param restore_dir le répertoire de destination de la restauration
 */
void restore_backup(const char *backup_id, const char *restore_dir) {
    
}

/**
 * @brief Une fonction qui calcule la taille d'un répertoire
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
 * @brief Une procédure permettant de lister des informations sur des répertoires
 * 
 * @param directory le chemin du répertoire que l'on souhaite traiter
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
