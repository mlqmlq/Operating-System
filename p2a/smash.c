#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

const char* DEFAULT_PATH = "/bin";
const int BUFFER_SIZE = 100;

void ERROR_HANDLER(){
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message)); 
}

int redirection(char *words) {
	char *p = strchr(words, '>');
	if (p) {
		int index = (int)(p - words);
		//If there exists more than one '>'
		if (strchr(p+1, '>'))
			return -1;
		else 
			return index;
	}
	else
		return 0;
}

int parse_space(char *line, char **words) {
	char *str = strdup(line);
	char *p = strsep(&str, " ");
	int j = 0;
	char *q;
	while (p) {
		q = strsep(&p, "\t");
		while (q != NULL) {
			if (strlen(q) > 0)
				words[j++] = q;
			q = strsep(&p, "\t"); 
		}
		p = strsep(&str, " ");
	}
	words[j] = NULL;
	return j;
}

int find_path(char **paths, int npaths, char *command){
	int i;
	for (i = npaths-1; i >= 0; i--) {
		char *tmp = strdup(paths[i]);
		char *bin = strcat(strcat(tmp, "/"), command);
		if (!access(bin, X_OK))
		return i;
	}
	return -1;
}

int execute(char *line, int *npaths, char **paths, int mode) {
	//Mode 1 no parallel, mode 2 with parallel.
	int i, j, status = 0;
	//Firstly make sure wwe don't have cases like 'ls&'
	char *tmp = strdup(line);
	line = strsep(&tmp, "&");
	int index = redirection(line);
	char *_args[BUFFER_SIZE], *_files[BUFFER_SIZE];
	int nargs, nfiles;
	if (index == -1) {
		ERROR_HANDLER();
		return -1;
	}
	//If we get one '>', do redirection. No need to worry build-in commands.
	else if (index > 0) {
		char *seg1 = strndup(line, index);
		int saved = dup(1);
		nargs = parse_space(seg1, _args);
		if (nargs == 0)
			return 0;
		nfiles = parse_space(&line[index+1], _files);
		if (nfiles > 1) {
			ERROR_HANDLER();
			return -1;
		}
		status = find_path(paths, *npaths, _args[0]);
		//Couldn't locate the binary file.
		if(status == -1) {
			ERROR_HANDLER();
			return -1;
		} else {
			tmp = strdup(paths[status]);
			char *bin = strcat(strcat(tmp, "/"), _args[0]);
			if (nargs > 0) {
				_args[nargs] = NULL;
				FILE *new_f = fopen(_files[0], "w");
				int newfd = fileno(new_f);
				dup2(newfd, 1);
				dup2(newfd, 2);
			}
			if (mode == 2) {
				execv(bin, _args);
			}
			int rc = fork();
			if (rc < 0) {
				ERROR_HANDLER();
			} else if (rc == 0) { //Child process
				execv(bin, _args);
			}
			else {
				if (wait(NULL) != rc) {
                	ERROR_HANDLER();
					return -1;
            	}
			}
			fflush(stdout);
			//change back to stdout.
			dup2(saved, 1);
			return 0;
		}				
	}
	//No redirection:
	else {
		nargs = parse_space(line, _args);
		if (nargs == 0)
			return 0;
		else if (strcmp(_args[0], "exit") == 0) {
			if (nargs > 1) {
				ERROR_HANDLER();
				return -1;
			}
			exit(0);
		} 
		//Built-in command for 'cd'
		else if (strcmp(_args[0], "cd") == 0) {
			if (chdir(_args[1]) != 0 || nargs != 2) {
				ERROR_HANDLER();
				return -1;
			}
		} 
		//Built-in command for 'path'
		else if (strcmp(_args[0], "path") == 0) {
			if (nargs == 2 && strcmp(_args[1], "clear") == 0){
				for (i = 0; i < *npaths; i++)
					paths[i] = NULL;
				*npaths = 0;
				return 0;
			} else if (nargs == 3 && strcmp(_args[1], "add") == 0) {
					paths[*npaths] = _args[2];
					(*npaths)++;
					return 0;
			} else if (nargs == 3 && strcmp(_args[1], "remove") == 0) {
				for (i = 0; i <= *npaths - 1; i++) {
					if (strcmp(_args[2], paths[i]) == 0){
						for (j = i+1; j < *npaths-1; j++) {
							paths[i] = paths[i+1];
						}
						paths[(*npaths)-1] = NULL;
						(*npaths)--;
						status = 1;
						return 0;
					}
				}
				if (status == 0) {
					ERROR_HANDLER();
					return -1;
				}

			} else {
				ERROR_HANDLER();
				return(-1);
			}
		} 
		//Find bin file from path
		else {
			for (i = (*npaths)-1; i >= 0; i--) {
				char *tmp = strdup(paths[i]);
				char *bin = strcat(strcat(tmp, "/"), _args[0]);
				if (access(bin, X_OK) != 0)
					continue;
				else {
					if (mode == 2) {
						execv(bin, _args);
					}
					int rc = fork();
					if (rc < 0) {
						ERROR_HANDLER();
					} else if (rc == 0) { //Child process
						execv(bin, _args);
					}
					else {
						if (wait(NULL) != rc) {
    	                	ERROR_HANDLER();
    	            	}
						status = 1;
						break;
					}
				}
			}
			if (status == 0) {
				ERROR_HANDLER();
				return -1;
			}
			return 0;
		}
	}
	return 0;
}

