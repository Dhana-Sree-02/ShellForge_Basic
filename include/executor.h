#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parser.h"

void execute_command(Command *cmd);
void execute_pipeline(char *input);

#endif
