#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#define MAX_JOBS 64
#define MAX_JOB_COMMAND 256

typedef enum
{
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
} job_state_t;

typedef struct
{
    int job_id;
    pid_t pgid;
    job_state_t state;
    char command[MAX_JOB_COMMAND];
} job_t;

static job_t job_table[MAX_JOBS];
static int next_job_id = 1;

void jobs_init(void)
{
    for (int i = 0; i < MAX_JOBS; i++)
    {
        job_table[i].job_id = 0;
        job_table[i].pgid = 0;
        job_table[i].state = JOB_DONE;
        job_table[i].command[0] = '\0';
    }

    next_job_id = 1;
}

int job_add(pid_t pgid, const char *command, job_state_t state)
{
    for (int i = 0; i < MAX_JOBS; i++)
    {
        if (job_table[i].job_id == 0)
        {
            job_table[i].job_id = next_job_id++;
            job_table[i].pgid = pgid;
            job_table[i].state = state;

            if (command != NULL)
            {
                snprintf(job_table[i].command,
                         sizeof(job_table[i].command),
                         "%s",
                         command);
            }
            else
            {
                job_table[i].command[0] = '\0';
            }

            return job_table[i].job_id;
        }
    }

    fprintf(stderr, "job table full\n");
    return -1;
}

job_t *job_find(int job_id)
{
    for (int i = 0; i < MAX_JOBS; i++)
    {
        if (job_table[i].job_id == job_id)
            return &job_table[i];
    }

    return NULL;
}

job_t *job_find_by_pgid(pid_t pgid)
{
    for (int i = 0; i < MAX_JOBS; i++)
    {
        if (job_table[i].job_id != 0 &&
            job_table[i].pgid == pgid)
        {
            return &job_table[i];
        }
    }

    return NULL;
}

void job_remove(int job_id)
{
    job_t *job = job_find(job_id);

    if (job == NULL)
        return;

    job->job_id = 0;
    job->pgid = 0;
    job->state = JOB_DONE;
    job->command[0] = '\0';
}

void job_stop(pid_t pgid)
{
    job_t *job = job_find_by_pgid(pgid);

    if (job != NULL)
        job->state = JOB_STOPPED;
}

void job_continue(pid_t pgid)
{
    job_t *job = job_find_by_pgid(pgid);

    if (job != NULL)
        job->state = JOB_RUNNING;
}

void job_done(pid_t pgid)
{
    job_t *job = job_find_by_pgid(pgid);

    if (job != NULL)
        job->state = JOB_DONE;
}

void jobs_print(void)
{
    for (int i = 0; i < MAX_JOBS; i++)
    {
        if (job_table[i].job_id == 0)
            continue;

        const char *state_text;

        if (job_table[i].state == JOB_RUNNING)
            state_text = "Running";
        else if (job_table[i].state == JOB_STOPPED)
            state_text = "Stopped";
        else
            state_text = "Done";

        printf("[%d] %-8s %s\n",
               job_table[i].job_id,
               state_text,
               job_table[i].command);
    }
}
