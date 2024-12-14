# Compilateur
CC = gcc

# Options de compilation
CFLAGS = -Wall -Wextra -I./src -pedantic -O2 -g -Wno-deprecated-declarations

# Bibliothèque Openssl
LDFLAGS = -lssl -lcrypto -Wno-deprecated-declarations
# Ce dernier flag permet de ne pas mettre de warn pour les fonctions dépréciées, la fonction
# MD5 n'étant plus sécurisée. Cependant, cette approche n'est viable à long terme, car les fonctions
# dépréciées pourraient être complètement supprimées dans de futures versions d'OpenSSL.



# Liste des fichiers sources
SRC = src/main.c src/file_handler.c src/deduplication.c src/backup_manager.c

# Fichiers objets correspondants
OBJ = $(SRC:.c=.o)

# Nom de l'exécutable
TARGET = lp25_borgbackup

# Règle par défaut
all: $(TARGET)

# Construction de l'exécutable à partir des fichiers objets
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compilation des fichiers sources en fichiers objets
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Nettoyage des fichiers générés
clean:
	rm -f $(OBJ) $(TARGET)

# Nettoyage complet
distclean: clean
	rm -f core dump