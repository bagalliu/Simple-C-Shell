#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <fcntl.h>

#define MAX_INPUT_LENGTH 1024
#define TOKEN_DELIMITERS " \t\r\n\a"

typedef struct Alias {
    char *name;
    char *command;
    struct Alias *next;
} Alias;

Alias *aliases = NULL;

void add_alias(const char *name, const char *command) {
    Alias *new_alias = malloc(sizeof(Alias));
    new_alias->name = strdup(name);
    new_alias->command = malloc(strlen(command) + 1);
    strcpy(new_alias->command, command);
    new_alias->next = aliases;
    aliases = new_alias;
}


void remove_alias(const char *name) {
    Alias *current = aliases;
    Alias *previous = NULL;

    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            if (previous == NULL) {
                aliases = current->next;
            } else {
                previous->next = current->next;
            }
            free(current->name);
            free(current->command);
            free(current);
            return;
        }
        previous = current;
        current = current->next;
    }
}

const char *find_alias(const char *name) {
    Alias *current = aliases;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current->command;
        }
        current = current->next;
    }
    return NULL;
}

char *read_input(void) {
    char *input = readline("> ");
    if (input && *input) {
        add_history(input);
    }
    return input;
}

char **tokenize_input(char *input) {
    int buffer_size = 64;
    int position = 0;
    char **tokens = malloc(buffer_size * sizeof(char *));
    char *token;

    while (*input != '\0') {
        while (isspace(*input)) {
            input++;
        }

        if (*input == '"' || *input == '\'') {
            char quote = *input++;
            token = input;
            while (*input != quote && *input != '\0') {
                input++;
            }
            if (*input == quote) {
                *input++ = '\0';
            }
        } else {
            token = input;
            while (!isspace(*input) && *input != '\0') {
                input++;
            }
            if (*input != '\0') {
                *input++ = '\0';
            }
        }

        const char *alias_command = NULL;
        if (position == 0) {
            alias_command = find_alias(token);
        }
        
        if (alias_command) {
            char *alias_command_copy = strdup(alias_command);
            char *alias_token = strtok(alias_command_copy, TOKEN_DELIMITERS);
            while (alias_token != NULL) {
                tokens[position] = strdup(alias_token);
                position++;

                if (position >= buffer_size) {
                    buffer_size += 64;
                    tokens = realloc(tokens, buffer_size * sizeof(char *));
                }

                alias_token = strtok(NULL, TOKEN_DELIMITERS);
            }
            free(alias_command_copy);
        } else {
            tokens[position] = strdup(token);
            position++;

            if (position >= buffer_size) {
                buffer_size += 64;
                tokens = realloc(tokens, buffer_size * sizeof(char *));
            }
        }
    }

    tokens[position] = NULL;
    return tokens;
}


int handle_builtin(char **args) {
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "cd: expected argument\n");
            return 1;
        } else {
            if (chdir(args[1]) != 0) {
                perror("cd");
                return 1;
            }
            return 1;
        }
    }

    if (strcmp(args[0], "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("getcwd");
        }
        return 1;
    }

    if (strcmp(args[0], "alias") == 0) {
        if (args[1] == NULL) {
            Alias *current = aliases;
            while (current != NULL) {
                printf("%s='%s'\n", current->name, current->command);
                current = current->next;
            }
        } else if (args[2] != NULL) {
            // Combine the rest of the arguments into a single command string
            int command_length = 0;
            for (int i = 2; args[i] != NULL; i++) {
                command_length += strlen(args[i]) + 1;
            }
            char *command = malloc(command_length);
            strcpy(command, args[2]);
            for (int i = 3; args[i] != NULL; i++) {
                strcat(command, " ");
                strcat(command, args[i]);
            }
            add_alias(args[1], command);
            free(command);
        } else {
            fprintf(stderr, "alias: expected argument\n");
        }
        return 1;
    }

    if (strcmp(args[0], "unalias") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "unalias: expected argument\n");
            return 1;
        } else {
            remove_alias(args[1]);
            return 1;
        }
    }

    return 0;
}

int execute_command(char **args) {
    int builtin_status = handle_builtin(args);
    if (builtin_status) {
        return builtin_status;
    }

    char *script_path = args[0];
    int script_length = strlen(script_path);
    if (script_length >= 3 && strcmp(&script_path[script_length - 3], ".sh") == 0) {
        int new_argc = 0;
        while (args[new_argc]) {
            new_argc++;
        }

        char **new_argv = malloc((new_argc + 2) * sizeof(char *));
        new_argv[0] = "/bin/sh";
        for (int i = 0; i < new_argc; i++) {
            new_argv[i + 1] = args[i];
        }
        new_argv[new_argc + 1] = NULL;
        args = new_argv;
    }
    
    int in_fd = -1, out_fd = -1;
    int pipe_position = -1;

    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {
            in_fd = open(args[i + 1], O_RDONLY);
            if (in_fd == -1) {
                perror("open");
                return 1;
            }
            args[i] = NULL;
        } else if (strcmp(args[i], ">") == 0) {
            out_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (out_fd == -1) {
                perror("open");
                return 1;
            }
            args[i] = NULL;
        } else if (strcmp(args[i], "|") == 0) {
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
            if (in_fd != -1) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            if (out_fd != -1) {
                dup2(out_fd, STDOUT_FILENO);
                close(out_fd);
            }

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

char *completion_generator(const char *text, int state);

char **custom_completion(const char *text, int start, int end) {
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, completion_generator);
}

char *completion_generator(const char *text, int state) {
    static int list_index, len;
    char *name;

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    while ((name = rl_filename_completion_function(text, list_index++))) {
        if (strncmp(name, text, len) == 0) {
            return name;
        }
        free(name);
    }

    return NULL;
}

void initialize_readline() {
    rl_attempted_completion_function = custom_completion;
}

int main(void) {
    char *input;
    char **args;
    int status;

    do {
        input = read_input();
        args = tokenize_input(input);
        status = execute_command(args);

        free(input);
        free(args);
    } while (status);

    return EXIT_SUCCESS;
}