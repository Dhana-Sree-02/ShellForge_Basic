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
     * Milestone 4.2:
     *
     * Clean up completed background processes.
     */
    setup_background_handler();


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
         * Ctrl+D / EOF.
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
         * LEXICAL ANALYSIS
         * --------------------------------------------
         */
        lexer(line, &tokens);


        /*
         * --------------------------------------------
         * PARSING
         * --------------------------------------------
         *
         * IMPORTANT:
         *
         * Your parser returns:
         *
         *     1 = success
         *     0 = error
         *
         * So we MUST use:
         *
         *     if (parse(...))
         *
         * NOT:
         *
         *     if (parse(...) != 0)
         */
        if (parse(&tokens, &pipeline))
        {
            /*
             * ----------------------------------------
             * VARIABLE EXPANSION
             * ----------------------------------------
             */
            expand_variables(&pipeline);


            /*
             * ----------------------------------------
             * MILESTONE 4.1
             *
             * Execute the complete pipeline.
             * ----------------------------------------
             */
            if (pipeline.command_count > 0)
            {
                execute_pipeline(&pipeline);
            }


            /*
             * Free dynamically allocated
             * command data.
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
