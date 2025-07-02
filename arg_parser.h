
#ifndef ARG_PARSER_H
#define ARG_PARSER_H

#include <string.h>
#include <stdint.h>

typedef struct {
    int result;
    void* val;
} Arg_t;

typedef struct {
    char* cli_protocol;
    char* cli_ip;
    uint16_t cli_port;
    uint16_t cli_udp_timeout;
    uint8_t cli_udp_retries;
} Cli_config_t;


Arg_t get_arg(char* arg_name, int argc, char* argv[]);

Cli_config_t get_cli_config(int argc, char* argv[]);


#endif