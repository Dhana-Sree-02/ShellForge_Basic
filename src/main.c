#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "parser.h"
#include "executor.h"

#define MAX_INPUT 1024

int main(void)
{
    char input[MAX_INPUT];

    while (1)
    {
        printf("shellforge$ ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            printf("\n");
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        if (strlen(input) == 0)
            continue;

        /*
         * Exit
         */
        if (strcmp(input, "exit") == 0)
        {
            break;
        }

        /*
         * Built-in cd
         */
        if (strncmp(input, "cd", 2) == 0 &&
            (input[2] == '\0' || input[2] == ' '))
        {
            char *path = input + 2;

            while (*path == ' ')
                path++;

            if (*path == '\0')
                path = getenv("HOME");

            if (chdir(path) != 0)
                perror("cd");

            continue;
        }

        /*
         * Pipeline
         */
        if (strchr(input, '|') != NULL)
        {
            execute_pipeline(input);
            continue;
        }

        /*
         * Normal command
         */
        Command *cmd = parse_command(input);

        if (cmd == NULL)
            continue;

        execute_command(cmd);

        free_command(cmd);
    }

    return 0;
}
