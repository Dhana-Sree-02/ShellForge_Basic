#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/history.h>
#include <readline/readline.h>
int main(void)
{
 // Display a welcome banner when the shell starts
 printf("=====================================\n");
 printf("Shellforge \n");
 printf(" A Unix Style Shell written in C\n");
 printf("=====================================\n");
 char *line;
 while (1)
 {
 line = readline("shellforge$ ");
 if (line == NULL)
 {
 printf("\nGoodbye!\n");
 break;
 }
 if (strlen(line) == 0)
 {
 free(line);
 continue;
 }
 add_history(line);
 if (strcmp(line, "exit") == 0)
 {
 free(line);
 printf("Exiting...\n");
 break;
 }
 printf(" YOU ENTERED : %s\n", line);
 free(line);
 }
 return 0;
}
