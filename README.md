# Shell Command-Line Interpreter

## Overview

This project implements a basic shell, a command-line interpreter that allows users to interact with the operating system through a series of commands. The shell supports built-in commands, the execution of external programs, and piping between multiple programs.

### How It Works

1. **Prompting:** The shell displays a prompt, waiting for the user to enter a command.
2. **Interpreting Commands:**
   - If a built-in command (e.g., `cd`) is entered, the shell directly executes this command.
   - If an external program (e.g., `/bin/ls`) is specified, or if multiple programs are connected through pipes (e.g., `ls -l | less`), the shell creates child processes to execute these programs and waits for the processes to terminate or suspend.
   - If an invalid command is entered, the shell displays an error message.
3. **Termination:** The shell continues to operate until you press `Ctrl-D` to close STDIN or enter the `exit` command, which causes the shell to exit.

## Specifications

### Prompt

The format of the prompt in this shell is specifically designed as follows:

- An opening bracket `[`.
- The string `nyush`.
- A whitespace.
- The basename of the current working directory.
- A closing bracket `]`.
- A dollar sign `$`.
- Another whitespace.

This format ensures that at every step, users are clearly aware of their current directory and the context in which commands are being executed.
