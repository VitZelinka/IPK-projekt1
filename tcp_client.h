
#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <string.h>

#include "arg_parser.h"
#include "command_parser.h"

#define BUFFER_SIZE 2048

typedef enum {
    TCP_START,
    TCP_AUTH,
    TCP_OPEN,
    TCP_ERROR,
    TCP_END
} TCP_State;

typedef struct {
    TCP_State state;
    int* p2c_pipe;
    int socket_fd;
    char display_name[MAX_ANYNAME_LEN+1];
} TCP_Client_t;

void TCP_client_start(TCP_Client_t* client, int* p2c_pipe, Cli_config_t* config);

#endif