#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "deduplication.h"
#include "backup.h"

int main(){
    const char *filename = "file_1.txt";
    const char *backup_dir = "leress.txt";
    //int index_test;
    //index_test = read_identificator("!/(1)/![*(6)*]");
    //printf("Index : %d\n", index_test);
    backup_file(filename, backup_dir);
    return 0;
}