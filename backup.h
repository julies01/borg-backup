#ifndef BACKUP_H
#define BACKUP_H

#include "deduplication.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>

// Fonction permettant la restauration du fichier backup via le tableau de chunk
void write_backup_file(const char *output_filename, Chunk_list chunks);

// Fonction permettant la restauration du fichier backup via le tableau de chunk
void write_restored_file(const char *output_filename, Chunk_list chunks);

// Fonction pour la sauvegarde de fichier dédupliqué
void backup_file(const char *filename, const char *backup_dir);


#endif // BACKUP_H
