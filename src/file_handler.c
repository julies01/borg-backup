#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include "file_handler.h"

#define BUFFER_SIZE 4096

int verbose = 0;

/**
 * @brief Affiche sur la sortie standard les fichiers, fichiers cachés et dossiers situés à l'endroit du chemin passé en paramètre. 
 * 
 * @param path : chemin absolu ou relatif vers un dossier impérativement
 */
char **list_files(const char *path, int *count) {
    struct dirent *dir;
    DIR *d = opendir(path);

    if (!d) {
        printf("Le chemin passé en option n'est pas valide !\n");
        *count = 0; // Aucun fichier trouvé
        return NULL;
    }

    char **file_list = NULL;
    *count = 0; // Initialisation du compteur de fichiers

    while ((dir = readdir(d)) != NULL) {
        // Ignorer les entrées spéciales "." et ".."
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
            continue;
        }

        // Allouer de l'espace pour le nouveau fichier
        file_list = realloc(file_list, (*count + 1) * sizeof(char *));
        if (!file_list) {
            printf("Erreur d'allocation mémoire !\n");
            closedir(d);
            return NULL;
        }

        // Allouer de l'espace pour le nom du fichier
        file_list[*count] = strdup(dir->d_name);
        if (!file_list[*count]) {
            printf("Erreur d'allocation mémoire pour le fichier !\n");
            closedir(d);
            for (int i = 0; i < *count; i++) {
                free(file_list[i]);
            }
            free(file_list);
            return NULL;
        }

        (*count)++; // Augmenter le compteur
    }

    closedir(d); // Fermer le dossier

    if (verbose == 1) {
        printf("Mode verbose activé. Liste des fichiers trouvés :\n");
        for (int i = 0; i < *count; i++) {
            printf("%s\n", file_list[i]);
        }
    }
    return file_list; // Retourner le tableau des fichiers
}

/**
 * @brief copie un fichier SOURCE (src) dans un dossier DESTINATION (dest)
 * 
 * @param src chamin absolu ou relatif vers un FICHIER (source)
 * @param dest chamin absolu ou relatif vers un DOSSIER (destination) sans le dernier "/"
 */
void copy_file(const char *src, const char *dest) {
    int src_fd, dest_fd;
    ssize_t bytes_read, bytes_written;
    char buffer[BUFFER_SIZE];
    char dest_path[4096];
    struct stat dest_stat;

    // Ouvrir le fichier source en lecture
    src_fd = open(src, O_RDONLY);
    if (src_fd < 0) {
        perror("Erreur lors de l'ouverture du fichier source");
        return;
    }

    // Vérifier si la destination est un dossier
    if (stat(dest, &dest_stat) == 0 && S_ISDIR(dest_stat.st_mode)) {
        // Construire le chemin complet vers le fichier de destination
        const char *filename = strrchr(src, '/');
        if (filename == NULL) {
            filename = src; // Pas de slash, utiliser le nom complet
        } else {
            filename++; // Ignorer le '/'
        }

        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest, filename);
    } else {
        strncpy(dest_path, dest, sizeof(dest_path) - 1);
        dest_path[sizeof(dest_path) - 1] = '\0'; // Sécurité
    }

    // Ouvrir le fichier destination en écriture (création s'il n'existe pas)
    dest_fd = open(dest_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd < 0) {
        perror("Erreur lors de l'ouverture du fichier de destination");
        close(src_fd);
        return;
    }

    // Copier le contenu du fichier source vers la destination
    while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written < 0) {
            perror("Erreur lors de l'écriture dans le fichier de destination");
            close(src_fd);
            close(dest_fd);
            return;
        }
    }

    if (bytes_read < 0) {
        perror("Erreur lors de la lecture du fichier source");
    }

    // Fermer les fichiers
    close(src_fd);
    close(dest_fd);

    if (verbose) {
        printf("Copie du fichier de %s vers %s terminée avec succès.\n", src, dest_path);
    }
}

/**
 * @brief Prend en paramètre un chemin absolu ou relatif vers le fichier .backup_log
 *          et retourne une liste doublement chainée avec ses données lues
 * 
 * @param logfile chemin absolu ou relatif vers le fichier .backup_log
 * @return log_t liste doublement chainée avec les données lues dans le fichier
 */
