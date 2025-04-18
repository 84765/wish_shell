#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

char *lshReadLine(void);
void lshExecute(char *line);

int main() {
    char *line = NULL;
    size_t len = 0;

    do {
        printf("wish> ");

        line = lshReadLine();
        
        if (line == NULL) {
            printf("Error reading line\n");
            continue;
        }

        if (strcmp(line, "exit\n") == 0) {
            printf("Goodbye!\n");
            free(line);
            exit(0);
        }

        line[strcspn(line, "\n")] = 0;
        lshExecute(line);

    } while (1);
    
    free(line);   
    return 0;
}

char *lshReadLine(void) {
    char *line = NULL;
    size_t len = 0;

    if (getline(&line, &len, stdin) == -1) {
        if (feof(stdin)) {
            free(line);
            return NULL;

        } else if (ferror(stdin)) {
            perror("getline");
            exit(1);
        }
    }

    return line;
}

void lshExecute(char *line) {
    if (strncmp(line, "cd ", 3) == 0) {
        char *path = line + 3;
        if (chdir(path) != 0) {
            perror("cd");
        }
        return;
    }

    char *args[64];
    char *token = strtok(line, " ");
    int i = 0;

    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        char *args[] = {line, NULL};
        execv(args[0], args);
        perror("execv");
        exit(1);
    } else if (pid > 0) {
        wait(NULL);
    } else {
        perror("fork");
        exit(1);
    }
}
