#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FILELENGTH 100
#define FILESIZE 8

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("wis-untar: tar-file\n");
        exit(1);
    }
    FILE *f1;
    FILE *f2;
    f1 = fopen(argv[1], "r");
    if (f1 == NULL) {
        printf("wis-untar: cannot open file\n");
        exit(1);
    }
    char *buf;
    buf = (char *)malloc(FILELENGTH);
    long num;
    int temp;
    temp = fread(buf, 1, FILELENGTH, f1);
    while (temp == FILELENGTH) {
        f2 = fopen(buf, "w");
        fread(&num, FILESIZE, 1, f1);
        buf = (char *)realloc(buf, num);
        fread(buf, 1, num, f1);
        fwrite(buf, 1, num, f2);
        fclose(f2);
        buf = (char *)malloc(FILELENGTH);
        temp = fread(buf, 1, FILELENGTH, f1); 
    }
    fclose(f1);

    return 0;
}
