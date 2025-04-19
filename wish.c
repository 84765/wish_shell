//https://brennan.io/2015/01/16/write-a-shell-in-c/
//https://www.geeksforgeeks.org/making-linux-shell-c/
//https://github.com/jmreyes/simple-c-shell/blob/master/simple-c-shell.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

char *ReadLine(void);
void Execute(char *line, const char *shellRoot);
void wishCat(int argc, char **argv);
int changeDirectory(char *args[]);
void wishPwd(const char *shellRoot);
void wishHelp();
void wishLs();
void wishLsLa();
//void wishPath(char *input);
void wishEcho(int argc, char **argv);

int main() {
    char *line = NULL;
    size_t len = 0;

    char shellRoot[1024];
    if (getcwd(shellRoot, sizeof(shellRoot)) == NULL) {
        perror("getcwd");
        exit(1);
    }

    do {
        printf("wish> ");

        line = ReadLine();
        
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
        Execute(line, shellRoot);

        free(line);
        line = NULL;

    } while (1);
     
    return 0;
}

char *ReadLine(void) {
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

void Execute(char *line, const char *shellRoot) {

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
        wishPwd(shellRoot);
        return;
    }

    if (strcmp(args[0], "help") == 0) {
        wishHelp();
        return;
    }

    if (strcmp(args[0], "ls") == 0 && args[1] != NULL && strcmp(args[1], "-la") == 0) {
        wishLsLa();
        return;
    }

    if (strcmp(args[0], "ls") == 0) {
        wishLs();
        return;
    }

    /*if (strcmp(args[0], "path") == 0) {
        wishPath(line + 5);
        return;
    }*/

    if (strcmp(args[0], "echo") == 0) {
        wishEcho(i, args);
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

    //runCommand(line);
}

/*void runCommand(char *command) {
    char *args[500];
    char *token = strtok(command, " ");
    int i = 0;

    while (token != NULL && i < 64) {
        args[i++] = token;
        token = strtok(NULL, " ");
    } args[i] = NULL;

    if (execvp(args[0], args) == -1) {
        perror("execvp");
    }
}*/

void wishCat(int argc, char **argv) {

    if (argc < 2) {
        fprintf(stderr, "cat: expected file arguments\n");
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

void wishPwd(const char *shellRoot) {
    char cwd[1024];

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        const char *relative = cwd + strlen(shellRoot);
        if (relative[0] == '\0') {
            printf("/wish\n"); 
        } else {
            printf("/wish%s\n", relative);
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
    printf("  help         - show this help message\n");
    printf("  ls           - list files in current directory\n");
    printf("  ls -la       - list all files in current directory\n");
    printf("  echo [text]  - print text to stdout\n");
    printf("  echo [text] > [file] - write text to file\n");
    printf("  path [dir1:dir2:...] - set PATH variable\n");
    printf("  path         - print current PATH variable\n");
    printf("  path [dir]   - add directory to PATH\n");
}

void wishLs() {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        char *args[] = {"/bin/ls", NULL};
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

void wishLsLa() {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        char *args[] = {"/bin/ls", "-la", NULL};
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

/*void wishPath(char *input) {
    char *pathList[100];
    int pathCount = 0;

    // Pilkotaan syöte ja tallennetaan polut
    char *token = strtok(input, " ");
    while (token != NULL) {
        pathList[pathCount] = strdup(token);
        pathCount++;
        token = strtok(NULL, " ");
    }

    if (pathCount > 0) {
        char newPath[1024] = "";

        for (int i = 0; i < pathCount; i++) {
            strcat(newPath, pathList[i]);
            if (i < pathCount - 1) {
                strcat(newPath, ":");
            }
        }

        if (setenv("PATH", newPath, 1) == -1) {
            perror("setenv failed");
        } else {
            printf("PATH set to: %s\n", newPath);
        }
    } else {
        printf("No paths provided\n");
    }

    for (int i = 0; i < pathCount; i++) {
        free(pathList[i]);
    }
}*/

void wishEcho(int argc, char **argv) {

    if (argc < 2) {
        fprintf(stderr, "echo: expected file arguments\n");
        return;
    }

    if (strcmp(argv[argc - 2], ">") == 0) {
        FILE *file = fopen(argv[argc - 1], "w");
        if (file == NULL) {
            perror("Cannot open file for writing\n");
            return;
        }

        for (int i = 1; i < argc - 2; i++) {
            fprintf(file, "%s ", argv[i]);
        }
        fprintf(file, "\n");

        fclose(file);

        printf("Output written to %s\n", argv[argc - 1]);

    } else {
        for (int i = 1; i < argc; i++) {
            printf("%s ", argv[i]);
        }
        printf("\n");
    }
}