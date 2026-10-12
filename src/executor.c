#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include "executor.h"
#include "builtin.h"

/* Milestone 5 job-control implementation.
 * The Makefile is intentionally unchanged, so these source files
 * are included here as part of executor.c. */
#include "job.c"
#include "jobcontrol.c"


/* =========================================================
 * SIGCHLD HANDLER
 * ========================================================= */

/*
 * Reap finished background processes.
 *
 * WNOHANG means:
 *     do not wait if no child has finished.
 *
 * This prevents zombie processes.
 */
static volatile sig_atomic_t sigchld_pending = 0;

static void sigchld_handler(int signal_number)
{
    (void)signal_number;

    /*
     * Do not call waitpid() here.  The job table needs the child
     * status so that jobs can distinguish Running/Stopped/Done.
     * Reaping is therefore performed outside the signal handler.
     */
    sigchld_pending = 1;
}

/*
 * Install SIGCHLD handler.
 */
void setup_background_handler(void)
{
    struct sigaction action;

    action.sa_handler = sigchld_handler;

    sigemptyset(&action.sa_mask);

    action.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    if (sigaction(SIGCHLD, &action, NULL) < 0)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    /* Initialize the Milestone 5 job table. */
    jobs_init();

    /*
     * Keep the shell out of the terminal foreground while a child
     * process group is running.  Child processes restore the default
     * signal dispositions after fork().
     */
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    jobcontrol_set_shell_pgid(getpgrp());
}


/* =========================================================
 * SIGNAL BLOCKING
 * ========================================================= */

/*
 * Block SIGCHLD while creating/waiting for foreground
 * processes.
 *
 * This prevents the background cleanup handler from
 * accidentally reaping a foreground child.
 */
static int block_sigchld(sigset_t *old_mask)
{
    sigset_t mask;

    sigemptyset(&mask);

    sigaddset(&mask, SIGCHLD);

    if (sigprocmask(
            SIG_BLOCK,
            &mask,
            old_mask) < 0)
    {
        perror("sigprocmask");
        return -1;
    }

    return 0;
}


/*
 * Restore previous signal mask.
 */
static void restore_sigchld(const sigset_t *old_mask)
{
    if (sigprocmask(
            SIG_SETMASK,
            old_mask,
            NULL) < 0)
    {
        perror("sigprocmask");
    }
}


/* =========================================================
 * REDIRECTION
 * ========================================================= */

static int apply_redirections(command_t *cmd)
{
    int fd;


    /*
     * Input:
     *
     * command < file
     */
    if (cmd->input != NULL)
    {
        fd = open(cmd->input, O_RDONLY);

        if (fd < 0)
        {
            perror(cmd->input);
            return -1;
        }

        if (dup2(fd, STDIN_FILENO) < 0)
        {
            perror("dup2");

            close(fd);

            return -1;
        }

        close(fd);
    }


    /*
     * Output:
     *
     * command > file
     *
     * command >> file
     */
    if (cmd->output != NULL)
    {
        if (cmd->append)
        {
            fd = open(
                cmd->output,
                O_WRONLY | O_CREAT | O_APPEND,
                0644
            );
        }
        else
        {
            fd = open(
                cmd->output,
                O_WRONLY | O_CREAT | O_TRUNC,
                0644
            );
        }

        if (fd < 0)
        {
            perror(cmd->output);
            return -1;
        }

        if (dup2(fd, STDOUT_FILENO) < 0)
        {
            perror("dup2");

            close(fd);

            return -1;
        }

        close(fd);
    }

    return 0;
}


/* =========================================================
 * BACKGROUND STDIN
 * ========================================================= */

/*
 * Background processes must not read from the shell
 * terminal.
 *
 * Therefore stdin is connected to /dev/null.
 */
static int redirect_background_stdin(void)
{
    int fd;

    fd = open("/dev/null", O_RDONLY);

    if (fd < 0)
    {
        perror("/dev/null");
        return -1;
    }

    if (dup2(fd, STDIN_FILENO) < 0)
    {
        perror("dup2");

        close(fd);

        return -1;
    }

    close(fd);

    return 0;
}


