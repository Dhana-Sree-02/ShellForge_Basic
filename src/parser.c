#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

Command *parse_command(char *input)
{
    Command *cmd = malloc(sizeof(Command));

    if (cmd == NULL)
    {
        perror("malloc");
        return NULL;
    }

    cmd->argc = 0;

    char *token = strtok(input, " \t\n");

    while (token != NULL && cmd->argc < MAX_ARGS - 1)
    {
        cmd->args[cmd->argc] = strdup(token);

        if (cmd->args[cmd->argc] == NULL)
        {
            perror("strdup");
            free_command(cmd);
            return NULL;
        }

        cmd->argc++;

        token = strtok(NULL, " \t\n");
    }

    cmd->args[cmd->argc] = NULL;

    return cmd;
}

void free_command(Command *cmd)
{
    if (cmd == NULL)
        return;

    for (int i = 0; i < cmd->argc; i++)
    {
        free(cmd->args[i]);
    }

    free(cmd);
}
