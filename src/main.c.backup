#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>

#include "history.h"
#include "token.h"
#include "lexer.h"
#include "parser.h"
#include "expand.h"
#include "builtin.h"
#include "executor.h"


int main(void)
{
    token_list tokens;
    pipeline_t pipeline;
    char *line;


    /*
     * Initialize command history.
     */
    history_init();


    /*
     * Display welcome message.
     */
    printf("=====================================\n");
    printf("             Shellforge\n");
    printf("      A Unix Style Shell in C\n");
    printf("=====================================\n");


    /*
     * Main shell loop.
     */
    while (1)
    {
        /*
         * Read command from user.
         */
        line = readline("shellforge$ ");


        /*
         * Ctrl+D / EOF
         */
        if (line == NULL)
        {
            printf("\nGoodbye!\n");
            break;
        }


        /*
         * Ignore empty commands.
         */
        if (strlen(line) == 0)
        {
            free(line);
            continue;
        }


        /*
         * Special history command.
         */
        if (strcmp(line, "history") == 0)
        {
            history_show();

            free(line);
            continue;
        }


        /*
         * Add command to shell history.
         */
        history_add_command(line);


        /*
         * --------------------------------------------
         * MILESTONE 2.1
         * LEXICAL ANALYSIS
         * --------------------------------------------
         */
        lexer(line, &tokens);


        /*
         * --------------------------------------------
         * MILESTONE 2.2
         * PARSING
         * --------------------------------------------
         */
        if (parse(&tokens, &pipeline))
        {
            /*
             * Expand environment variables.
             */
            expand_variables(&pipeline);


            /*
             * ----------------------------------------
             * MILESTONE 3.1 + 3.2
             * EXECUTE COMMANDS
             * ----------------------------------------
             */
            for (int i = 0;
                 i < pipeline.command_count;
                 i++)
            {
                int result;

                result =
                    execute_command(
                        &pipeline.commands[i]
                    );


                /*
                 * Built-in exit returns 1.
                 */
                if (result == 1)
                {
                    pipeline_free(&pipeline);
                    free(line);
                    history_cleanup();

                    return 0;
                }
            }


            /*
             * Free dynamically allocated command data.
             */
            pipeline_free(&pipeline);
        }


        /*
         * Free readline memory.
         */
        free(line);
    }


    /*
     * Clean up history.
     */
    history_cleanup();


    return 0;
}