/* =========================================================
 * JOB-CONTROL HELPERS
 * ========================================================= */

static void restore_child_signals(void)
{
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
}

static void set_foreground_pgid(pid_t pgid)
{
    if (isatty(STDIN_FILENO))
    {
        if (tcsetpgrp(STDIN_FILENO, pgid) < 0 &&
            errno != ENOTTY)
        {
            perror("tcsetpgrp");
        }
    }
}

static void set_shell_foreground(void)
{
    if (isatty(STDIN_FILENO))
    {
        if (tcsetpgrp(STDIN_FILENO, getpgrp()) < 0 &&
            errno != ENOTTY)
        {
            perror("tcsetpgrp");
        }
    }
}

static void build_job_command(const pipeline_t *pipeline,
                              char *buffer,
                              size_t size)
{
    size_t used = 0;
    int i;
    int j;

    if (buffer == NULL || size == 0)
        return;

    buffer[0] = '\0';

    for (i = 0; i < pipeline->command_count; i++)
    {
        const command_t *cmd = &pipeline->commands[i];

        for (j = 0; j < cmd->argc; j++)
        {
            int written;

            if (used >= size - 1)
                return;

            if (used != 0)
            {
                written = snprintf(buffer + used,
                                   size - used,
                                   " ");
                if (written < 0 || (size_t)written >= size - used)
                    return;
                used += (size_t)written;
            }

            written = snprintf(buffer + used,
                               size - used,
                               "%s",
                               cmd->argv[j]);
            if (written < 0 || (size_t)written >= size - used)
                return;
            used += (size_t)written;
        }

        if (i < pipeline->command_count - 1)
        {
            if (used >= size - 1)
                return;

            if (snprintf(buffer + used,
                         size - used,
                         " | ") < 0)
                return;

            used = strlen(buffer);
        }
    }

    if (pipeline->commands[pipeline->command_count - 1].background &&
        used < size - 2)
    {
        strcat(buffer, " &");
    }
}

static int is_job_builtin(const command_t *cmd)
{
    if (cmd == NULL || cmd->argc == 0 || cmd->argv[0] == NULL)
        return 0;

    return strcmp(cmd->argv[0], "jobs") == 0 ||
           strcmp(cmd->argv[0], "fg") == 0 ||
           strcmp(cmd->argv[0], "bg") == 0;
}

static int execute_job_builtin(command_t *cmd)
{
    if (strcmp(cmd->argv[0], "jobs") == 0)
        return jobcontrol_jobs(cmd);

    if (strcmp(cmd->argv[0], "fg") == 0)
        return jobcontrol_fg(cmd);

    if (strcmp(cmd->argv[0], "bg") == 0)
        return jobcontrol_bg(cmd);

    return -1;
}

static void update_jobs_if_needed(void)
{
    if (sigchld_pending)
        sigchld_pending = 0;

    jobcontrol_update();
}


/* =========================================================
 * SINGLE COMMAND
 * ========================================================= */

