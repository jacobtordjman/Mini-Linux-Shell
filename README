## README

# Custom Shell Project

## Overview

This project implements a custom shell in C. The shell supports various functionalities, including command execution, background job management, aliasing, command redirection, logical operators, and script execution. The shell provides a command-line interface for users to interact with the system and perform various operations similar to a standard Unix shell.

## Features

- **Command Execution**: Execute commands and programs by providing the command name and arguments.
- **Background Jobs**: Run commands in the background using the `&` operator and manage them with `jobs` command.
- **Aliases**: Create and manage command aliases using `alias` and `unalias` commands.
- **Command Redirection**: Redirect standard error (`stderr`) output to a file using `2>` operator.
- **Logical Operators**: Handle logical operators `&&` and `||` to execute commands based on the success or failure of previous commands.
- **Script Execution**: Execute shell scripts using the `source` command.
- **Signal Handling**: Proper handling of child process termination using `SIGCHLD`.

## Compilation and Execution

1. **Compilation**:
   ```sh
   gcc -o custom_shell custom_shell.c
   ```

2. **Execution**:
   ```sh
   ./custom_shell
   ```

## Usage

### Basic Command Execution

Enter commands as you would in a standard shell:
```sh
$ ls
$ echo "Hello, World!"
```

### Background Jobs

Run commands in the background using the `&` operator:
```sh
$ sleep 10 &
```

List background jobs using the `jobs` command:
```sh
$ jobs
```

### Aliases

Create an alias:
```sh
$ alias ll="ls -l"
```

Use the alias:
```sh
$ ll
```

Remove an alias:
```sh
$ unalias ll
```

### Command Redirection

Redirect `stderr` output to a file:
```sh
$ command 2> error.log
```

### Logical Operators

Use logical operators to chain commands:
```sh
$ command1 && command2
$ command1 || command2
```

### Script Execution

Execute a shell script:
```sh
$ source script.sh
```

### Exit the Shell

Exit the shell using the `exit_shell` command:
```sh
$ exit_shell
```

## Implementation Details

### Main Function

The `main` function sets up signal handling, enters an infinite loop to read user input, tokenizes the input, handles special commands (`exit_shell`, `source`, `alias`, `unalias`, `jobs`), manages background jobs, and executes commands. It also handles command redirection and logical operators.

### Tokenization

The `tokenize_input` function splits the input string into individual tokens based on delimiters (space, tab, newline, carriage return).

### Alias Management

Alias management functions include `create_alias`, `find_command`, `change_alias`, `print_aliases`, `handle_alias_command`, and `remove_alias`.

### Background Job Management

Background job management functions include `add_job`, `remove_job`, `list_jobs`, and `sigchld_handler`.

### Command Execution

Commands are executed using the `fork` and `execvp` system calls. The shell waits for foreground jobs to complete and adds background jobs to the job list.

### Logical Operators

Logical operators (`&&` and `||`) are handled by splitting the input into separate commands and executing them based on the success or failure of previous commands.

### Command Redirection

Command redirection is handled by the `handle_redirection` function, which redirects `stderr` output to a specified file.

## Error Handling

The shell includes error handling for various operations, including command execution, file opening, memory allocation, and invalid commands. Errors are reported using `perror` and appropriate error messages.

## Conclusion

This custom shell project demonstrates the implementation of a shell-like command-line interface in C with features commonly found in Unix shells. It supports command execution, background jobs, aliases, command redirection, logical operators, and script execution, providing a comprehensive tool for interacting with the system.
