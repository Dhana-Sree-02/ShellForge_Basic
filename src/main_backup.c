#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "expand.h"
#include "builtin.h"

int main()
{
    char input[4096];

    while (1)
    {
        printf("shellforge$ ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        if (strlen(input) == 0)
        {
            continue;
        }

        /* Expand environment variables */
        char *expanded = expand_variables(input);

        if (expanded == NULL)
        {
            continue;
        }

        /* Parse command */
        Command *cmd = parse_command(expanded);

        if (cmd == NULL)
        {
            free(expanded);
            continue;
        }

        /* Check and execute builtin */
        if (is_builtin(cmd))
        {
            execute_builtin(cmd);
        }
        else
        {
            printf("Command not found: %s\n", cmd->args[0]);
        }

        free_command(cmd);
        free(expanded);
    }

    return 0;
}
