#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

	char *source = NULL;
	char *dest = NULL;
	char *d_server = NULL;
	char *s_server = NULL;
	int backup=0, restore=0, list_back=0, dry_run=0, verbose=0, d_port = 0, s_port = 0;
	while ((opt = getopt_long(argc, argv, "", my_opts, NULL)) != -1) {
		switch (opt) {
			case 'b':
				backup = 1;
				break;

			case 'r':
				restore = 1;
				break;

			case 'l':
				list_back = 1;
				break;

			case 'u':
				dry_run = 1;
				break;

			case 'e':
				d_server = strdup(optarg);
				break;

            case 'p':
				d_port = atoi(optarg);
				break;

			case 'a':
				s_server = strdup(optarg);
				break;

			case 't':
				s_port = atoi(optarg);
				break;

            case 'd':
				dest = strdup(optarg);
				break;

			case 's':
				source = strdup(optarg);
				break;

			case 'v':
				verbose = 1;
				break;

			case '?': // Option non reconnue
				fprintf(stderr, "Unknown option encountered.\n");
				exit(EXIT_FAILURE);
		}
	}

    // Gestion des options
	printf("Liste option :\n backup : %d\n restore : %d\n list-backups : %d\n dry-run : %d\n d-server : %s\n d-port : %d\n s-server : %s\n s-port : %d\n destination %s\n source %s\n verbose %d\n",backup,restore,list_back,dry_run,d_server,d_port,s_server,s_port,dest,source,verbose);
	if (backup+restore+list_back > 1) {
		fprintf(stderr, "Erreur : plusieurs options choisies\n");
		exit(EXIT_FAILURE);
	} else if(backup == 1) {
		if (source != NULL && dest != NULL) {
			create_backup(source, dest);
		} else {
			fprintf(stderr, "Erreur : source ou/et destination non spécifiées\n");
			exit(EXIT_FAILURE);
		}
	} else if (restore == 1) {
		if (source != NULL && dest != NULL) {
			restore_backup(source, dest);
		} else {
			fprintf(stderr, "Erreur : source ou/et destination non spécifiées\n");
			exit(EXIT_FAILURE);
		}
	} else if (list_back == 1) {
		if (dest != NULL) {
			list_backup(dest, verbose);
		} else {
			fprintf(stderr, "Erreur : destination non spécifiée\n");
			exit(EXIT_FAILURE);
		}
	}
	
    return EXIT_SUCCESS;
}