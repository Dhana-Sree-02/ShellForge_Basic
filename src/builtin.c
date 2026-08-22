#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtin.h"

int is_builtin(Command *cmd)
{
    if (cmd == NULL || cmd->argc == 0)
        return 0;

    if (strcmp(cmd->args[0], "cd") == 0)
        return 1;

    if (strcmp(cmd->args[0], "pwd") == 0)
        return 1;

    if (strcmp(cmd->args[0], "exit") == 0)
        return 1;

    return 0;
}

int execute_builtin(Command *cmd)
{
    if (cmd == NULL || cmd->argc == 0)
        return 0;

    /* cd command */
    if (strcmp(cmd->args[0], "cd") == 0)
    {
        const char *directory;

        if (cmd->argc < 2)
        {
            directory = getenv("HOME");
        }
        else
        {
            directory = cmd->args[1];
        }

        if (directory == NULL)
        {
            fprintf(stderr, "cd: HOME not set\n");
            return 1;
        }

        if (chdir(directory) != 0)
        {
            perror("cd");
        }

        return 1;
    }

    /* pwd command */
    if (strcmp(cmd->args[0], "pwd") == 0)
    {
        char cwd[4096];

        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
	printf("This is My Function");
            printf("%s\n", cwd);
        }
        else
        {
            perror("pwd");
        }

        return 1;
    }

    /* exit command */
    if (strcmp(cmd->args[0], "exit") == 0)
    {
        exit(0);
    }

    return 0;
}
