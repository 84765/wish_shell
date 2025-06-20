// Creators: Annika Hovilehto & Irem Yasar

// AI statement:
// ChatGpt was used to help understanding errors and code better, translation help, explain how to use some of the handling functions.

// - Basic structure and logic of the shell
// 1.   https://brennan.io/2015/01/16/write-a-shell-in-c/
// 2.   https://www.geeksforgeeks.org/making-linux-shell-c/
// 3.   https://github.com/jmreyes/simple-c-shell/blob/master/simple-c-shell.c
// 4.   https://jacksonmowry.github.io/shell.html
//
// - Shell syntax and usage with examples
// 5.   https://www.grymoire.com/Unix/Csh.html  // Basic usage: commands, piping, redirections
//
// - dup2 and file handling
// 6.   https://stackoverflow.com/questions/298510/how-to-get-the-current-directory-in-a-c-program
// 7.   https://www.geeksforgeeks.org/dup-dup2-linux-system-call/
// 8.   https://stackoverflow.com/questions/63484842/using-o-creat-o-wronly-how-to-get-current-position-in-the-file-and-append-data
// 9.   https://stackoverflow.com/questions/24538470/what-does-dup2-do-in-c
//
// - PATH and environment variables
// 10.  https://www.digitalocean.com/community/tutorials/how-to-view-and-update-the-linux-path-environment-variable
// 11.  https://askubuntu.com/questions/9848/what-are-path-and-bin-how-can-i-have-personal-scripts
//
// - String handling functions
// 12.  https://www.w3schools.com/c/ref_string_strncmp.php
// 13.  https://www.geeksforgeeks.org/strcspn-in-c/
// 14.  https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/
// 15.  https://www.geeksforgeeks.org/strchr-in-c/
// 16.  https://www.w3schools.com/c/ref_string_strstr.php
// 17.  https://en.cppreference.com/w/c/language/main_function.html

// - Error handling and checks
// 18.  https://www.geeksforgeeks.org/use-fflushstdin-c/
// 19.  https://www.geeksforgeeks.org/eof-and-feof-in-c/
// 20   https://www.tutorialspoint.com/c_standard_library/c_function_ferror.htm

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>

char *ReadLine(void);
char *readLineForFile(FILE *inputFile);
void Execute(char *line, const char *shellRoot);
void runSingleCommand(char *line, const char *shellRoot);
char *searchExecutable(const char *command);
void writeOutputToFile(int redirectOutput, const char *outputFile);
void wishCat(int argc, char **argv);
int changeDirectory(char *args[], const char *shellRoot);
void wishPwd(const char *shellRoot);
void wishHelp();
void wishLs();
void wishLsLa();
void wishPath(char *input);
void wishEcho(int argc, char **argv, int redirect, char *outfile);

char *wishPathList[100];
int wishPathCount = 0;
int path_modified = 0;

// Clears the path list and resets the count
void clear_path_list() {
    for (int i = 0; i < wishPathCount; i++) {
        free(wishPathList[i]);
        wishPathList[i] = NULL;
    } 
    wishPathCount = 0;
}

// Checks if the given path is directory
int is_path_directory(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        perror("stat");
        return 0;
    }
    return S_ISDIR(statbuf.st_mode);
}

// Main function to initialize the shell and handle input
int main(int argc, char *argv[]) {

    wishPathList[wishPathCount++] = strdup("/bin");
    wishPathList[wishPathCount++] = strdup("/usr/bin");

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
                printf("\nGoodbye!\n");
                break;
            }
            else {
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
            changeDirectory(args, shellRoot);
            free(lineCopy);
            free(line);
            continue;
        } free(lineCopy);

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

        if (strncmp(line, "path", 4) == 0 &&
            (line[4] == '\0' || line[4] == ' ')) {

            wishPath(strdup(line));
            free(line);

            continue;
        }

        Execute(line, shellRoot);

        free(line);
        line = NULL;

    } while (1);

    return 0;
}

// used source: 1
// Reads a line from standard input and returns a dynamically allocated string
char *ReadLine(void) {
    char *line = NULL;
    size_t len = 0;

    if (getline(&line, &len, stdin) == -1) {
        if (feof(stdin)) {
            free(line);
            return NULL;
        }
        else if (ferror(stdin)) {
            perror("getline");
            exit(1);
        }
    }

    return line;
}

// Reads a line from a file (batch.txt) and returns a copy of the line
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

// Executes a command line, splitting it by '&' and running each command in parallel
// Each command is run in a separate child process
// used sources: 1, 4.
void Execute(char *line, const char *shellRoot) {

    char lineCopy[1024];
    strncpy(lineCopy, line, sizeof(lineCopy));
    lineCopy[sizeof(lineCopy) - 1] = '\0';

    char *command[64];
    int count = 0;
    char *saveptr;
    char *cmd = strtok_r(line, "&", &saveptr);

    while (cmd != NULL && count < 64) {
        command[count++] = cmd;
        cmd = strtok_r(NULL, "&", &saveptr);
    }

    pid_t pids[64];

    for (int i = 0; i < count; i++) {
        pid_t pid = fork();
        if (pid == 0)
        {
            runSingleCommand(command[i], shellRoot);
            exit(0);
        } else if (pid > 0) {
            pids[i] = pid;
        } else {
            perror("fork");
            continue;
        }
    }

    for (int i = 0; i < count; i++) {
        waitpid(pids[i], NULL, 0);
    }
}

