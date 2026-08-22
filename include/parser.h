#ifndef PARSER_H
#define PARSER_H

#define MAX_ARGS 64

typedef struct
{
    char *args[MAX_ARGS];
    int argc;
} Command;

Command *parse_command(char *input);
void free_command(Command *cmd);

#endif
