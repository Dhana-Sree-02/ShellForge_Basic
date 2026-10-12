#ifndef HISTORY_H
#define HISTORY_H

void history_init(void);

void history_add_command(const char *command);

void history_show(void);

void history_cleanup(void);

#endif
