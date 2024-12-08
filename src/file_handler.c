/*                                                                    
FFFFFFFFFFFFFFFFFFFFFF                    iiii           tttt          
F::::::::::::::::::::F                   i::::i       ttt:::t          
F::::::::::::::::::::F                    iiii        t:::::t          
FF::::::FFFFFFFFF::::F                                t:::::t          
  F:::::F       FFFFFF  aaaaaaaaaaaaa   iiiiiii ttttttt:::::ttttttt    
  F:::::F               a::::::::::::a  i:::::i t:::::::::::::::::t    
  F::::::FFFFFFFFFF     aaaaaaaaa:::::a  i::::i t:::::::::::::::::t    
  F:::::::::::::::F              a::::a  i::::i tttttt:::::::tttttt    
  F:::::::::::::::F       aaaaaaa:::::a  i::::i       t:::::t          
  F::::::FFFFFFFFFF     aa::::::::::::a  i::::i       t:::::t          
  F:::::F              a::::aaaa::::::a  i::::i       t:::::t          
  F:::::F             a::::a    a:::::a  i::::i       t:::::t    tttttt
FF:::::::FF           a::::a    a:::::a i::::::i      t::::::tttt:::::t
F::::::::FF           a:::::aaaa::::::a i::::::i      tt::::::::::::::t
F::::::::FF            a::::::::::aa:::ai::::::i        tt:::::::::::tt
FFFFFFFFFFF             aaaaaaaaaa  aaaaiiiiiiii          ttttttttttt  
*/

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

/**
 * @brief Affiche sur la sortie standard les fichiers, fichiers cachés et dossiers situés à l'endroit du chemin passé en paramètre. 
 * 
 * @param path : chemin absolu ou relatif vers un dossier impérativement
 */
void list_files(const char *path) {
    struct dirent *dir;
    DIR *d = opendir(path);
    if (!d) {
        printf("Le chemin passé en option n'est pas valide !\n");
        return;
    }

    while ((dir = readdir(d)) != NULL) {
        // Ignorer les entrées spéciales "." et ".."
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
            continue;
        }

        printf("%s\n", dir->d_name);
    }

    // Fermeture du dossier
    closedir(d);
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

    printf("Fichier copié de %s vers %s\n", src, dest_path);
}

/**
 * @brief Prend en paramètre un chemin absolu ou relatif vers le fichier .backup_log
 *          et retourne une liste doublement chainée avec ses données lues
 * 
 * @param logfile chemin absolu ou relatif vers le fichier .backup_log
 * @return log_t liste doublement chainée avec les données lues dans le fichier
 */
log_t read_backup_log(const char *logfile){
    log_t logs = { .head = NULL, .tail = NULL };
    FILE *file = fopen(logfile, "r");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier .backup_log");
        return logs;
    }

    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        log_element *elt = malloc(sizeof(log_element));
        if (!elt) {
            perror("Erreur d'allocation mémoire");
            fclose(file);
            return logs;
        }

        elt->path = strdup(strtok(line, ","));    // Chemin
        elt->md5 = strdup(strtok(NULL, ","));   // Somme md5
        elt->date = strdup(strtok(NULL, "\n"));  // Date

        elt->next = NULL;
        elt->prev = logs.tail;

        if (logs.tail) {
            logs.tail->next = elt;
        } else {
            logs.head = elt;
        }
        logs.tail = elt;
    }

    fclose(file);
    return logs;
}



/*
EEEEEEEEEEEEEEEEEEEEEE                                                                                                                   
E::::::::::::::::::::E                                                                                                                   
E::::::::::::::::::::E                                                                                                                   
EE::::::EEEEEEEEE::::E                                                                                                                   
  E:::::E       EEEEEEnnnn  nnnnnnnn             cccccccccccccccc   ooooooooooo   uuuuuu    uuuuuu  rrrrr   rrrrrrrrr       ssssssssss   
  E:::::E             n:::nn::::::::nn         cc:::::::::::::::c oo:::::::::::oo u::::u    u::::u  r::::rrr:::::::::r    ss::::::::::s  
  E::::::EEEEEEEEEE   n::::::::::::::nn       c:::::::::::::::::co:::::::::::::::ou::::u    u::::u  r:::::::::::::::::r ss:::::::::::::s 
  E:::::::::::::::E   nn:::::::::::::::n     c:::::::cccccc:::::co:::::ooooo:::::ou::::u    u::::u  rr::::::rrrrr::::::rs::::::ssss:::::s
  E:::::::::::::::E     n:::::nnnn:::::n     c::::::c     ccccccco::::o     o::::ou::::u    u::::u   r:::::r     r:::::r s:::::s  ssssss 
  E::::::EEEEEEEEEE     n::::n    n::::n     c:::::c             o::::o     o::::ou::::u    u::::u   r:::::r     rrrrrrr   s::::::s      
  E:::::E               n::::n    n::::n     c:::::c             o::::o     o::::ou::::u    u::::u   r:::::r                  s::::::s   
  E:::::E       EEEEEE  n::::n    n::::n     c::::::c     ccccccco::::o     o::::ou:::::uuuu:::::u   r:::::r            ssssss   s:::::s 
EE::::::EEEEEEEE:::::E  n::::n    n::::n     c:::::::cccccc:::::co:::::ooooo:::::ou:::::::::::::::uu r:::::r            s:::::ssss::::::s
E::::::::::::::::::::E  n::::n    n::::n      c:::::::::::::::::co:::::::::::::::o u:::::::::::::::u r:::::r            s::::::::::::::s 
E::::::::::::::::::::E  n::::n    n::::n       cc:::::::::::::::c oo:::::::::::oo   uu::::::::uu:::u r:::::r             s:::::::::::ss  
EEEEEEEEEEEEEEEEEEEEEE  nnnnnn    nnnnnn         cccccccccccccccc   ooooooooooo       uuuuuuuu  uuuu rrrrrrr              sssssssssss    
*/





// Fonction permettant de mettre à jour une ligne du fichier .backup_log
void update_backup_log(const char *logfile, log_t *logs){
  /* Implémenter la logique de modification d'une ligne du fichier ".bakcup_log"
  * @param: logfile - le chemin vers le fichier .backup_log
  *         logs - qui est la liste de toutes les lignes du fichier .backup_log sauvegardée dans une structure log_t
  */

}

void write_log_element(log_element *elt, FILE *logfile){
  /* Implémenter la logique pour écrire un élément log de la liste chaînée log_element dans le fichier .backup_log
   * @param: elt - un élément log à écrire sur une ligne
   *         logfile - le chemin du fichier .backup_log
   */
}

