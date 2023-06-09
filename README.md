# Simple C Shell
This project demonstrates my ability to create a simple Unix shell using the C programming language. The shell supports basic command execution with proper argument handling and child process creation.

## Features:

* Reads input from the user and tokenizes the command into arguments.
* Forks a child process and executes the command using execvp().
* Handles basic process management, including process creation and waiting for child processes to finish.
* Supports pipes (|) for chaining commands together, connecting the output of one command to the input of another.
* Implements command history with arrow key navigation and tab-completion for commands and file paths using the readline library.
* Supports input/output redirection using < and > symbols, allowing users to redirect the output of a command to a file or read input from a file.
* Allows users to execute shell scripts directly without specifying an interpreter.
* Provides alias functionality to create shortcuts for frequently used commands, as well as managing and removing aliases using the alias and unalias built-in commands.
* Implements built-in commands such as cd, pwd, alias, and unalias.

## How to compile and run

1. Install the readline library, if not already installed. On Ubuntu or other Debian-based systems, you can use the following command:

```
sudo apt-get install libreadline-dev
```

2. Compile the simple_shell.c file using the following command:

```
gcc -o simple_shell simple_shell.c -lreadline
```

3. Run the compiled shell:

```
./simple_shell
```
