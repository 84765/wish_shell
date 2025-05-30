        /*printf("Executing command: %s\n", args[0]);
        for (int j = 0; j < i; j++) {
            printf("arg[%d]: %s\n", j, args[j]);
            if (strcmp(args[j], ">") == 0) {
                printf("Redirecting output to file: %s\n", args[j + 1]);

                if (args[j + 1] == NULL) {
                    fprintf(stderr, "Error: No output file specified\n");
                    exit(1);
                }

                int fd = open(args[j + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                
                if (fd < 0) {
                    perror("open");
                    exit(1);
                }

                dup2(fd, STDOUT_FILENO);
                close(fd);
                args[j] = NULL;
                break;

            }
        }*/