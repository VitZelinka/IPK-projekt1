#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#include "arg_parser.h"
#include "command_parser.h"
#include "tcp_client.h"
#include "udp_client.h"

int p2c_pipe[2]; 

// Signal handler function to handle SIGINT (Ctrl+C)
void sigint_handler(int signum) {
    close(p2c_pipe[0]);
    close(p2c_pipe[1]);
    exit(EXIT_SUCCESS);
}

void print_help() {
    printf("-t - Transport protocol used for connection (tcp/udp)\n");
    printf("-s - Server IP or hostname\n");
    printf("-p - Server port\n");
    printf("-d - UDP confirmation timeout\n");
    printf("-r - Maximum number of UDP retransmissions\n");
    printf("-h - Prints program help output and exits\n");
}


int main( int argc, char *argv[] )  {
    if (get_arg("-h", argc, argv).result) {
        print_help();
        return 0;
    }

    // Set up signal handler for SIGINT
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        exit(EXIT_FAILURE);
    }

    Cli_config_t config = get_cli_config(argc, argv);

    if (pipe(p2c_pipe) == -1) exit(EXIT_FAILURE);

    pid_t pid = fork(); // fork the process
    if (pid == -1) exit(EXIT_FAILURE);

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags == -1) {
        return 1;
    }

    if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) == -1) {
        return 1;
    }


    if (pid == 0) { // Child process
        if (strcmp(config.cli_protocol, "tcp") == 0) {
            TCP_Client_t client; // Starts the TCP client
            TCP_client_start(&client, p2c_pipe, &config);
        } else {
            UDP_Client_t client; // Starts the UDP client
            UDP_client_start(&client, p2c_pipe, &config);
        }

    } else { // Parent process
        char buffer[BUFFER_SIZE] = {0}; // Buffer to store the input
        
        // Read a line of input from stdin
        while (1) {
            int status;
            pid_t child_pid = waitpid(pid, &status, WNOHANG); // Check if child has terminated

            if (child_pid == -1) {
                fprintf(stderr, "ERR: Fork error.\n");
                return 1;
            } else if (child_pid == 0) {
                //Child process has not terminated yet
            } else {
                if (WIFEXITED(status)) {
                    exit(EXIT_SUCCESS);
                } else if (WIFSIGNALED(status)) {
                    exit(EXIT_FAILURE);
                }
                break;
            }

            if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
                continue;
            }

            Command_t command; 
            parse_command(buffer, &command);

            write(p2c_pipe[1], &command, sizeof(command)); // Send command to child process
        }
    }

    return 0;
}