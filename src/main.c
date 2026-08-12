#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "lexer.h"
#include "token.h"

void display_history(void)
{
    HIST_ENTRY **hist = history_list();

    printf("\n");
    printf("+------+--------------------------------+\n");
    printf("| No.  | Command                        |\n");
    printf("+------+--------------------------------+\n");

    if (hist != NULL)
    {
        for (int i = 0; hist[i] != NULL; i++)
        {
            printf("| %-4d | %-30s |\n",
                   i + 1,
                   hist[i]->line);
        }
    }

    printf("+------+--------------------------------+\n");
    printf("\n");
}

int main(void)
{
    printf("=====================================\n");
    printf("Shellforge\n");
    printf(" A Unix Style Shell written in C\n");
    printf("=====================================\n");

    char *line;

    while (1)
    {
        line = readline("shellforge$ ");

        if (line == NULL)
        {
            printf("\nGoodbye!\n");
            break;
        }

        if (strlen(line) == 0)
        {
            free(line);
            continue;
        }

        /* Add command to history */
        add_history(line);

        /* Display command history */
        if (strcmp(line, "history") == 0)
        {
            display_history();
            free(line);
            continue;
        }

        /* Exit */
        if (strcmp(line, "exit") == 0)
        {
            free(line);
            printf("Exiting...\n");
            break;
        }

        /*
         * Create token list and run lexer
         */
        TokenList list;

        lexer(line, &list);

        /*
         * Display tokens
         */
        token_list_print(&list);

        free(line);
    }

    clear_history();

    return 0;
}
