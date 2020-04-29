#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#define FILELENGTH 100

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("wis-tar: tar-file file [...]\n");
        exit(1);
    }
    struct stat info;
    char filename[FILELENGTH] = {'\0'};
    FILE *fp;
    FILE *f;
    fp = fopen(argv[1], "w");
    int i;
    int err;
    char *buf;
    //size_t buf_size;
    //ssize_t line_size;
    for (i = 2; i <= argc-1; i++) {
        //Check if the input file is valid.
        err = stat(argv[i], &info);
        if (err == -1) {
            printf("wis-tar: cannot open file\n");
            exit(1);
        }
        strncpy(filename, argv[i], FILELENGTH);
        fwrite(filename, 1, FILELENGTH, fp);
        fwrite(&info.st_size, 8, 1, fp);
        f = fopen(argv[i], "r");
        buf = (char *)malloc(info.st_size);
        fread(buf, 1, info.st_size, f);
        fwrite(buf, 1, info.st_size, fp);
        /*line_size = getline(&buf, &buf_size, f);
        while (line_size >= 0) {
            fwrite(buf, 1, buf_size, fp);
            line_size = getline(&buf, &buf_size, f);
        }*/
        fclose(f);
    }   
    fclose(fp);
    return 0;
}
