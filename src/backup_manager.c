#include "backup_manager.h"
#include "deduplication.h"
#include "file_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <limits.h> 
#include <unistd.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <regex.h>
#include <openssl/md5.h>

#define PATH_MAX 4096

void add_log_element(log_t *logs, log_element *elem) {
    printf("Ajout de l'élément %s\n", elem->path);
    if (logs->tail == NULL) {
        // La liste est vide, initialiser head et tail
        logs->head = elem;
        logs->tail = elem;
        elem->prev = NULL;
        elem->next = NULL;
    } else {
        // Ajouter l'élément à la fin de la liste
        elem->prev = logs->tail;
        elem->next = NULL;
        logs->tail->next = elem;
        logs->tail = elem;
    }
}

int file_exists_in_directory(const char *file_path, const char *directory) {
    DIR *dir;
    struct dirent *entry;
    char relative_path[PATH_MAX];
    const char *relative_file_path = strrchr(file_path, '/');
    if (!relative_file_path) {
        relative_file_path = file_path;
    } else {
        relative_file_path++; // Skip the '/'
    }

    dir = opendir(directory);
    if (!dir) {
        perror("opendir");
        return 0;
    }

    struct stat stat_buf;
    while ((entry = readdir(dir)) != NULL) {
        snprintf(relative_path, sizeof(relative_path), "%s/%s", directory, entry->d_name);
        if (stat(relative_path, &stat_buf) == 0 && S_ISDIR(stat_buf.st_mode)) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            snprintf(relative_path, sizeof(relative_path), "%s/%s", directory, entry->d_name);
        } else {
            if (strcmp(entry->d_name, relative_file_path) == 0) {
                closedir(dir);
                return 1;
            }
        }
    }

    closedir(dir);
    return 0;
}

log_element* file_exists_in_log(const char *file_path, log_t *logs) {
    log_element *current = logs->head;
    const char *file_name = strrchr(file_path, '/');
    if (!file_name) {
        file_name = file_path;
    } else {
        file_name++;
    }

    while (current != NULL) {
        const char *log_file_name = strrchr(current->path, '/');
        if (!log_file_name) {
            log_file_name = current->path;
        } else {
            log_file_name++;
        }

        if (strcmp(log_file_name, file_name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void get_current_datetime(char *buffer, size_t buffer_size) {
    struct timeval tv;
    struct tm *tm_info;

    // Obtenir l'heure actuelle avec précision jusqu'aux millisecondes
    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);

    // Écrire la date et l'heure au format "YYYY-MM-DD-hh:mm:ss.sss"
    snprintf(buffer, buffer_size, "%04d-%02d-%02d-%02d:%02d:%02d.%03ld",
             tm_info->tm_year + 1900, // Année
             tm_info->tm_mon + 1,    // Mois (de 0 à 11)
             tm_info->tm_mday,       // Jour du mois
             tm_info->tm_hour,       // Heure
             tm_info->tm_min,        // Minute
             tm_info->tm_sec,        // Seconde
             tv.tv_usec / 1000);     // Millisecondes (µsecondes converties)
}

char *calculate_md5(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier");
        return NULL;
    }

    MD5_CTX md5_ctx;
    unsigned char data[1024];
    unsigned char hash[MD5_DIGEST_LENGTH];
    char *md5_string = malloc(MD5_DIGEST_LENGTH * 2 + 1); // 2 caractères par octet + 1 pour '\0'

    if (!md5_string) {
        fprintf(stderr, "Erreur d'allocation mémoire\n");
        fclose(file);
        return NULL;
    }

    MD5_Init(&md5_ctx);

    // Lecture du fichier par blocs et mise à jour du calcul MD5
    size_t bytes_read;
    while ((bytes_read = fread(data, 1, sizeof(data), file)) > 0) {
        MD5_Update(&md5_ctx, data, bytes_read);
    }

    if (ferror(file)) {
        perror("Erreur lors de la lecture du fichier");
        free(md5_string);
        fclose(file);
        return NULL;
    }

    MD5_Final(hash, &md5_ctx);
    fclose(file);

    // Convertir le hash en une chaîne hexadécimale
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(&md5_string[i * 2], "%02x", hash[i]);
    }

    return md5_string;
}

char *extract_from_date(const char *path) {
    const char *pattern = "[0-9]{4}-[0-9]{2}-[0-9]{2}-[0-9]{2}:[0-9]{2}:[0-9]{2}\\.[0-9]{3}";
    regex_t regex;
    regmatch_t match;

    // Compile le pattern
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        fprintf(stderr, "Erreur : Impossible de compiler l'expression régulière.\n");
        return NULL;
    }

    // Cherche le pattern dans le chemin
    if (regexec(&regex, path, 1, &match, 0) == 0) {
        // Trouve la position du début de la date
        size_t start = match.rm_so;
        // Alloue la mémoire pour le résultat
        char *result = strdup(path + start);
        regfree(&regex); // Libère la mémoire du regex
        return result;
    }

    // Si aucun match n'est trouvé
    regfree(&regex);
    return NULL;
}