int execute_command(command_t *cmd)
{
    pid_t pid;

    int status;

    sigset_t old_mask;


    /*
     * Invalid command.
     */
    if (cmd == NULL ||
        cmd->argc == 0 ||
        cmd->argv[0] == NULL)
    {
        return -1;
    }

    update_jobs_if_needed();

    /*
     * -----------------------------------------------------
     * JOB CONTROL BUILTIN
     * -----------------------------------------------------
     */
    if (is_job_builtin(cmd))
    {
        return execute_job_builtin(cmd);
    }


    /*
     * -----------------------------------------------------
     * FOREGROUND BUILTIN
     * -----------------------------------------------------
     *
     * cd must execute in the parent shell.
     */
    if (is_builtin(cmd) &&
        !cmd->background)
    {
        return execute_builtin(cmd);
    }


    /*
     * Block SIGCHLD while creating the child.
     */
    if (block_sigchld(&old_mask) < 0)
    {
        return -1;
    }


    /*
     * Create child.
     */
    pid = fork();


    if (pid < 0)
    {
        perror("fork");

        restore_sigchld(&old_mask);

        return -1;
    }


    /*
     * =====================================================
     * CHILD
     * =====================================================
     */
    if (pid == 0)
    {
        /*
         * Restore normal signal mask.
         */
        restore_sigchld(&old_mask);

        restore_child_signals();

        /*
         * Put this child in its own process group.
         */
        if (setpgid(0, 0) < 0)
        {
            perror("setpgid");
            _exit(1);
        }

        /*
         * Background command:
         *
         * stdin -> /dev/null
         */
        if (cmd->background)
        {
            if (redirect_background_stdin() < 0)
            {
                _exit(1);
            }
        }


        /*
         * Explicit redirection is applied after
         * /dev/null.
         *
         * Therefore:
         *
         * command &
         *
         * gets /dev/null
         *
         * while:
         *
         * command < input.txt &
         *
         * gets input.txt.
         */
        if (apply_redirections(cmd) < 0)
        {
            _exit(1);
        }


        /*
         * Background builtin.
         *
         * It runs in the child, so:
         *
         * cd /tmp &
         *
         * does NOT change the parent shell directory.
         */
        if (is_builtin(cmd))
        {
            int result;

            result = execute_builtin(cmd);

            if (result < 0)
            {
                _exit(1);
            }

            _exit(0);
        }


        /*
         * External command.
         */
        execvp(
            cmd->argv[0],
            cmd->argv
        );


        /*
         * execvp() failed.
         */
        perror("execvp");

        _exit(127);
    }


    /*
     * =====================================================
     * PARENT
     * =====================================================
     */

    /* Race-safe process-group setup: parent also requests the child
     * to become the leader of its own process group. */
    if (setpgid(pid, pid) < 0 && errno != EACCES && errno != ESRCH)
    {
        perror("setpgid");
    }


    /*
     * -----------------------------------------------------
     * BACKGROUND
     * -----------------------------------------------------
     */
    if (cmd->background)
    {
        char job_command[256];
        int job_id;

        /* execute_command() receives a single command. */
        snprintf(job_command, sizeof(job_command), "%s", cmd->argv[0]);
        for (int i = 1; i < cmd->argc && strlen(job_command) < sizeof(job_command) - 1; i++)
        {
            strncat(job_command, " ", sizeof(job_command) - strlen(job_command) - 1);
            strncat(job_command, cmd->argv[i], sizeof(job_command) - strlen(job_command) - 1);
        }
        strncat(job_command, " &", sizeof(job_command) - strlen(job_command) - 1);

        job_id = job_add(pid, job_command, JOB_RUNNING);

        if (job_id > 0)
        {
            printf("[%d] %d\n", job_id, (int)pid);
            fflush(stdout);
        }

        /* Do NOT wait for a background process. */
        restore_sigchld(&old_mask);

        return 0;
    }


    /*
     * -----------------------------------------------------
     * FOREGROUND
     * -----------------------------------------------------
     */
    set_foreground_pgid(pid);

    while (waitpid(
               pid,
               &status,
               WUNTRACED) < 0)
    {
        if (errno == EINTR)
        {
            continue;
        }

        perror("waitpid");

        restore_sigchld(&old_mask);

        return -1;
    }


    /* Return terminal control to the shell. */
    set_shell_foreground();

    /*
     * If Ctrl+Z stopped the foreground process, turn it into a job.
     */
    if (WIFSTOPPED(status))
    {
        char job_command[256];
        int job_id;

        snprintf(job_command, sizeof(job_command), "%s", cmd->argv[0]);
        for (int i = 1; i < cmd->argc && strlen(job_command) < sizeof(job_command) - 1; i++)
        {
            strncat(job_command, " ", sizeof(job_command) - strlen(job_command) - 1);
            strncat(job_command, cmd->argv[i], sizeof(job_command) - strlen(job_command) - 1);
        }

        job_id = job_add(pid, job_command, JOB_STOPPED);
        if (job_id > 0)
        {
            printf("[%d] Stopped  %s\n", job_id, job_command);
            fflush(stdout);
        }
    }

    /*
     * Restore SIGCHLD after foreground wait.
     */
    restore_sigchld(&old_mask);


    /*
     * Return exit status.
     */
    if (WIFEXITED(status))
    {
        return WEXITSTATUS(status);
    }


    if (WIFSIGNALED(status))
    {
        return 128 + WTERMSIG(status);
    }


    return 0;
}


