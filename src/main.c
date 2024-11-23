#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "file_handler.h"
#include "deduplication.h"
#include "backup_manager.h"
#include "network.h"


int main(int argc, char *argv[]) {
    // Analyse des arguments de la ligne de commande
	int opt = 0;
    struct option my_opts[] = {
		{.name="backup",.has_arg=0,.flag=0,.val='b'},
		{.name="restore",.has_arg=0,.flag=0,.val='r'},
		{.name="list-backups",.has_arg=0,.flag=0,.val='l'},
        {.name="dry-run",.has_arg=0,.flag=0,.val='u'},
		{.name="d-server",.has_arg=1,.flag=0,.val='e'},
		{.name="d-port",.has_arg=1,.flag=0,.val='p'},
        {.name="s-server",.has_arg=1,.flag=0,.val='a'},
		{.name="s-port",.has_arg=1,.flag=0,.val='t'},
        {.name="dest",.has_arg=1,.flag=0,.val='d'},
		{.name="source",.has_arg=1,.flag=0,.val='s'},
		{.name="verbose",.has_arg=0,.flag=0,.val='v'},
		{.name=0,.has_arg=0,.flag=0,.val=0},
	};
	char *source = ".";
	while ((opt = getopt_long(argc, argv, "", my_opts, NULL)) != -1) {
		switch (opt) {
			case 'b':
				printf("b\n");
				break;

			case 'r':
				printf("r\n");
				break;

			case 'l':
				printf("l\n");
				break;

			case 'u':
				printf("dr\n");
				break;

			case 'e':
				printf("ds\n");
				break;

            case 'p':
				printf("dp\n");
				break;

			case 'a':
				printf("ss\n");
				break;

			case 't':
				printf("sp\n");
				break;

            case 'd':
				printf("d\n");
				break;

			case 's':
				printf("s\n");
				break;

			case 'v':
				printf("v\n");
				break;

			case '?': // Option non reconnue
				fprintf(stderr, "Unknown option encountered.\n");
				exit(EXIT_FAILURE);
		}
	}
    // Implémentation de la logique de sauvegarde et restauration
    // Exemples : gestion des options --backup, --restore, etc.
    char *test = list_files("./src");
	if (test) {
		printf("Fichiers : %s\n", test);
		free(test); // Libérer la mémoire après utilisation
	} else {
		printf("Aucun fichier trouvé ou erreur lors de l'ouverture du répertoire.\n");
	}
    return EXIT_SUCCESS;
}
