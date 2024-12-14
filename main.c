#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "deduplication.h"
#include "backup.h"

int main(){
    const char *filename = "file_1.txt";
    const char *backup_dir = "file_petit.txt";
    backup_file(filename, backup_dir);

    /*Chunk_list chunks = NULL;
    FILE * file = fopen(backup_dir,"rb");
    if (file == NULL) {
        perror("Erreur lors de l'ouverture du fichier");
        return 1;
    }
    undeduplicate_file(file,&chunks);
    see_chunk_list(chunks);
    write_restored_file("image_restauree.jpg",chunks);*/

    /*int index_test_1;
    int index_test_2;
    index_test_1 = extract_first_number("!/(8)/![*(6)*]");
    index_test_2 = extract_second_number("!/(3)/![*(6)*]");
    printf("Index 1 : %d Index 2 : %d \n", index_test_1, index_test_2);*/
    
    return 0;
}