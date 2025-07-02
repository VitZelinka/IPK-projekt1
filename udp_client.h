
#ifndef UDP_CLIENT_H
#define UDP_CLIENT_H

#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include "arg_parser.h"
#include "command_parser.h"

#define BUFFER_SIZE 2048

#define M_UDP_CONFIRM 0x00
#define M_UDP_REPLY 0x01
#define M_UDP_AUTH 0x02
#define M_UDP_JOIN 0x03
#define M_UDP_MSG 0x04
#define M_UDP_ERR 0xFE
#define M_UDP_BYE 0xFF

typedef enum {
    UDP_START,
    UDP_AUTH,
    UDP_OPEN,
    UDP_ERROR,
    UDP_END
} UDP_State;

typedef struct {
    UDP_State state;
    int* p2c_pipe;
    int socket_fd;
    struct sockaddr_in server_addr;
    uint16_t msg_counter;
    int recv_msg_id;
    int waiting;
    struct timeval timeout;
    size_t last_msg_len;
    int retry;
    char last_msg[BUFFER_SIZE];
    char display_name[MAX_ANYNAME_LEN+1];
} UDP_Client_t;

void UDP_client_start(UDP_Client_t* client, int* p2c_pipe, Cli_config_t* config);

#endif