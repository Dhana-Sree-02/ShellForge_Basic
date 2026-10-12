#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parser.h"

/*
 * Execute one command.
 *
 * Built-in commands are executed directly
 * by the shell process.
 *
 * External commands are executed using
 * fork() and execvp().
 */
int execute_command(command_t *cmd);

#endif
