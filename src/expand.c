#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "expand.h"

#define MAX_EXPANDED 4096

char *expand_variables(const char *input)
{
    char *output = malloc(MAX_EXPANDED);

    if (output == NULL)
    {
        perror("malloc");
        return NULL;
    }

    output[0] = '\0';

    const char *p = input;

    while (*p != '\0')
    {
        if (*p == '$')
        {
            p++;

            char variable[256];
            int i = 0;

            while ((*p >= 'A' && *p <= 'Z') ||
                   (*p >= 'a' && *p <= 'z') ||
                   (*p >= '0' && *p <= '9') ||
                   *p == '_')
            {
                if (i < 255)
                    variable[i++] = *p;

                p++;
            }

            variable[i] = '\0';

            char *value = getenv(variable);

            if (value != NULL)
            {
                strcat(output, value);
            }
        }
        else
        {
            int len = strlen(output);

            if (len < MAX_EXPANDED - 1)
            {
                output[len] = *p;
                output[len + 1] = '\0';
            }

            p++;
        }
    }

    return output;
}
