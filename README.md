# Simple_C_Shell
This project demonstrates my ability to create a simple Unix shell using the C programming language. The shell supports basic command execution with proper argument handling and child process creation.

Features:

* Reads input from the user and tokenizes the command into arguments.
* Forks a child process and executes the command using execvp().
* Handles basic process management, including process creation and waiting for child processes to finish.
* Supports pipes (|) for chaining commands together, connecting the output of one command to the input of another.