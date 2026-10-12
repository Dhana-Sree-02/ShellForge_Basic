#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "executor.h"
#include "builtin.h"


/*
 * Execute one command.
 *
 * Built-in commands:
 *      cd
 *      pwd
 *      echo
 *      exit
 *
 * External commands:
 *      fork()
 *      execvp()
 *      waitpid()
 *
 * Returns:
 *      0  -> success
 *      1  -> exit requested
 *     -1  -> error
 */
int execute_command(command_t *cmd)
{
    pid_t pid;
    int status;


    /*
     * Check whether command is valid.
     */
    if (cmd == NULL || cmd->argc == 0)
    {
        return -1;
    }


    /*
     * ------------------------------------------------
     * CHECK FOR BUILT-IN COMMAND
     * ------------------------------------------------
     *
     * Built-in commands must execute inside
     * the shell process.
     *
     * This is especially important for cd because
     * changing directory inside a child process
     * would not change the shell's directory.
     */
    if (is_builtin(cmd))
    {
        return execute_builtin(cmd);
    }


    /*
     * ------------------------------------------------
     * CREATE CHILD PROCESS
     * ------------------------------------------------
     */
    pid = fork();


    /*
     * fork() failed.
     */
    if (pid < 0)
    {
        perror("fork");
        return -1;
    }


    /*
     * ------------------------------------------------
     * CHILD PROCESS
     * ------------------------------------------------
     */
    if (pid == 0)
    {
        int fd;

        /*
         * --------------------------------------------
         * INPUT REDIRECTION
         * --------------------------------------------
         *
         * Example:
         *
         * cat < file.txt
         */
        if (cmd->input != NULL)
        {
            fd = open(cmd->input, O_RDONLY);

            if (fd < 0)
            {
                perror("Shellforge");
                exit(EXIT_FAILURE);
            }

            if (dup2(fd, STDIN_FILENO) < 0)
            {
                perror("Shellforge");
                close(fd);
                exit(EXIT_FAILURE);
            }

            close(fd);
        }


        /*
         * --------------------------------------------
         * OUTPUT REDIRECTION
         * --------------------------------------------
         *
         * Example:
         *
         * cat > file.txt
         */
        if (cmd->output != NULL)
        {
            if (cmd->append)
            {
                /*
                 * >> append mode
                 */
                fd = open(
                    cmd->output,
                    O_WRONLY | O_CREAT | O_APPEND,
                    0644
                );
            }
            else
            {
                /*
                 * > overwrite/create mode
                 */
                fd = open(
                    cmd->output,
                    O_WRONLY | O_CREAT | O_TRUNC,
                    0644
                );
            }


            if (fd < 0)
            {
                perror("Shellforge");
                exit(EXIT_FAILURE);
            }


            if (dup2(fd, STDOUT_FILENO) < 0)
            {
                perror("Shellforge");
                close(fd);
                exit(EXIT_FAILURE);
            }


            close(fd);
        }


        /*
         * --------------------------------------------
         * PREPARE ARGUMENTS FOR execvp()
         * --------------------------------------------
         *
         * execvp() requires:
         *
         * args[0] = command
         * args[1] = first argument
         * args[2] = second argument
         * ...
         * args[argc] = NULL
         */
        char *args[MAX_ARGS + 1];

        for (int i = 0; i < cmd->argc; i++)
        {
            args[i] = cmd->argv[i];
        }

        args[cmd->argc] = NULL;


        /*
         * --------------------------------------------
         * EXECUTE EXTERNAL COMMAND
         * --------------------------------------------
         *
         * execvp() replaces the child process
         * with the requested external program.
         */
        execvp(args[0], args);


        /*
         * If execvp() returns, it means execution
         * failed.
         */
        perror("Shellforge");

        exit(EXIT_FAILURE);
    }


    /*
     * ------------------------------------------------
     * PARENT PROCESS
     * ------------------------------------------------
     *
     * Wait for the child to finish.
     */
    if (waitpid(pid, &status, 0) == -1)
    {
        perror("waitpid");
        return -1;
    }


    /*
     * ------------------------------------------------
     * CHECK CHILD EXIT STATUS
     * ------------------------------------------------
     */
    if (WIFEXITED(status))
    {
        return WEXITSTATUS(status);
    }


    /*
     * Child was terminated by a signal.
     */
    if (WIFSIGNALED(status))
    {
        fprintf(
            stderr,
            "Process terminated by signal %d\n",
            WTERMSIG(status)
        );

        return -1;
    }


    return 0;
}
