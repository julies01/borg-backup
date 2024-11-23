#include <dirent.h>
#include "file_handler.h"

char *list_files(const char *path) {
    struct dirent *dir;
    DIR *d = opendir(path);
    if (!d) {
        printf("Le chemin passé en option n'est pas valide !\n");
        return NULL;
    }

    // Buffer initial pour stocker les noms de fichiers
    size_t buffer_size = 1024;
    char *result = malloc(buffer_size);
    if (!result) {
        perror("Erreur d'allocation mémoire");
        closedir(d);
        return NULL;
    }
    result[0] = '\0'; // Initialise la chaîne de caractères

    while ((dir = readdir(d)) != NULL) {
        // Ignorer les entrées spéciales "." et ".."
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
            continue;
        }

        // Calculer la taille requise pour le nouveau nom + la virgule
        size_t new_entry_length = strlen(dir->d_name) + 2; // 1 pour la virgule, 1 pour '\0'
        if (strlen(result) + new_entry_length >= buffer_size) {
            // Étendre la mémoire si nécessaire
            buffer_size *= 2;
            char *new_result = realloc(result, buffer_size);
            if (!new_result) {
                perror("Erreur d'allocation mémoire");
                free(result);
                closedir(d);
                return NULL;
            }
            result = new_result;
        }

        // Ajouter le nom de fichier à la chaîne de résultat
        strcat(result, dir->d_name);
        strcat(result, ",");
    }

    closedir(d);

    // Supprimer la dernière virgule, si elle existe
    size_t len = strlen(result);
    if (len > 0 && result[len - 1] == ',') {
        result[len - 1] = '\0';
    }

    return result;
}

char *read_file(const char *filepath, size_t *size) {
    FILE *file = fopen(filepath, "rb"); // Ouvrir le fichier en mode binaire
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier");
        return NULL;
    }

    // Se déplacer à la fin du fichier pour obtenir sa taille
    if (fseek(file, 0, SEEK_END) != 0) {
        perror("Erreur avec fseek");
        fclose(file);
        return NULL;
    }

    long file_size = ftell(file); // Obtenir la position actuelle (fin du fichier)
    if (file_size < 0) {
        perror("Erreur avec ftell");
        fclose(file);
        return NULL;
    }

    *size = (size_t)file_size; // Stocker la taille dans le pointeur fourni

    // Revenir au début du fichier
    rewind(file);

    // Allouer la mémoire pour lire le contenu du fichier
    char *content = (char *)malloc(*size + 1); // +1 pour le caractère de fin de chaîne
    if (!content) {
        perror("Erreur d'allocation mémoire");
        fclose(file);
        return NULL;
    }

    // Lire le fichier dans le buffer
    size_t read_size = fread(content, 1, *size, file);
    if (read_size != *size) {
        perror("Erreur de lecture du fichier");
        free(content);
        fclose(file);
        return NULL;
    }

    content[*size] = '\0'; // Ajouter le caractère nul pour une chaîne valide

    fclose(file); // Fermer le fichier
    return content;
}

void write_file(const char *filepath, const void *data, size_t size) {
    // Ouvrir le fichier en mode ajout binaire
    FILE *file = fopen(filepath, "ab");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier");
        return;
    }

    // Écrire les données dans le fichier
    size_t written_size = fwrite(data, 1, size, file);
    if (written_size != size) {
        perror("Erreur lors de l'écriture dans le fichier");
    }

    fclose(file); // Fermer le fichier
}
