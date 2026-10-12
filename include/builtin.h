#ifndef BUILTIN_H
#define BUILTIN_H

#include "parser.h"

/*
 * Check whether the command is a built-in.
 *
 * Supported built-ins:
 * cd
 * pwd
 * echo
 * exit
 */
int is_builtin(const command_t *cmd);

/*
 * Execute a built-in command.
 */
int execute_builtin(command_t *cmd);

#endif
