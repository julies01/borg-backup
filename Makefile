# Compilateur
CC = gcc

# Options de compilation
CFLAGS = -Wall -Wextra -I./src

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
	$(CC) $(CFLAGS) -o $@ $^

# Compilation des fichiers sources en fichiers objets
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Nettoyage des fichiers générés
clean:
	rm -f $(OBJ) $(TARGET)