//https://brennan.io/2015/01/16/write-a-shell-in-c/
//https://www.geeksforgeeks.org/making-linux-shell-c/
//https://github.com/jmreyes/simple-c-shell/blob/master/simple-c-shell.c
//https://stackoverflow.com/questions/24538470/what-does-dup2-do-in-c
//https://www.geeksforgeeks.org/dup-dup2-linux-system-call/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

char *ReadLine(void);
char *readLineForFile(FILE *inputFile);
void Execute(char *line, const char *shellRoot);
void runSingleCommand(char *line, const char *shellRoot);
void writeOutputToFile(int redirectOutput, const char *outputFile);
void wishCat(int argc, char **argv);
int changeDirectory(char *args[]);
void wishPwd(const char *shellRoot);
void wishHelp();
void wishLs();
void wishLsLa();
void wishPath(char *input);
void wishEcho(int argc, char **argv, int redirect, char *outfile);

char *wishPathList[100];
int wishPathCount = 0;

int main(int argc, char *argv[]) {
    FILE *inputFile = stdin;

    if (argc == 2) {
        inputFile = fopen(argv[1], "r");
        if (inputFile == NULL) {
            perror("Cannot open batch file");
            exit(1);
        }
    } else if (argc > 2) {
        fprintf(stderr, "Usage: %s [batch_file]\n", argv[0]);
        exit(1);
    }

    char *line = NULL;
    size_t len = 0;
    char shellRoot[1024];

    if (getcwd(shellRoot, sizeof(shellRoot)) == NULL) {
        perror("getcwd");
        exit(1);
    }

    do {
        if (inputFile == stdin) {
            printf("wish> ");
            fflush(stdout);
            line = ReadLine();
        } else {
            line = readLineForFile(inputFile);
        }
        
        if (line == NULL) {
            if (feof(inputFile)) {
                printf("Goodbye!\n");
                break;
            } else {
                printf("Error reading line\n");
                continue;
            }
        }

        line[strcspn(line, "\n")] = 0;

        char *lineCopy = strdup(line);
        char *cmd = strtok(lineCopy, " ");
        if (cmd != NULL && strcmp(cmd, "cd") == 0) {
            char *arg = strtok(NULL, " ");
            char *args[] = {cmd, arg, NULL};
            changeDirectory(args);
            free(lineCopy);
            free(line);
            continue;
        }
        free(lineCopy);

        if (strncmp(line, "exit", 4) == 0 && 
            (line[4] == '\0' || line[4] == ' ')) {

                char *temp = strdup(line);
                char *args[10];
                int i = 0;
                char *token = strtok(temp, " ");

                while (token != NULL && i < 10) {
                    args[i++] = token;
                    token = strtok(NULL, " ");
                }

                if (i > 1) {
                    fprintf(stderr, "exit: too many arguments\n");
                    free(temp);
                    free(line);
                    continue;
                }

                free(temp);
                printf("Goodbye!\n");
                free(line);
                exit(0);
            }

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

char *readLineForFile(FILE *inputFile) {
    char *line = NULL;
    size_t len = 0;

    ssize_t read = getline(&line, &len, inputFile);
    if (read == -1) {
        free(line);
        return NULL;
    }

    if (read > 0 && line[read - 1] == '\n') {
        line[read - 1] = '\0';
    }

    if (strstr(line, "exit") != NULL) {
        printf("Goodbye!\n");
        free(line);
        exit(0);
    }

    char *copy = strdup(line);
    free(line);

    return copy;
}

void Execute(char *line, const char *shellRoot) {

    char *command[64];
    int count = 0;
    char *saveptr;
    char *cmd = strtok_r(line, "&", &saveptr);

    while (cmd != NULL && count < 64) {
        command[count++] = cmd;
        cmd = strtok_r(NULL, "&", &saveptr);
    }

    pid_t pids[64];

    for (int i = 0;  i < count; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            runSingleCommand(command[i], shellRoot);
            exit(0);
        } else if (pid > 0) {
            pids[i] = pid;
        } else {
            perror("fork");
        }
    }

    for (int i = 0; i < count; i++) {
        waitpid(pids[i], NULL, 0);
    }
    
    char lineCopy[1024];
    strncpy(lineCopy, line, sizeof(lineCopy));
    lineCopy[sizeof(lineCopy) - 1] = '\0';
}

void runSingleCommand(char *line, const char *shellRoot) {
    char *args[64];
    char *token = strtok(line, " ");
    int i = 0;

    char *outputFile = NULL;
    int redirectOutput = 0;

    while (token != NULL && i < 64) {

        if (strcmp(token, ">") == 0) {
            redirectOutput = 1;
            token = strtok(NULL, " ");
            if (token == NULL) {
                perror("No output file specified");
                return;
            }
            
            printf("Redirecting output to file: %s\n", token);
            
            outputFile = token;
            token = strtok(NULL, " ");
            continue; 
        }

        args[i++] = token;
        token = strtok(NULL, " ");
    } args[i] = NULL;

    if (args[0] == NULL) {
        return;
    }

    // Katso myöhemmin tarpeellisuus
    if (strcmp(args[0], "exit") == 0 && 
        (args[1] == NULL || strcmp(args[1], "") == 0)) {
        printf("Goodbye!\n");
        free(line);
        exit(0);
    }

    if (strcmp(args[0], "cd") == 0) {
        changeDirectory(args);
        return;
    }

    if (strcmp(args[0], "cat") == 0) {
        if (redirectOutput && outputFile != NULL) {
            writeOutputToFile(redirectOutput, outputFile);
        }

        wishCat(i, args);
        return;
    }

    if (strcmp(args[0], "pwd") == 0) {
        if (redirectOutput && outputFile != NULL) {
            writeOutputToFile(redirectOutput, outputFile);
        }

        wishPwd(shellRoot);
        return;
    }

    if (strcmp(args[0], "help") == 0) {
        if (redirectOutput && outputFile != NULL) {
            writeOutputToFile(redirectOutput, outputFile);
        }
        
        wishHelp();
        return;
    }

    if (strcmp(args[0], "ls") == 0 && args[1] != NULL && strcmp(args[1], "-la") == 0) {
        if (redirectOutput && outputFile != NULL) {
            writeOutputToFile(redirectOutput, outputFile);
        }
        
        wishLsLa();
        return;
    }

    if (strcmp(args[0], "ls") == 0) {
        if (redirectOutput && outputFile != NULL) {
            writeOutputToFile(redirectOutput, outputFile);
        }
        
        wishLs();
        return;
    }

    if (strcmp(args[0], "path") == 0) {
        if (redirectOutput && outputFile != NULL) {
            writeOutputToFile(redirectOutput, outputFile);
        }

        char *restOfLine = strchr(line, ' ');
        wishPath(restOfLine ? restOfLine + 1 : NULL);
        return;
    }

    if (strcmp(args[0], "echo") == 0) {
        wishEcho(i, args, redirectOutput, outputFile);
        return;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    } else if (pid == 0) {
        execvp(args[0], args);
        perror("execvp failed");
        exit(1);
    } else {
        wait(NULL);
    }
}

void writeOutputToFile(int redirectOutput, const char *outputFile) {

    int fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    
    if (fd < 0) {
        perror("Cannot open file for writing");
        exit(1);
    }

    dup2(fd, STDOUT_FILENO);
    close(fd);

    return;
}

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

void wishPath(char *input) {

    if (input == NULL || strlen(input) == 0) {
        printf("wish PATH: ");
        if (wishPathCount == 0) {
            printf("(no path set)\n");
        } else {
            for (int i = 0; i < wishPathCount; i++) {
                printf("%s", wishPathList[i]);
                if (i < wishPathCount - 1) printf(":");
            }
            printf("\n");
        }
        return;
    }

    for (int i = 0; i < wishPathCount; i++) {
        free(wishPathList[i]);
    }
    wishPathCount = 0;

    char *token = strtok(input, ":");
    while (token != NULL && wishPathCount < 100) {
        wishPathList[wishPathCount] = strdup(token);
        if (wishPathList[wishPathCount] == NULL) {
            perror("strdup failed");
            exit(1);
        }
        wishPathCount++;
        token = strtok(NULL, ":");
    }

    printf("wish PATH updated\n");
}

void wishEcho(int argc, char **argv, int redirect, char *outfile) {
    if (argc < 2) {
        fprintf(stderr, "echo: expected arguments\n");
        return;
    }


    FILE *output = stdout;

    if (redirect && outfile != NULL) {
        output = fopen(outfile, "w");
        if (output == NULL) {
            perror("Cannot open file for writing");
            return;
        }
    }

    for (int i = 1; i < argc; i++) {
        fprintf(output, "%s", argv[i]);
        if (i < argc - 1) fprintf(output, " ");
    }

    fprintf(output, "\n");

    if (output != stdout) {
        fclose(output);
        printf("Output written to %s\n", outfile);
    }
}



// > kirjoittaa tulosteen halutuun tiedostoon - valmis
// virheen käsittely preammanksi
// path puuttu varsinainen execvp
// (voit käyttää access() + execv() PATH-hakemistoista 
// hakemiseen, kuten projektiohjeessa mainittu)
// exit toimi, mutta batch-tiedostossa se toimii kun sana sisältää vain exit
// tarkista ctrl+D (EOF) -tilanne, se tulostaa sen wish> vaikka ei kuuluisi
// cd virheen käsittely sekoaa .. oletuksessa, sekoittaa kaiken toiminnan
// virheen käsittely jos tiesdotoa ei ole

/*Also test error cases:

cd with no args

exit extra

Invalid paths in path

Unknown commands*/