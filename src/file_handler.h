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

#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <openssl/md5.h>

// Structure pour une ligne du fichier log
typedef struct log_element {
    char *path; // Chemin du fichier/dossier
    char *md5; // MD5 du fichier dédupliqué, le considérer comme une chaine de caracter est suffisant
    char *date; // Date de dernière modification
    struct log_element *next;
    struct log_element *prev;
} log_element;

// Structure pour une liste de log représentant le contenu du fichier backup_log
typedef struct {
    log_element *head; // Début de la liste de log 
    log_element *tail; // Fin de la liste de log
} log_t;

char **list_files(const char *path, int *count);
void copy_file(const char *src, const char *dest);
log_t read_backup_log(const char *logfile);

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

/*
// fonctions de l'ancienne version du sujet

// char *list_files(const char *path); // prototype modifié dans la nouvelle version
char *read_file(const char *filepath, size_t *size);
void write_file(const char *filepath, const void *data, size_t size);





// fonctions de la nouvelle version du sujet


void update_backup_log(const char *logfile, log_t *logs);
void write_log_element(log_element *elt, FILE *logfile);

*/

#endif // FILE_HANDLER_H
