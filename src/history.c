#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "../include/history.h"

void history_init(void)
{
    using_history();
}

void history_add_command(const char *command)
{
    if (command == NULL || strlen(command) == 0)
    {
        return;
    }

    /*
     * Do not store the "history" command itself.
     * This matches the expected Milestone-2 behavior.
     */
    if (strcmp(command, "history") == 0)
    {
        return;
    }

    add_history(command);
}

void history_show(void)
{
    HIST_ENTRY **list;
    int count;

    list = history_list();

    printf("\n---------- Command History ----------\n");

    if (list != NULL)
    {
        count = history_length;

        for (int i = 0; i < count; i++)
        {
            if (list[i] != NULL)
            {
                printf("%d  %s\n",
                       i + 1,
                       list[i]->line);
            }
        }
    }

    printf("-------------------------------------\n");
}

void history_cleanup(void)
{
    clear_history();
}
