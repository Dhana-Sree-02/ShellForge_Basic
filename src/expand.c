#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "../include/expand.h"

static void expand_string(char *text)
{
    char result[MAX_TOKEN_LEN];

    int i = 0;
    int j = 0;

    while (text[i] != '\0' &&
           j < MAX_TOKEN_LEN - 1)
    {
        /* Environment variable */
        if (text[i] == '$')
        {
            char variable[128];
            int v = 0;

            i++;

            /* ${VARIABLE} */
            if (text[i] == '{')
            {
                i++;

                while (text[i] != '\0' &&
                       text[i] != '}' &&
                       v < 127)
                {
                    variable[v++] = text[i++];
                }

                if (text[i] == '}')
                    i++;
            }
            /* $VARIABLE */
            else
            {
                while (text[i] != '\0' &&
                       (isalnum((unsigned char)text[i]) ||
                        text[i] == '_') &&
                       v < 127)
                {
                    variable[v++] = text[i++];
                }
            }

            variable[v] = '\0';

            if (v > 0)
            {
                const char *value = getenv(variable);

                if (value != NULL)
                {
                    for (int k = 0;
                         value[k] != '\0' &&
                         j < MAX_TOKEN_LEN - 1;
                         k++)
                    {
                        result[j++] = value[k];
                    }
                }

                continue;
            }

            /* If '$' is not followed by a variable name */
            result[j++] = '$';

            continue;
        }

        /* Normal character */
        result[j++] = text[i++];
    }

    result[j] = '\0';

    strcpy(text, result);
}

void expand_variables(pipeline_t *pipeline)
{
    if (pipeline == NULL)
        return;

    for (int i = 0;
         i < pipeline->command_count;
         i++)
    {
        command_t *command =
            &pipeline->commands[i];

        /* Expand command arguments */
        for (int j = 0;
             j < command->argc;
             j++)
        {
            expand_string(command->argv[j]);
        }

        /* Expand input filename */
        if (command->input != NULL)
        {
            expand_string(command->input);
        }

        /* Expand output filename */
        if (command->output != NULL)
        {
            expand_string(command->output);
        }
    }
}
