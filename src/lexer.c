#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "lexer.h"
#include "token.h"

void lexer(const char *input, TokenList *list)
{
    int i = 0;

    token_list_init(list);

    while (input[i] != '\0')
    {
        /* Skip whitespace */
        if (isspace((unsigned char)input[i]))
        {
            i++;
            continue;
        }

        /* Pipe */
        if (input[i] == '|')
        {
            token_add(list, TOKEN_PIPE, "|");
            i++;
            continue;
        }

        /* Input redirection */
        if (input[i] == '<')
        {
            token_add(list, TOKEN_INPUT, "<");
            i++;
            continue;
        }

        /* Output redirection / append */
        if (input[i] == '>')
        {
            if (input[i + 1] == '>')
            {
                token_add(list, TOKEN_APPEND, ">>");
                i += 2;
            }
            else
            {
                token_add(list, TOKEN_OUTPUT, ">");
                i++;
            }

            continue;
        }

        /* Background */
        if (input[i] == '&')
        {
            token_add(list, TOKEN_BACKGROUND, "&");
            i++;
            continue;
        }

        /*
         * Build a normal WORD token.
         */
        char word[1024];
        int j = 0;

        while (input[i] != '\0' &&
               !isspace((unsigned char)input[i]) &&
               input[i] != '|' &&
               input[i] != '<' &&
               input[i] != '>' &&
               input[i] != '&')
        {
            char c = input[i];

            /*
             * Single quote
             */
            if (c == '\'')
            {
                i++;

                while (input[i] != '\0' && input[i] != '\'')
                {
                    if (j < (int)sizeof(word) - 1)
                    {
                        word[j++] = input[i];
                    }

                    i++;
                }

                if (input[i] == '\0')
                {
                    fprintf(stderr,
                            "Lexer Error: Unterminated single quote\n");
                    break;
                }

                /* Skip closing quote */
                i++;
                continue;
            }

            /*
             * Double quote
             */
            if (c == '"')
            {
                i++;

                while (input[i] != '\0' && input[i] != '"')
                {
                    if (input[i] == '\\')
                    {
                        i++;

                        if (input[i] == '\0')
                        {
                            fprintf(stderr,
                                    "Lexer Error: Unterminated escape\n");
                            break;
                        }
                    }

                    if (j < (int)sizeof(word) - 1)
                    {
                        word[j++] = input[i];
                    }

                    i++;
                }

                if (input[i] == '\0')
                {
                    fprintf(stderr,
                            "Lexer Error: Unterminated double quote\n");
                    break;
                }

                /* Skip closing quote */
                i++;
                continue;
            }

            /*
             * Escape character
             */
            if (c == '\\')
            {
                i++;

                if (input[i] == '\0')
                {
                    fprintf(stderr,
                            "Lexer Error: Unterminated escape\n");
                    break;
                }

                if (j < (int)sizeof(word) - 1)
                {
                    word[j++] = input[i];
                }

                i++;
                continue;
            }

            /*
             * Normal character
             */
            if (j < (int)sizeof(word) - 1)
            {
                word[j++] = c;
            }

            i++;
        }

        word[j] = '\0';

        if (j > 0)
        {
            token_add(list, TOKEN_WORD, word);
        }
    }

    token_add(list, TOKEN_END, "END");
}