/* =========================================================
 * PIPELINE
 * ========================================================= */

int execute_pipeline(pipeline_t *pipeline)
{
    int command_count;

    int background;

    int pipes[MAX_COMMANDS - 1][2];

    pid_t pids[MAX_COMMANDS];

    int last_status = 0;
    int pipeline_stopped = 0;

    sigset_t old_mask;


    /*
     * Invalid pipeline.
     */
    if (pipeline == NULL)
    {
        return -1;
    }

    update_jobs_if_needed();

    command_count = pipeline->command_count;


    /*
     * Empty pipeline.
     */
    if (command_count <= 0)
    {
        return -1;
    }


    /*
     * Single command.
     */
    if (command_count == 1)
    {
        return execute_command(
            &pipeline->commands[0]
        );
    }


    /*
     * Background status belongs to the LAST command.
     *
     * The parser already places the background flag
     * on the final command of the pipeline.
     */
    background =
        pipeline->commands[
            command_count - 1
        ].background;


    /*
     * Block SIGCHLD while creating the pipeline.
     */
    if (block_sigchld(&old_mask) < 0)
    {
        return -1;
    }


    /*
     * =====================================================
     * CREATE PIPES
     * =====================================================
     */
    for (int i = 0;
         i < command_count - 1;
         i++)
    {
        if (pipe(pipes[i]) < 0)
        {
            perror("pipe");


            for (int j = 0; j < i; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }


            restore_sigchld(&old_mask);

            return -1;
        }
    }


    /*
     * =====================================================
     * CREATE CHILDREN
     * =====================================================
     */
    for (int i = 0;
         i < command_count;
         i++)
    {
        pid_t pid;

        command_t *cmd;


        cmd = &pipeline->commands[i];


        pid = fork();


        if (pid < 0)
        {
            perror("fork");


            /*
             * Close pipes.
             */
            for (int j = 0;
                 j < command_count - 1;
                 j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }


            /*
             * Wait for already-created children
             * when this is a foreground pipeline.
             */
            if (!background)
            {
                for (int j = 0;
                     j < i;
                     j++)
                {
                    waitpid(
                        pids[j],
                        NULL,
                        0
                    );
                }
            }


            restore_sigchld(&old_mask);

            return -1;
        }


        /*
         * =================================================
         * CHILD
         * =================================================
         */
        if (pid == 0)
        {
            /*
             * Restore normal signal mask and default terminal signals.
             */
            restore_sigchld(&old_mask);
            restore_child_signals();

            /* Put every pipeline process in one process group. */
            if (i == 0)
            {
                if (setpgid(0, 0) < 0)
                {
                    perror("setpgid");
                    _exit(1);
                }
            }
            else
            {
                if (setpgid(0, pids[0]) < 0)
                {
                    perror("setpgid");
                    _exit(1);
                }
            }


            /*
             * ---------------------------------------------
             * INPUT FROM PREVIOUS PIPE
             * ---------------------------------------------
             */
            if (i > 0)
            {
                if (dup2(
                        pipes[i - 1][0],
                        STDIN_FILENO) < 0)
                {
                    perror("dup2");

                    _exit(1);
                }
            }


            /*
             * ---------------------------------------------
             * OUTPUT TO NEXT PIPE
             * ---------------------------------------------
             */
            if (i < command_count - 1)
            {
                if (dup2(
                        pipes[i][1],
                        STDOUT_FILENO) < 0)
                {
                    perror("dup2");

                    _exit(1);
                }
            }


            /*
             * ---------------------------------------------
             * BACKGROUND PIPELINE STDIN
             * ---------------------------------------------
             *
             * Only the FIRST command needs terminal input.
             *
             * Example:
             *
             *     cat | grep abc &
             *
             * cat gets /dev/null.
             *
             * grep gets input from cat through the pipe.
             */
            if (background &&
                i == 0 &&
                cmd->input == NULL)
            {
                if (redirect_background_stdin() < 0)
                {
                    _exit(1);
                }
            }


            /*
             * ---------------------------------------------
             * EXPLICIT REDIRECTION
             * ---------------------------------------------
             */
            if (apply_redirections(cmd) < 0)
            {
                _exit(1);
            }


            /*
             * ---------------------------------------------
             * CLOSE ALL PIPE DESCRIPTORS
             * ---------------------------------------------
             */
            for (int j = 0;
                 j < command_count - 1;
                 j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }


            /*
             * ---------------------------------------------
             * BUILTIN IN PIPELINE
             * ---------------------------------------------
             *
             * Pipeline builtins execute in a child.
             */
            if (is_builtin(cmd))
            {
                int result;

                result = execute_builtin(cmd);

                if (result < 0)
                {
                    _exit(1);
                }

                _exit(0);
            }


            /*
             * ---------------------------------------------
             * EXTERNAL COMMAND
             * ---------------------------------------------
             */
            execvp(
                cmd->argv[0],
                cmd->argv
            );


            /*
             * execvp() failed.
             */
            perror("execvp");

            _exit(127);
        }


        /*
         * Parent stores PID and establishes the same process group
         * for the complete pipeline.
         */
        pids[i] = pid;

        if (i == 0)
        {
            if (setpgid(pid, pid) < 0 && errno != EACCES && errno != ESRCH)
                perror("setpgid");
        }
        else
        {
            if (setpgid(pid, pids[0]) < 0 && errno != EACCES && errno != ESRCH)
                perror("setpgid");
        }
    }


    /*
     * =====================================================
     * PARENT CLOSES ALL PIPE DESCRIPTORS
     * =====================================================
     */
    for (int i = 0;
         i < command_count - 1;
         i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }


    /*
     * =====================================================
     * BACKGROUND PIPELINE
     * =====================================================
     */
    if (background)
    {
        char job_command[256];
        int job_id;

        build_job_command(pipeline, job_command, sizeof(job_command));
        job_id = job_add(pids[0], job_command, JOB_RUNNING);

        if (job_id > 0)
        {
            printf("[%d] %d\n", job_id, (int)pids[0]);
            fflush(stdout);
        }

        /* Do NOT wait. */
        restore_sigchld(&old_mask);

        return 0;
    }


    /*
     * =====================================================
     * FOREGROUND PIPELINE
     * =====================================================
     */
    set_foreground_pgid(pids[0]);

    for (int i = 0;
         i < command_count;
         i++)
    {
        int status;


        while (waitpid(
                   pids[i],
                   &status,
                   WUNTRACED) < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            perror("waitpid");

            restore_sigchld(&old_mask);

            return -1;
        }


        if (WIFSTOPPED(status))
            pipeline_stopped = 1;

        /*
         * Save only the status of the last command.
         */
        if (i == command_count - 1)
        {
            if (WIFEXITED(status))
            {
                last_status =
                    WEXITSTATUS(status);
            }
            else if (WIFSIGNALED(status))
            {
                last_status =
                    128 + WTERMSIG(status);
            }
        }
    }


    /* Return terminal control to the shell. */
    set_shell_foreground();

    if (pipeline_stopped)
    {
        char job_command[256];
        int job_id;

        build_job_command(pipeline, job_command, sizeof(job_command));
        job_id = job_add(pids[0], job_command, JOB_STOPPED);

        if (job_id > 0)
        {
            printf("[%d] Stopped  %s\n", job_id, job_command);
            fflush(stdout);
        }
    }

    /*
     * Restore SIGCHLD handling.
     */
    restore_sigchld(&old_mask);


    return last_status;
}
