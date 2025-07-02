
#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include <string.h>

#define MAX_MESSAGE_LEN 1400
#define MAX_ANYNAME_LEN 20
#define SECRET_LEN 128

typedef enum {
    AUTH,
    JOIN,
    RENAME,
    HELP,
    MESSAGE,
    ERROR
} CommandName;

typedef struct {
    CommandName name;
    char message_content[MAX_MESSAGE_LEN+1];
    char username[MAX_ANYNAME_LEN+1];
    char channel_id[MAX_ANYNAME_LEN+1];
    char secret[SECRET_LEN];
    char display_name[MAX_ANYNAME_LEN+1];
} Command_t;


void parse_command(char* stdin_line, Command_t *command);


#endif