#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "token.h"

void token_list_init(TokenList *list)
{
    list->count = 0;
    list->capacity = 16;

    list->tokens = malloc(sizeof(Token) * list->capacity);

    if (list->tokens == NULL)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
}

void token_add(TokenList *list, TokenType type, const char *value)
{
    if (list->count >= list->capacity)
    {
        list->capacity *= 2;

        list->tokens = realloc(
            list->tokens,
            sizeof(Token) * list->capacity
        );

        if (list->tokens == NULL)
        {
            perror("realloc");
            exit(EXIT_FAILURE);
        }
    }

    list->tokens[list->count].type = type;
    list->tokens[list->count].value = strdup(value);

    if (list->tokens[list->count].value == NULL)
    {
        perror("strdup");
        exit(EXIT_FAILURE);
    }

    list->count++;
}

void token_list_print(const TokenList *list)
{
    for (int i = 0; i < list->count; i++)
    {
        printf("TOKEN: ");

        switch (list->tokens[i].type)
        {
            case TOKEN_WORD:
                printf("WORD");
                break;

            case TOKEN_PIPE:
                printf("PIPE");
                break;

            case TOKEN_INPUT:
                printf("INPUT");
                break;

            case TOKEN_OUTPUT:
                printf("OUTPUT");
                break;

            case TOKEN_APPEND:
                printf("APPEND");
                break;

            case TOKEN_BACKGROUND:
                printf("BACKGROUND");
                break;

            case TOKEN_END:
                printf("END");
                break;
        }

        printf(" -> %s\n", list->tokens[i].value);
    }
}

void token_list_free(TokenList *list)
{
    for (int i = 0; i < list->count; i++)
    {
        free(list->tokens[i].value);
    }

    free(list->tokens);

    list->tokens = NULL;
    list->count = 0;
    list->capacity = 0;
}