int parallel(char *line, int *npaths, char **paths) {
	char *commands[BUFFER_SIZE];
	char *q, *str = strdup(line);
	int j = 0;
	int i;
	char *p = strsep(&str, "&");
	while(p) {
		if (strlen(p) > 0)
			commands[j++] = p;
		p = strsep(&str, "&");
	}
	if (j == 1)
		return 1;
	//prevent the case of 'ls& \t'
	str = strdup(commands[j-1]);
	p = strsep(&str, " ");
	while (p) {
		q = strsep(&p, "\t");
		while (q != NULL) {
			if (strlen(q) > 0) {
				j++;
				break;
			}
			q = strsep(&p, "\t"); 
		}
		p = strsep(&str, " ");
	}
	j--;
	for (i = 0; i < j; i++) {

		int rc = fork();
		if (rc < 0) {
			ERROR_HANDLER();
			printf("I spent so much time for this project!!\n");
			return -1;
		}  else if (rc == 0) { // child (new process)
			execute(commands[i], npaths, paths, 2);
		}
	}
	while (j > 0) {
		int status;
		wait(&status);
		--j;
	}
	return 2;
}


int main (int argc, char **argv) {
	//Batch mode.
	if (argc == 2) {
		FILE *fp;
		char *paths[BUFFER_SIZE];
    	paths[0] = strdup(DEFAULT_PATH);
		if (!(fp = fopen(argv[1], "r")) ){
            ERROR_HANDLER();
            exit(1);
        }
		char *line = NULL;
		size_t size;
		int i, npaths = 1;
		while(getline(&line, &size, fp) > 0) {
			if (line[strlen(line)-1] == '\n')
				line[strlen(line)-1] = 0;
			//Multiple commands
			char *chunk[BUFFER_SIZE];
			int j = 0;
			char *p = strsep(&line, ";");
			while(p) {
				if (strlen(p) > 0)
					chunk[j++] = p;
				p = strsep(&line, "&");
			}
			for (i = 0; i < j; i++) {
				if (parallel(chunk[i], &npaths, paths) == 1)
					execute(chunk[i], &npaths, paths, 1);
			}
			line = NULL;
		}
		exit(0);
	}
	if (argc > 2) {
		ERROR_HANDLER();
		exit(1);
	}
	//Interactive mode.
	int i, npaths = 1;
	char *paths[BUFFER_SIZE];
    paths[0] = strdup(DEFAULT_PATH);
	while(1){ 
		printf("smash> ");
		fflush(stdout);
		char *line = NULL;
		size_t size;
		if(getline(&line, &size, stdin) == -1)
			exit(0);
		if (line[strlen(line)-1] == '\n')
			line[strlen(line)-1] = 0;
		//Multiple commands
		char *chunk[BUFFER_SIZE];
		int j = 0;
		char *p = strsep(&line, ";");
		while(p) {
			if (strlen(p) > 0)
				chunk[j++] = p;
			p = strsep(&line, ";");
		}
		for (i = 0; i < j; i++) {
			if (parallel(chunk[i], &npaths, paths) == 1){
				execute(chunk[i], &npaths, paths, 1);
			}
		}
	}
	return 0;
}
