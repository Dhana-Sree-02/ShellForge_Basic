#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "executor.h"

#define MAX_COMMANDS 16
#define MAX_ARGS 64

/*
 * Milestone 3.2
 * Execute one external command.
 */
void execute_command(Command *cmd)
{
    if (cmd == NULL || cmd->argc == 0)
        return;

    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return;
    }

    if (pid == 0)
    {
        /* Child */
        execvp(cmd->args[0], cmd->args);

        perror("execvp");
        exit(EXIT_FAILURE);
    }
    else
    {
        /* Parent */
        if (waitpid(pid, NULL, 0) < 0)
            perror("waitpid");
    }
}


/*
 * Convert one command string into argv.
 */
static int make_args(char *command, char **args)
{
    int argc = 0;

    char *token = strtok(command, " \t\n");

    while (token != NULL && argc < MAX_ARGS - 1)
    {
        args[argc++] = token;
        token = strtok(NULL, " \t\n");
    }

    args[argc] = NULL;

    return argc;
}


/*
 * Execute pipeline.
 *
 * Example:
 *
 * ls | grep .c
 *
 * ls | grep .c | wc -l
 */
void execute_pipeline(char *input)
{
    char *commands[MAX_COMMANDS];
    int command_count = 0;

    /*
     * Split input using |
     */
    char *token = strtok(input, "|");

    while (token != NULL && command_count < MAX_COMMANDS)
    {
        commands[command_count++] = token;
        token = strtok(NULL, "|");
    }

    if (command_count == 0)
        return;

    int previous_pipe = -1;

    pid_t pids[MAX_COMMANDS];

    for (int i = 0; i < command_count; i++)
    {
        int pipefd[2];

        /*
         * Create pipe unless this is the last command.
         */
        if (i < command_count - 1)
        {
            if (pipe(pipefd) == -1)
            {
                perror("pipe");
                return;
            }
        }

        pids[i] = fork();

        if (pids[i] < 0)
        {
            perror("fork");
            return;
        }

        if (pids[i] == 0)
        {
            /*
             * CHILD
             */

            /*
             * If this is not the first command,
             * connect previous pipe to stdin.
             */
            if (previous_pipe != -1)
            {
                if (dup2(previous_pipe, STDIN_FILENO) == -1)
                {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            /*
             * If this is not the last command,
             * connect stdout to current pipe.
             */
            if (i < command_count - 1)
            {
                if (dup2(pipefd[1], STDOUT_FILENO) == -1)
                {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            /*
             * Close unused descriptors.
             */
            if (previous_pipe != -1)
                close(previous_pipe);

            if (i < command_count - 1)
            {
                close(pipefd[0]);
                close(pipefd[1]);
            }

            /*
             * Convert command to argv.
             */
            char *args[MAX_ARGS];

            int argc = make_args(commands[i], args);

            if (argc == 0)
                exit(EXIT_SUCCESS);

            /*
             * Execute command.
             */
            execvp(args[0], args);

            perror("execvp");
            exit(EXIT_FAILURE);
        }

        /*
         * PARENT
         */

        if (previous_pipe != -1)
            close(previous_pipe);

        if (i < command_count - 1)
        {
            close(pipefd[1]);

            /*
             * Save read end for next command.
             */
            previous_pipe = pipefd[0];
        }
    }

    /*
     * Close final pipe.
     */
    if (previous_pipe != -1)
        close(previous_pipe);

    /*
     * Wait for all children.
     */
    for (int i = 0; i < command_count; i++)
    {
        waitpid(pids[i], NULL, 0);
    }
}
