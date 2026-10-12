#ifndef TOKEN_H
#define TOKEN_H

#define MAX_TOKENS 128
#define MAX_TOKEN_LEN 256

typedef enum {
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_INPUT,
    TOKEN_OUTPUT,
    TOKEN_APPEND,
    TOKEN_BACKGROUND,
    TOKEN_END
} token_type;

typedef struct {
    token_type type;
    char text[MAX_TOKEN_LEN];
} token;

typedef struct {
    token tokens[MAX_TOKENS];
    int count;
} token_list;

void token_list_init(token_list *list);

void token_add(token_list *list,
               token_type type,
               const char *text);

const char *token_name(token_type type);

void token_print(const token_list *list);

#endif
