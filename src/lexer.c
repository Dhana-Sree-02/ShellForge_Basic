#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "../include/lexer.h"

static void add_word(token_list *list, char *word, int *length)
{
    if (*length == 0)
        return;

    word[*length] = '\0';

    token_add(list, TOKEN_WORD, word);

    *length = 0;
}

void lexer(const char *input, token_list *list)
{
    token_list_init(list);

    int i = 0;
    char word[MAX_TOKEN_LEN];
    int length = 0;

    while (input[i] != '\0')
    {
        char c = input[i];

        /* End of input */
        if (c == '\n')
        {
            add_word(list, word, &length);
            break;
        }

        /* Whitespace */
        if (isspace((unsigned char)c))
        {
            add_word(list, word, &length);
            i++;
            continue;
        }

        /* Pipe */
        if (c == '|')
        {
            add_word(list, word, &length);
            token_add(list, TOKEN_PIPE, "|");
            i++;
            continue;
        }

        /* Input redirection */
        if (c == '<')
        {
            add_word(list, word, &length);
            token_add(list, TOKEN_INPUT, "<");
            i++;
            continue;
        }

        /* Output redirection / append */
        if (c == '>')
        {
            add_word(list, word, &length);

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
        if (c == '&')
        {
            add_word(list, word, &length);
            token_add(list, TOKEN_BACKGROUND, "&");
            i++;
            continue;
        }

        /* Single quoted string */
        if (c == '\'')
        {
            i++;

            while (input[i] != '\0' && input[i] != '\'')
            {
                if (length < MAX_TOKEN_LEN - 1)
                    word[length++] = input[i];

                i++;
            }

            if (input[i] == '\'')
            {
                i++;
            }
            else
            {
                fprintf(stderr,
                        "Lexer Error: Unterminated single quote\n");
                return;
            }

            continue;
        }

        /* Double quoted string */
        if (c == '"')
        {
            i++;

            while (input[i] != '\0' && input[i] != '"')
            {
                if (input[i] == '\\' && input[i + 1] != '\0')
                {
                    i++;

                    if (length < MAX_TOKEN_LEN - 1)
                        word[length++] = input[i];

                    i++;
                    continue;
                }

                if (length < MAX_TOKEN_LEN - 1)
                    word[length++] = input[i];

                i++;
            }

            if (input[i] == '"')
            {
                i++;
            }
            else
            {
                fprintf(stderr,
                        "Lexer Error: Unterminated double quote\n");
                return;
            }

            continue;
        }

        /* Escape character */
        if (c == '\\')
        {
            /*
             * Keep the escaped character as part of the
             * current word.
             */
            if (input[i + 1] != '\0')
            {
                i++;

                if (length < MAX_TOKEN_LEN - 1)
                    word[length++] = input[i];

                i++;
            }
            else
            {
                if (length < MAX_TOKEN_LEN - 1)
                    word[length++] = '\\';

                i++;
            }

            continue;
        }

        /* Normal character */
        if (length < MAX_TOKEN_LEN - 1)
            word[length++] = c;

        i++;
    }

    add_word(list, word, &length);

    token_add(list, TOKEN_END, "END");
}
