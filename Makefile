# Compilateur
CC = gcc

# Options de compilation
CFLAGS = -Wall -Wextra -I./src -pedantic -std=c11 -O2 -Wno-deprecated-declarations

#Bibliothèque Openssl
LDFLAGS = -lssl -lcrypto

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
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Nettoyage des fichiers générés
clean:
	rm -f $(OBJ) $(TARGET)
