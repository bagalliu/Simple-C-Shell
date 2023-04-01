#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_INPUT_LENGTH 1024
#define TOKEN_DELIMITERS " \t\r\n\a"

char *read_input(void) {
    char *input = NULL;
    ssize_t buffer_size = 0;
    getline(&input, &buffer_size, stdin);
    return input;
}
char **tokenize_input(char *input) {
    int buffer_size = 64;
    int position = 0;
    char **tokens = malloc(buffer_size * sizeof(char *));
    char *token;

    token = strtok(input, TOKEN_DELIMITERS);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= buffer_size) {
            buffer_size += 64;
            tokens = realloc(tokens, buffer_size * sizeof(char *));
        }

        token = strtok(NULL, TOKEN_DELIMITERS);
    }
    tokens[position] = NULL;
    return tokens;
}

int execute_command(char **args) {
    int pipe_position = -1;

    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            pipe_position = i;
            break;
        }
    }

    if (pipe_position != -1) {

        int fd[2];
        if (pipe(fd) == -1) {
            perror("pipe");
            return 1;
        }

        args[pipe_position] = NULL;
        pid_t pid1, pid2;

        pid1 = fork();
        if (pid1 == 0) {
            close(fd[0]);
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);

            if (execvp(args[0], args) == -1) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        } else if (pid1 < 0) {
            perror("fork");
            return 1;
        }

        pid2 = fork();
        if (pid2 == 0) {
            close(fd[1]);
            dup2(fd[0], STDIN_FILENO);
            close(fd[0]);

            if (execvp(args[pipe_position + 1], &args[pipe_position + 1]) == -1) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        } else if (pid2 < 0) {
            perror("fork");
            return 1;
        }

        close(fd[0]);
        close(fd[1]);

        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);
    } else {
        pid_t pid;
        int status;

        pid = fork();
        if (pid == 0) {
            if (execvp(args[0], args) == -1) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        } else if (pid < 0) {
            perror("fork");
            return 1;
        } else {
            do {
                waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }
    }

    return 1;
}


int main(void) {
    char *input;
    char **args;
    int status;

    do {
        printf("> ");
        input = read_input();
        args = tokenize_input(input);
        status = execute_command(args);

        free(input);
        free(args);
    } while (status);

    return EXIT_SUCCESS;
}

