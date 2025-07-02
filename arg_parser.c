#include "arg_parser.h"
#include "string.h"
#include <stdint.h>
#include <stdlib.h>



Arg_t get_arg(char* arg_name, int argc, char* argv[]) {

    Arg_t arg;

    for (int i = 1; i < argc; i++) {

        if (strcmp(arg_name, argv[i]) == 0) {
            arg.result = 1;

            if (i + 1 < argc && argv[i+1][0] != '-') {
                arg.val = argv[i+1];
            } else {
                arg.val = '\0';
            }

            return arg;
        }
    }

    arg.result = 0;
    arg.val = '\0';
    return arg;
}


Cli_config_t get_cli_config(int argc, char* argv[]) {
    Cli_config_t config;
    config.cli_port = 4567;
    config.cli_udp_timeout = 250;
    config.cli_udp_retries = 3;

    config.cli_protocol = (char*) get_arg("-t", argc, argv).val;
    config.cli_ip = (char*) get_arg("-s", argc, argv).val;

    if (get_arg("-p", argc, argv).result) {
        config.cli_port = (uint16_t) atoi(get_arg("-p", argc, argv).val);
    }

    if (get_arg("-d", argc, argv).result) {
        config.cli_udp_timeout = (uint16_t) atoi(get_arg("-d", argc, argv).val);
    }

    if (get_arg("-r", argc, argv).result) {
        config.cli_udp_retries = (uint8_t) atoi(get_arg("-r", argc, argv).val);
    }

    return config;
}