log_t read_backup_log(FILE *file) {
    log_t logs = {NULL};

    if (!file) {
        perror("Erreur lors de l'ouverture du fichier de log");
        return logs;
    }

    // Repositionner le pointeur de fichier au début
    rewind(file);
    char line[512];
    while (fgets(line, sizeof(line), file)) {
        log_element *elem = malloc(sizeof(log_element));
        if (!elem) {
            fprintf(stderr, "Erreur d'allocation mémoire\n");
            return logs;
        }

        elem->path = malloc(256);
        elem->date = malloc(256);
        elem->md5 = malloc(256);
        if (!elem->path || !elem->date || !elem->md5) {
            fprintf(stderr, "Erreur d'allocation mémoire\n");
            free(elem->path);
            free(elem->date);
            free(elem->md5);
            free(elem);
            return logs;
        }

        sscanf(line, "%255[^;];%255[^;];%255s", elem->path, elem->md5, elem->date);
        elem->next = logs.head;
        logs.head = elem;
    }
    log_element *elem = logs.head;
    while (elem->next != NULL) {
        elem = elem->next;
    }
    logs.tail = elem;
    return logs;
}
 
/**
 * @brief Fonction ajoutant une ligne à une structure log_t 
 *          ouverte avec un fichier .backup_log 
 * 
 * @param new_line la nouvelle ligne à insérer dans la structure logs au 
 *          format /chemin/vers/le/fichier,somme_md5,date
 * @param logs la structure chainée des logs initialisée avec read_backup_log
 */
void update_backup_log(const char *new_line, log_t *logs) {
    if (!new_line || !logs) {
        fprintf(stderr, "Paramètres invalides pour update_backup_log\n");
        return;
    }

    char *new_path = strdup(strtok((char *)new_line, ";"));  // Chemin
    char *new_md5 = strdup(strtok(NULL, ";"));               // Somme md5
    char *new_date = strdup(strtok(NULL, "\n"));             // Date

    if (!new_path || !new_md5 || !new_date) {
        fprintf(stderr, "Erreur en découpant la nouvelle ligne\n");
        free(new_path);
        free(new_md5);
        free(new_date);
        return;
    }

    log_element *current = logs->head;
    while (current) {
        if (strcmp(current->path, new_path) == 0) {
            // Mise à jour de l'entrée existante, pas besoin de créer une nouvelle ligne
            free(current->md5);
            free(current->date);
            current->md5 = new_md5;
            current->date = new_date;

            free(new_path); // `new_path` n'est pas utilisé dans ce cas
            return;
        }
        current = current->next;
    }

    // Si le chemin n'existe pas encore, ajouter un nouvel élément
    log_element *new_elt = malloc(sizeof(log_element));
    if (!new_elt) {
        perror("Erreur d'allocation mémoire");
        free(new_path);
        free(new_md5);
        free(new_date);
        return;
    }

    new_elt->path = new_path;
    new_elt->md5 = new_md5;
    new_elt->date = new_date;

    new_elt->next = NULL;
    new_elt->prev = logs->tail;

    if (logs->tail) {
        logs->tail->next = new_elt;
    } else {
        logs->head = new_elt;
    }

    logs->tail = new_elt;
    if (verbose) {
        printf("Ajout/Mise à jour de l'élément : Chemin = %s, MD5 = %s, Date = %s\n", new_elt->path, new_elt->md5, new_elt->date);
    }
}

/**
 * @brief Fonction qui prend un element d'une liste chainée de type log_t et qui 
 *          l'écrit dans le fichier logfile (.backup_log)
 * 
 * @param elt un élément log_element à écrire sur une ligne
 * @param logfile le chemin du fichier .backup_log
 */
void write_log_element(log_element *elt, FILE *logfile) {
    if (!elt || !logfile) {
        fprintf(stderr, "Paramètres invalides pour write_log_element\n");
        return;
    }

    // Écriture des données au format spécifié : chemin,somme_md5,date\n
    if (fprintf(logfile, "%s;%s;%s\n", elt->path, elt->md5, elt->date) < 0) {
        perror("Erreur lors de l'écriture dans le fichier");
    }

    if (verbose) {
        printf("Écriture de l'élément dans le fichier log : Chemin = %s, MD5 = %s, Date = %s\n", elt->path, elt->md5, elt->date);
    }
}
