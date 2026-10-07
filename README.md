# Simple Shell Interpreter

A Unix-style shell interpreter written in C, developed as part of my CSC 360 (Operating Systems) coursework at the University of Victoria. This project explores process creation and management, background execution, signal handling, and directory navigation in a Linux environment.

## Features

- Executes commands with an arbitrary number of arguments
- Supports foreground and background process execution
- Tracks active background processes and reports completed processes
- Supports directory navigation with `cd`, including relative paths, `~`, and the home directory
- Handles `SIGINT` (`Ctrl+C`) appropriately for the shell and foreground processes
- Displays a dynamic prompt containing the current user, hostname, and working directory

## Technical Concepts

This project provided hands-on experience with:

- Process creation using `fork()`
- Program execution using `execvp()`
- Process synchronization and monitoring using `waitpid()`
- Unix signal handling with `sigaction()`
- Dynamic memory allocation
- Linked lists for background process tracking
- Linux system calls and process management
- GNU Readline for command-line input

## Building

The project is designed to run in a Linux environment.

Compile using the included Makefile:

```bash
make ssi
