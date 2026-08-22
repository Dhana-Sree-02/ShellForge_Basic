#ifndef BUILTIN_H
#define BUILTIN_H

#include "parser.h"

int is_builtin(Command *cmd);
int execute_builtin(Command *cmd);

#endif
