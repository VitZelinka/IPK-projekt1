#include "command_parser.h"
#include <string.h>
#include <stdio.h>

void parse_command(char* stdin_line, Command_t *command) {
    memset(command->channel_id, 0, MAX_ANYNAME_LEN+1);
    memset(command->display_name, 0, MAX_ANYNAME_LEN+1);
    memset(command->message_content, 0, MAX_MESSAGE_LEN+1);
    memset(command->username, 0, MAX_ANYNAME_LEN+1);
    memset(command->secret, 0, SECRET_LEN);


    if (stdin_line[0] == '/') { // command mode
        char buffer[MAX_ANYNAME_LEN+1] = {0};
        int i = 0;
        
        while (stdin_line[i] != ' ' && stdin_line[i] != '\n' && i < MAX_ANYNAME_LEN) { // load command name, ex. /auth
            buffer[i] = stdin_line[i];
            i++;
        }

        if (strcmp(buffer, "/auth") == 0) {
            
            if (stdin_line[i] != ' ') {
                command->name = ERROR;
                return;
            }
            
            command->name = AUTH;
            int offset = i + 1;
            i = 0;
            memset(buffer, 0, MAX_ANYNAME_LEN+1);

            while (stdin_line[i + offset] != ' ' && stdin_line[i + offset] != '\n' && i < MAX_ANYNAME_LEN) { // load Username for AUTH
                buffer[i] = stdin_line[i + offset];
                i++;
            }
            
            if (stdin_line[i + offset] != ' ') {
                command->name = ERROR;
                return;
            }
            strcpy(command->username, buffer);

            offset += i + 1;
            i = 0;
            char secret_buffer[SECRET_LEN] = {0};

            while (stdin_line[i + offset] != ' ' && stdin_line[i + offset] != '\n' && i < SECRET_LEN) { // load Secret for AUTH
                secret_buffer[i] = stdin_line[i + offset];
                i++;
            }
            
            if (stdin_line[i + offset] != ' ') {
                command->name = ERROR;
                return;
            }
            strcpy(command->secret, secret_buffer);


            offset += i + 1;
            i = 0;
            memset(buffer, 0, MAX_ANYNAME_LEN+1);

            while (stdin_line[i + offset] != ' ' && stdin_line[i + offset] != '\n' && i < MAX_ANYNAME_LEN) { // load DisplayName for AUTH
                buffer[i] = stdin_line[i + offset];
                i++;
            }
            
            if (stdin_line[i + offset] != '\n') {
                command->name = ERROR;
                return;
            }
            strcpy(command->display_name, buffer);

            return;

        } else if (strcmp(buffer, "/join") == 0) {

            if (stdin_line[i] != ' ') {
                command->name = ERROR;
                return;
            }
            
            command->name = JOIN;
            int offset = i + 1;
            i = 0;
            memset(buffer, 0, MAX_ANYNAME_LEN+1);

            while (stdin_line[i + offset] != ' ' && stdin_line[i + offset] != '\n' && i < MAX_ANYNAME_LEN) { // load ChannelID for JOIN
                buffer[i] = stdin_line[i + offset];
                i++;
            }
            
            if (stdin_line[i + offset] != '\n') {
                command->name = ERROR;
                return;
            }
            strcpy(command->channel_id, buffer);
            
            return;

        } else if (strcmp(buffer, "/rename") == 0) {
            
            if (stdin_line[i] != ' ') {
                command->name = ERROR;
                return;
            }
            
            command->name = RENAME;
            int offset = i + 1;
            i = 0;
            memset(buffer, 0, MAX_ANYNAME_LEN+1);

            while (stdin_line[i + offset] != ' ' && stdin_line[i + offset] != '\n' && i < MAX_ANYNAME_LEN) { // load DisplayName for RENAME
                buffer[i] = stdin_line[i + offset];
                i++;
            }
            
            if (stdin_line[i + offset] != '\n') {
                command->name = ERROR;
                return;
            }
            strcpy(command->display_name, buffer);
            
            return;

        } else if (strcmp(buffer, "/help") == 0) {
            command->name = HELP;
            return;
        } else {
            command->name = ERROR;
            return;
        }

    } else { // message mode
        command->name = MESSAGE;
        strcpy(command->message_content, stdin_line);
        for (size_t index = 0; index < strlen(command->message_content); index++) {
            if (command->message_content[index] == '\n') {
                command->message_content[index] = '\0';
            }
        }
        return;
    }
}

