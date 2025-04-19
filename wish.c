//https://brennan.io/2015/01/16/write-a-shell-in-c/
//https://www.geeksforgeeks.org/making-linux-shell-c/
//https://github.com/jmreyes/simple-c-shell/blob/master/simple-c-shell.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

char *lshReadLine(void);
void lshExecute(char *line);
void wishCat(int argc, char **argv);
int changeDirectory(char *args[]);
void wishPwd();
void wishHelp();

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

        free(line);
        line = NULL;

    } while (1);
     
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

    char *args[64];
    char *token = strtok(line, " ");
    int i = 0;

    while (token != NULL && i < 64) {
        args[i++] = token;
        token = strtok(NULL, " ");
    } args[i] = NULL;

    if (args[0] == NULL) {
        return;
    }

    if (strcmp(args[0], "cd") == 0) {
        changeDirectory(args);
        return;
    }

    if (strcmp(args[0], "cat") == 0) {
        wishCat(i, args);
        return;
    }

    if (strcmp(args[0], "pwd") == 0) {
        wishPwd();
        return;
    }

    if (strcmp(args[0], "help") == 0) {
        wishHelp();
        return;
    }

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

void wishCat(int argc, char **argv) {

    if (argc < 2) {
        fprintf(stderr, "cat: expected file arguments\n\n");
        return;
    }

    for (int i = 1; i < argc; i++) {
        FILE *file = fopen(argv[i], "r");
        if (file == NULL) {
            perror("Cannot open file\n");
            continue;
        }

        char *line = NULL;
        size_t len = 0;

        while ((getline(&line, &len, file)) != -1) {
            printf("%s", line);
        }

        free(line);
        fclose(file);
    }
}

int changeDirectory(char *args[]) {

    if (args[1] == NULL) {
        if (chdir(getenv("HOME")) == -1) {
            perror("cd");
            return -1;
        }
        return 1;
    } else {
        if (chdir(args[1]) == -1) {
            printf("%s: no such directory\n", args[1]);
            return -1;
        }
    }

    return 0;
}

void wishPwd() {
    char cwd[1024];
    char *rootPath = "/home/liisa/wish_shell";

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        
        if (strncmp(cwd, rootPath, strlen(rootPath)) == 0) {
            printf("/wish%s\n", cwd + strlen(rootPath)); 
        } else {
            printf("%s\n", cwd);
        }
    } else {
        perror("getcwd() error");
    }
}

void wishHelp() {
    printf("Supported commands:\n");
    printf("  cd [dir]     - change directory\n");
    printf("  cat [file]   - print file contents\n");
    printf("  pwd          - print current directory\n");
    printf("  exit         - exit the shell\n");
}