// Fonction pour convertir un nom de dossier en structure tm (date)
int parse_folder_date(const char *folder_name, struct tm *result) {
    return sscanf(folder_name, "%4d-%2d-%2d-%2d:%2d:%2d.%3d",
                  &result->tm_year, &result->tm_mon, &result->tm_mday,
                  &result->tm_hour, &result->tm_min, &result->tm_sec, &(int){0}) == 7;
}

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

void copy_directory(const char *source_dir, const char *dest_dir, FILE *log, int new) {
    DIR *dir = opendir(source_dir);
    if (!dir) {
        perror("Erreur lors de l'ouverture du répertoire source");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    struct stat statbuf;

    mkdir(dest_dir, 0755);
    if(new==1){
        while ((entry = readdir(dir)) != NULL) {
                printf("Copie de %s/%s\n", dest_dir, entry->d_name);
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                    continue;
                }
                char src_path[PATH_MAX];
                char dest_path[PATH_MAX];
                snprintf(src_path, sizeof(src_path), "%s/%s", source_dir, entry->d_name);
                snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, entry->d_name);
                printf("Copie de %s vers %s\n", src_path, dest_path);
                log_element *elem = malloc(sizeof(log_element));
                char date[64];
                get_current_datetime(date, sizeof(date));
                char *chemin_date = extract_from_date(dest_path);
                elem->path = chemin_date;
                elem->date = date;
                elem->md5 = calculate_md5(src_path);
                write_log_element(elem,log);
                if (stat(src_path, &statbuf) == -1) {
                    perror("Erreur lors de la récupération des informations sur un fichier");
                    continue;
                }

                if (S_ISDIR(statbuf.st_mode)) {
                    copy_directory(src_path, dest_path, log, 0);
                } else {
                    backup_file(src_path, dest_path);
                }
        }
    }else{
        log_t *logs = malloc(sizeof(log_t));
        *logs = read_backup_log(log);
        log_element *current = logs->head;
        if (current == NULL) {
            printf("Aucun élément de log trouvé.\n");
        } else {
            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                    continue;
                }
                char src_path[PATH_MAX];
                char dest_path[PATH_MAX];
                snprintf(src_path, sizeof(src_path), "%s/%s", source_dir, entry->d_name);
                snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, entry->d_name);
                printf("Copie de %s vers %s\n", src_path, dest_path);
                int est_present = file_exists_in_directory(src_path, dest_dir);
                if(est_present){
                    log_element *elem = malloc(sizeof(log_element));
                    char *test = extract_from_date(dest_path);
                    printf("Date : %s\n", test);
                    elem = file_exists_in_log(test, logs);
                    char *md5=calculate_md5(src_path);
                    if(elem==NULL){
                        printf("Le fichier %s n'est pas présent dans %s\n", src_path, dest_dir);
                    }else{
                        if(strcmp(md5,elem->md5)==0){
                            char *chemin_date = extract_from_date(dest_path);
                            elem->path = chemin_date;
                        }else{
                            printf("Le fichier %s a été modifié\n", src_path);
                            char date[64];
                            get_current_datetime(date, sizeof(date));
                            char *chemin_date = extract_from_date(dest_path);
                            elem->path = chemin_date;
                            elem->date = date;
                            elem->md5 = md5;
                            backup_file(src_path, dest_path);
                        }
                    }
                }else{
                    printf("Le fichier %s n'est pas présent dans %s\n", src_path, dest_dir);
                    log_element *elem = malloc(sizeof(log_element));
                    elem->prev = logs->tail;
                    elem->next = NULL;
                    logs->tail->next = elem;
                    logs->tail = elem;
                    char date[64];
                    get_current_datetime(date, sizeof(date));
                    char *chemin_date = extract_from_date(dest_path);
                    char *md5=calculate_md5(src_path);
                    elem->path = chemin_date;
                    elem->date = date;
                    elem->md5 = md5;
                    backup_file(src_path, dest_path);
                }
            }
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
        log = fopen(fichierlog, "a");
        printf("Copie des fichier de : %s dans : %s\n", source_dir, new_backup_dir);
        copy_directory(source_dir, new_backup_dir, log, 1);
    }else{
        log = fopen(fichierlog, "a+");
        char *closest_backup = find_most_recent_folder(backup_dir);
        strcat(backup_dir, "/");
        strcat(backup_dir, closest_backup);
        printf("Restauration de la sauvegarde la plus proche : %s\n", closest_backup);
        copy_directory_link(backup_dir, new_backup_dir);
        copy_directory(source_dir, new_backup_dir, log, 0);
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
void write_backup_file(const char *output_filename, Chunk_list chunks) {
    FILE *file = fopen(output_filename, "wb");//Ouverture du fichier en écriture binaire
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier");
        return;
    }
    Chunk *current = chunks;
    int chunk_count = 0;
    while(current != NULL){ //tant que le chunk courant n'est pas NULL (on est pas arrivés au bout de la liste chaînée)
        chunk_count++;
        char* identificator = (char*)malloc(30*sizeof(char)); //Création de l'identificateur
        if (current->data == NULL){ //Gestion d'erreurs
            printf("Erreur, il n'y a pas de data \n");
        } else {
            if(current->is_unique == 1){//Si le chunk courant est n'est pas unique
                int index;
                memcpy(&index, current->data, sizeof(int));// On copie dans index la valeur de la référence
                sprintf(identificator, "!/(%d)/![*(%d)*]", chunk_count,index);
                size_t ecriture = fwrite(identificator, strlen(identificator) * sizeof(char), 1, file); //On écrit l'index du chunk et la référence dans le fichier
                free(identificator);//On libère la mémoire
                if (ecriture != 1) { //Gestion d'erreurs
                    perror("Erreur lors de l'écriture de l'index et de la référence dans le fichier");
                }
            }
            else {
                int index = 0;//Si le chunk est unique, on écrit l'index du chunk et la data dans le fichier
                sprintf(identificator, "!/(%d)/![*(%d)*]", chunk_count,index);
                size_t ecriture = fwrite(identificator, strlen(identificator) * sizeof(char), 1, file);
                if (ecriture != 1) {//Gestion d'erreurs
                    perror("Erreur lors de l'écriture de l'index et de la référence dans le fichier");
                }
                free(identificator);//On libère la mémoire
                fwrite("\n", sizeof(char), 1, file);
                size_t data = fwrite(current->data, CHUNK_SIZE, 1, file);//On écrit la data dans le fichier
                if (data != 1) {// Gestion d'erreurs
                    perror("Erreur lors de l'écriture de la data dans le fichier");
                }
            }
        }
        fwrite("\n", sizeof(char), 1, file);
        current = current->next;//On passe au chunk suivant
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
    FILE *file = fopen(filename, "rb"); // Ouverture du fichier en lecture binaire
    Md5Entry *hash_table[HASH_TABLE_SIZE] = {NULL}; // Initialisation de la table de hachage
    Chunk_list chunks = NULL; // Initialisation de la liste de chunks

    deduplicate_file(file, &chunks, hash_table);
    write_backup_file(backup_dir, chunks);

    fclose(file);
    free(chunks);
}


/** 
 * @brief Une procédure permettant la restauration du fichier backup via le tableau de chunk
 * 
 * @param output_filename fichier de sortie avec les chunks restorés
 * @param chunks tableau de chunks
 * @param chunk_count nombre de chunks contenus dans le tableau
 */
void write_restored_file(const char *output_filename, Chunk_list chunks) {
    FILE *dest = fopen(output_filename, "wb");//Ouverture du fichier en écriture binaire
    if (!dest){ //Gestion d'erreurs
        fprintf(stderr, "erreur : impossible de créer le fichier %s : %s\n", output_filename, strerror(errno));
        return;
    }
    Chunk *current = chunks;
    while (current != NULL){ //Tant que le chunk courant n'est pas NULL (qu'on est pas arrivés au bout de la liste chaînée)
        if (current->data == NULL){ //Gestion d'erreurs
            printf("Erreur, il n'y a pas de data\n");
        } else {
            size_t data = fwrite(current->data, CHUNK_SIZE, 1, dest); //On écrit la data du chunk dans le fichier
            if (data != 1) {
                perror("Erreur lors de l'écriture de la data dans le fichier");
            }
        }
        current = current->next;//On passe au chunk suivant
    }
    fclose(dest);
    free(chunks);
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