#include "backup_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

void create_backup(const char *source, const char *destination) {
    // Implémenter la logique de création d'une sauvegarde
}

void restore_backup(const char *backup_id, const char *destination) {
    // Implémenter la logique de restauration d'une sauvegarde
}

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