// Runs a single command, handling built-in commands like cd, cat, pwd, help, ls, echo
void runSingleCommand(char *line, const char *shellRoot) {
    char lineOrginal[1024];
    strncpy(lineOrginal, line, sizeof(lineOrginal));
    lineOrginal[sizeof(lineOrginal) - 1] = '\0';

    char *args[64];
    char *token = strtok(line, " ");
    int i = 0;

    char *outputFile = NULL;
    int redirectOutput = 0;

    // Check for output redirection '>'
    while (token != NULL && i < 64) {

        if (strcmp(token, ">") == 0) {
            redirectOutput = 1;
            token = strtok(NULL, " ");

            if (token == NULL) {
                fprintf(stderr, "Syntax error: expected output file after '>'\n");
                return;
            }


            outputFile = token;
            token = strtok(NULL, " ");

            if (token != NULL) {
                fprintf(stderr, "Redirection misformatted\n");
                return;
            }

            continue;
        }

        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    if (args[0] == NULL) {
        return;
    }

    if (strcmp(args[0], "cd") == 0){
        changeDirectory(args, shellRoot);
        return;
    }

    if (strcmp(args[0], "cat") == 0) {
        // If output redirection is specified, write to the file
        if (redirectOutput && outputFile != NULL) {
            writeOutputToFile(redirectOutput, outputFile);
        }

        wishCat(i, args);
        return;
    }

    if (strcmp(args[0], "pwd") == 0) {
        if (i > 1) {
            fprintf(stderr, "pwd: too many arguments\n");
            return;
        }

        if (redirectOutput && outputFile != NULL) {
            writeOutputToFile(redirectOutput, outputFile);
        }

        wishPwd(shellRoot);
        return;
    }

    if (strcmp(args[0], "help") == 0) {
        if (i > 1) {
            fprintf(stderr, "help: too many arguments\n");
            return;
        }

        if (redirectOutput && outputFile != NULL) {
            writeOutputToFile(redirectOutput, outputFile);
        }

        wishHelp();
        return;
    }

    // handle ls and ls -la commands
    if (strcmp(args[0], "ls") == 0) {
        if (args[1] != NULL && strcmp(args[1], "-la") != 0) {
            fprintf(stderr, "ls: unsupported argument '%s'\n", args[1]);
            return;
        }

        if (redirectOutput && outputFile != NULL) {
            writeOutputToFile(redirectOutput, outputFile);
        }

        if (args[1] != NULL && strcmp(args[1], "-la") == 0) {
            if (i > 2) {
                fprintf(stderr, "ls -la: too many arguments\n");
                return;
            } 
            wishLsLa();
        } else {
            wishLs();
        }
        return;
    }


    if (strcmp(args[0], "echo") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "echo: expected arguments\n");
            return;
        } else {
            wishEcho(i, args, redirectOutput, outputFile);
        }
        return;
    }

    // If the command is not a built-in command, search for the executable in PATH
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    } else if (pid == 0) {
        char *path = searchExecutable(args[0]);
        if (path) {
            execv(path, args);
            perror("execv failed");
        } else {
            fprintf(stderr, "%s: command not found\n", args[0]);
        } 
        exit(1);
    } else {
        wait(NULL);
    }
}

// Searches for the executable in the PATH directories
char *searchExecutable(const char *command) {

    static char fullPath[1024];

    for (int i = 0; i < wishPathCount; i++) {
        snprintf(fullPath, sizeof(fullPath), "%s/%s", wishPathList[i], command);
        if (access(fullPath, X_OK) == 0) {
            return fullPath;
        }
    }

    return NULL;
}

// Redirects the output to the specified file
// If the file does not exist, it will be created
// used sources: 6, 7, 8, 9.
void writeOutputToFile(int redirectOutput, const char *outputFile) {

    int fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd < 0) {
        perror("Cannot open file for writing");
        exit(1);
    }

    printf("Output written to file: %s\n", outputFile);

    dup2(fd, STDOUT_FILENO);
    close(fd);

    return;
}

