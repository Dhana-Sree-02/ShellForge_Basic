#ifndef PARSER_H
#define PARSER_H

#include "token.h"

#define MAX_COMMANDS 16
#define MAX_ARGS 64

typedef struct
{
    char *argv[MAX_ARGS];

    int argc;

    char *input;
    char *output;

    int append;
    int background;

} command_t;

typedef struct
{
    command_t commands[MAX_COMMANDS];

    int command_count;

} pipeline_t;

void pipeline_init(pipeline_t *pipeline);

int parse(const token_list *tokens,
          pipeline_t *pipeline);

void pipeline_print(const pipeline_t *pipeline);

void pipeline_free(pipeline_t *pipeline);

#endif
