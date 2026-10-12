#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

static pid_t jobcontrol_shell_pgid = -1;

void jobcontrol_set_shell_pgid(pid_t pgid)
{
    jobcontrol_shell_pgid = pgid;
}

static void jobcontrol_give_terminal(pid_t pgid)
{
    if (isatty(STDIN_FILENO))
    {
        if (tcsetpgrp(STDIN_FILENO, pgid) == -1 &&
            errno != ENOTTY)
        {
            perror("tcsetpgrp");
        }
    }
}

static void jobcontrol_take_terminal_back(void)
{
    if (jobcontrol_shell_pgid > 0 &&
        isatty(STDIN_FILENO))
    {
        if (tcsetpgrp(STDIN_FILENO,
                      jobcontrol_shell_pgid) == -1 &&
            errno != ENOTTY)
        {
            perror("tcsetpgrp");
        }
    }
}

void jobcontrol_update(void)
{
    int status;
    pid_t pid;

    while ((pid = waitpid(-1,
                          &status,
                          WNOHANG |
                          WUNTRACED |
                          WCONTINUED)) > 0)
    {
        job_t *job = job_find_by_pgid(pid);

        if (job == NULL)
            continue;

        if (WIFSTOPPED(status))
        {
            job_stop(job->pgid);
        }
        else if (WIFCONTINUED(status))
        {
            job_continue(job->pgid);
        }
        else if (WIFEXITED(status) ||
                 WIFSIGNALED(status))
        {
            job_done(job->pgid);
        }
    }
}

int jobcontrol_jobs(command_t *cmd)
{
    if (cmd->argc > 1)
    {
        fprintf(stderr,
                "jobs: too many arguments\n");
        return -1;
    }

    jobcontrol_update();

    jobs_print();

    /*
     * Remove completed jobs after displaying
     * their Done state once.
     */
    for (int id = 1;
         id < next_job_id;
         id++)
    {
        job_t *job = job_find(id);

        if (job != NULL &&
            job->state == JOB_DONE)
        {
            job_remove(id);
        }
    }

    return 0;
}

int jobcontrol_bg(command_t *cmd)
{
    if (cmd->argc != 2)
    {
        fprintf(stderr,
                "usage: bg <job_id>\n");
        return -1;
    }

    int job_id = atoi(cmd->argv[1]);

    job_t *job = job_find(job_id);

    if (job == NULL)
    {
        fprintf(stderr,
                "bg: job %d not found\n",
                job_id);
        return -1;
    }

    if (job->state == JOB_DONE)
    {
        fprintf(stderr,
                "bg: job %d is already done\n",
                job_id);

        job_remove(job_id);

        return -1;
    }

    /*
     * Negative PGID means send the signal
     * to the whole process group.
     */
    if (kill(-job->pgid, SIGCONT) == -1)
    {
        perror("bg");
        return -1;
    }

    job_continue(job->pgid);

    printf("[%d] %s &\n",
           job->job_id,
           job->command);

    return 0;
}

int jobcontrol_fg(command_t *cmd)
{
    if (cmd->argc != 2)
    {
        fprintf(stderr,
                "usage: fg <job_id>\n");
        return -1;
    }

    int job_id = atoi(cmd->argv[1]);

    job_t *job = job_find(job_id);

    if (job == NULL)
    {
        fprintf(stderr,
                "fg: job %d not found\n",
                job_id);
        return -1;
    }

    if (job->state == JOB_DONE)
    {
        fprintf(stderr,
                "fg: job %d is already done\n",
                job_id);

        job_remove(job_id);

        return -1;
    }

    printf("%s\n",
           job->command);

    /*
     * Give terminal control to the job.
     */
    jobcontrol_give_terminal(job->pgid);

    /*
     * If stopped earlier using Ctrl+Z,
     * continue it.
     */
    if (job->state == JOB_STOPPED)
    {
        if (kill(-job->pgid,
                 SIGCONT) == -1)
        {
            perror("fg");

            jobcontrol_take_terminal_back();

            return -1;
        }

        job_continue(job->pgid);
    }

    int status;
    pid_t result;

    do
    {
        result =
            waitpid(-job->pgid,
                    &status,
                    WUNTRACED);
    }
    while (result == -1 &&
           errno == EINTR);

    /*
     * Return terminal control to Shellforge.
     */
    jobcontrol_take_terminal_back();

    if (result == -1)
    {
        if (errno == ECHILD)
        {
            job_done(job->pgid);
            job_remove(job_id);

            return 0;
        }

        perror("fg: waitpid");

        return -1;
    }

    /*
     * Ctrl+Z stopped it again.
     */
    if (WIFSTOPPED(status))
    {
        job_stop(job->pgid);

        printf("[%d] Stopped  %s\n",
               job->job_id,
               job->command);

        return 0;
    }

    /*
     * Job completed.
     */
    if (WIFEXITED(status) ||
        WIFSIGNALED(status))
    {
        job_done(job->pgid);
        job_remove(job_id);
    }

    return 0;
}
