#ifndef TOKEN_H
#define TOKEN_H

typedef enum
{
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_INPUT,
    TOKEN_OUTPUT,
    TOKEN_APPEND,
    TOKEN_BACKGROUND,
    TOKEN_END
} TokenType;

typedef struct
{
    TokenType type;
    char *value;
} Token;

typedef struct
{
    Token *tokens;
    int count;
    int capacity;
} TokenList;

void token_list_init(TokenList *list);
void token_add(TokenList *list, TokenType type, const char *value);
void token_list_print(const TokenList *list);
void token_list_free(TokenList *list);

#endif