// Prints the contents of the specified files
void wishCat(int argc, char **argv) {

    char *path = searchExecutable("cat");
    if (path == NULL) {
        fprintf(stderr, "cat: command not found\n");
        return;
    }

    if (argc < 2) {
        fprintf(stderr, "cat: expected file arguments\n");
        return;
    }

    for (int i = 1; i < argc; i++) {
        FILE *file = fopen(argv[i], "r");
        if (file == NULL) {
            perror("Cannot open file");
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

// Changes the current directory to the specified directory
int changeDirectory(char *args[], const char *shellRoot) {

    char cwd[1024];

    // Get the current working directory
    if (getcwd(cwd, sizeof(cwd)) == NULL){
        perror("getcwd");
        return -1;
    }

    // If no directory is specified, change to the home directory
    if (args[1] == NULL) {
        fprintf(stderr, "cd: no directory specified\n");
        return -1;
    }

    // If no directory is specified, change to the home directory
    if (args[1] == NULL) {
        if (chdir(getenv("HOME")) == -1) {
            perror("cd");
            return -1;
        }
        return 1;
    }

    // If the specified directory is "..", change to the parent directory
    if (strcmp(args[1], "..") == 0) {
        if (strcmp(cwd, shellRoot) == 0) {
            printf("Already at root directory\n");
            return 0;
        }
    }

    if (chdir(args[1]) == -1) {
        printf("%s: no such directory\n", args[1]);
        return -1;
    }

    return 0;
}

// Prints the current working directory relative to the shell root
void wishPwd(const char *shellRoot) {
    char cwd[1024];

    char *path = searchExecutable("pwd");
    if (path == NULL) {
        fprintf(stderr, "pwd: command not found\n");
        return;
    }

    // Get the current working directory
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        const char *relative = cwd + strlen(shellRoot);
        if (relative[0] == '\0'){
            printf("/wish\n");
        } else {
            printf("/wish%s\n", relative);
        }
    } else {
        perror("getcwd() error");
    }
}

// Prints help message with supported commands
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
    printf("  path [dirs]  - set or print PATH directories\n");
    printf("  > [file]     - redirect output to file\n");
    printf("  &            - run commands in parallel\n");
    printf("  Note: 'wish' is the name of this shell.\n");
}

// Lists all files in the current directory
void wishLs() {

    char *path = searchExecutable("ls");
    if (path == NULL) {
        fprintf(stderr, "ls: command not found\n");
        return;
    }

    // Fork a new process to run the ls command
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        char *args[] = {path, NULL};
        execv(path, args);
        perror("execv");
        exit(1);
    } else {
        wait(NULL);
    }
}

// Lists all files in the current directory with detailed information
void wishLsLa() {

    char *path = searchExecutable("ls");
    if (path == NULL) {
        fprintf(stderr, "ls -la: command not found\n");
        return;
    }

    // Fork a new process to run the ls -la command
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        char *args[] = {path, "-la", NULL};
        execv(path, args);
        perror("execv");
        exit(1);
    } else {
        wait(NULL);
    }
}

// Sets or prints the PATH directories
// If no arguments are given, prints the current PATH
// If arguments are given, updates the PATH with the specified directories
// If a directory is not valid, it will print a warning
// used sources: 10, 11.
void wishPath(char *input) {
    char *args = strchr(input, ' ');

    path_modified += 1;

    if (path_modified == 1) {
        clear_path_list();
    }

    // If no arguments are given, print the current PATH
    if (args == NULL) {
        if (wishPathCount == 0) {
            printf("PATH is empty\n");
            return;
        }

        for (int i = 0; i < wishPathCount; i++) {
            printf("%s", wishPathList[i]);
            if (i < wishPathCount - 1) {
                printf(":");
            }
        }

        printf("\n");
        return;
    }

    args++;

    // Check if the input is empty after 'path '
    char *pathsCopy = strdup(args);
    if (pathsCopy == NULL) {
        perror("strdup failed");
        return;
    }

    // If the input is empty, clear the path list
    for (int i = 0; i < wishPathCount; i++) {
        free(wishPathList[i]);
        wishPathList[i] = NULL;
    }
    wishPathCount = 0;

    // Split the input by ':' to get individual paths
    char *subtoken = strtok(pathsCopy, ":");
    while (subtoken != NULL) {
        if (is_path_directory(subtoken)) {
            wishPathList[wishPathCount++] = strdup(subtoken);
        } else {
            fprintf(stderr, "Warning: '%s' is not a valid directory\n", subtoken);
        } 
        subtoken = strtok(NULL, ":");
    }

    free(pathsCopy);

    path_modified = 2;
}

// Prints the arguments passed to echo command
// If output redirection is specified, writes to the specified file
void wishEcho(int argc, char **argv, int redirect, char *outfile) {

    // Check if the first argument is "echo"
    char *path = searchExecutable("echo");
    if (path == NULL) {
        fprintf(stderr, "echo: command not found\n");
        return;
    }

    FILE *output = stdout;
    int end = argc;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], ">") == 0) {
            end = i;
            break;
        }
    }

    // Open the file for writing
    if (redirect && outfile != NULL) {
        output = fopen(outfile, "w");
        if (output == NULL) {
            perror("Cannot open file for writing");
            return;
        }
    }

    // Print the arguments to the output
    for (int i = 1; i < end; i++) {
        fprintf(output, "%s", argv[i]);
        if (i < end - 1) {
            fprintf(output, " ");
        }
    }

    fprintf(output, "\n");

    if (output != stdout) {
        printf("Output written to file: %s\n", outfile);
        fclose(output);
    }
}
