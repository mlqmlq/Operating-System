#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char** argv) {
    if (argc < 2) {
        printf("wis-grep: searchterm [file ...]\n");
        exit(1);
    }
    char* search_term = argv[1];
    if (argc == 2) {
        char *line = NULL;
        size_t size;
        while (getline(&line, &size, stdin) > -1) {
            if (strstr(line, search_term) != NULL) {
                printf("%s", line);
            }
        } 
        return 0;
    }
    char *filename;
    int i;
    FILE *f;
    char *buf;
    size_t buf_size;
    ssize_t line_size;
    for (i = 2; i <= argc-1; i++)
    {
        filename = argv[i];
        f = fopen(filename, "r");
        if (f == NULL) {
            printf("wis-grep: cannot open file\n");
            exit(1);
        }
        buf = NULL;
        buf_size = 0;
        line_size = getline(&buf, &buf_size, f);
        while (line_size >= 0) {
            if (strstr(buf, search_term) != NULL) {
                printf("%s", buf);
            }
            line_size = getline(&buf, &buf_size, f);
        }
        fclose(f);
    }
    return 0;
}
