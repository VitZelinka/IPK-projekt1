#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "tcp_client.h"
#include "command_parser.h"
#include "arg_parser.h"

void start_processing(TCP_Client_t* client);
void handle_incoming(TCP_Client_t* client, char* message);
void handle_incoming_msg(char* message, int i);
void handle_incoming_reply(TCP_Client_t* client, char* message, int i);
void handle_incoming_err(TCP_Client_t* client, char* message, int i);
void handle_incoming_bye(TCP_Client_t* client);
void send_bye(TCP_Client_t* client);
void connect_to_server(TCP_Client_t* client, Cli_config_t* config);
void disconnect_from_server(TCP_Client_t* client);
void handle_command(TCP_Client_t* client, Command_t command);
void handle_command_auth(TCP_Client_t* client, Command_t command);
void handle_command_join(TCP_Client_t* client, Command_t command);
void handle_command_rename(TCP_Client_t* client, Command_t command);
void handle_command_message(TCP_Client_t* client, Command_t command);
void handle_command_error();
void handle_command_help();


// Client initialization
void TCP_client_start(TCP_Client_t* client, int* p2c_pipe, Cli_config_t* config) {
    client->state = TCP_START;
    client->p2c_pipe = p2c_pipe;

    close(client->p2c_pipe[1]); // Close the write end of the pipe
    
    connect_to_server(client, config);

    start_processing(client);

    close(client->p2c_pipe[0]); // Close the read end of the pipe
}

// Finds and returns IP of a hostname
char* TCP_resolve_hostname(char* hostname) {
    struct hostent *host;
    struct in_addr **addr_list;

    if ((host = gethostbyname(hostname)) == NULL) {
        exit(EXIT_FAILURE);
    }

    addr_list = (struct in_addr **)host->h_addr_list;

    return inet_ntoa(*addr_list[0]);
}

// Creates a socket and connect to a server
void connect_to_server(TCP_Client_t* client, Cli_config_t* config) {
    // Create TCP socket
    if ((client->socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "ERR: Socket creation error.\n");
        exit(EXIT_FAILURE);
    }

    // Initialize server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config->cli_port);
    if (inet_pton(AF_INET, TCP_resolve_hostname(config->cli_ip), &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "ERR: IP creation error.\n");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(client->socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "ERR: Failed to connect to server.\n");
        exit(EXIT_FAILURE);
    }
}

// Main loop of the client, checks for incoming commands from user and messages from server
void start_processing(TCP_Client_t* client) {
    fd_set readfds;
    pid_t parent_pid = getppid();

    while (1) {
        // Clear the set
        FD_ZERO(&readfds);

        // Add stdin file descriptor to the set
        FD_SET(client->p2c_pipe[0], &readfds);

        // Add client socket file descriptor to the set
        FD_SET(client->socket_fd, &readfds);

        // Find the maximum file descriptor
        int max_fd = (client->socket_fd > client->p2c_pipe[0]) ? client->socket_fd : client->p2c_pipe[0];

        // Wait for activity on any of the sockets
        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) == -1) {
            fprintf(stderr, "ERR: Select function error.\n");
            exit(EXIT_FAILURE);
        }

        // Check if client socket has data to read
        if (FD_ISSET(client->socket_fd, &readfds)) {
            // Read data from the server
            char buffer[BUFFER_SIZE] = {0};
            int bytes_received;
            if ((bytes_received = recv(client->socket_fd, buffer, BUFFER_SIZE, 0)) > 0) {
                buffer[bytes_received] = '\0';
                handle_incoming(client, buffer);
            } else if (bytes_received == 0) {
                fprintf(stderr, "ERR: Server disconnected.\n");
                exit(EXIT_FAILURE);
            } else {
                fprintf(stderr, "ERR: Receive data from server error.\n");
                exit(EXIT_FAILURE);
            }
        }

        // Check if stdin has data to read
        if (FD_ISSET(client->p2c_pipe[0], &readfds)) {
            Command_t pipe_command;
            read(client->p2c_pipe[0], &pipe_command, sizeof(pipe_command));

            handle_command(client, pipe_command);
        }

        if (kill(parent_pid, 0) == -1) {
            if (errno == ESRCH) {
                if (client->state == TCP_OPEN){
                    send_bye(client);
                    disconnect_from_server(client);
                }
                exit(EXIT_SUCCESS);
            }
        }
    }
}


// Handles incoming commands from user
void handle_command(TCP_Client_t* client, Command_t command) {
    switch (command.name) {
        case MESSAGE:
            handle_command_message(client, command);
            break;
        
        case AUTH:
            handle_command_auth(client, command);
            break;

        case JOIN:
            handle_command_join(client, command);
            break;
        
        case RENAME:
            handle_command_rename(client, command);
            break;
        
        case ERROR:
            handle_command_error();
            break;

        case HELP:
            handle_command_help();
            break;

        default:
            break;
    }
}

void handle_command_auth(TCP_Client_t* client, Command_t command) {
    if (client->state == TCP_AUTH || client->state == TCP_START) {
        client->state = TCP_AUTH;

        char auth_msg[BUFFER_SIZE] = {0};
        snprintf(auth_msg, BUFFER_SIZE, "AUTH %s AS %s USING %s\r\n", command.username, command.display_name, command.secret);

        if (send(client->socket_fd, auth_msg, strlen(auth_msg), 0) < 0) {
            fprintf(stderr, "ERR: Send operation failed.\n");
            exit(EXIT_FAILURE);
        }

        strcpy(client->display_name, command.display_name);
    } else {
        fprintf(stderr, "ERR: Cannot authenticate in current state.\n");
    }
}

void handle_command_join(TCP_Client_t* client, Command_t command) {
    if (client->state == TCP_OPEN) {
        char msg[BUFFER_SIZE] = {0};
        snprintf(msg, BUFFER_SIZE, "JOIN %s AS %s\r\n", command.channel_id, client->display_name);

        if (send(client->socket_fd, msg, strlen(msg), 0) < 0) {
            fprintf(stderr, "ERR: Send operation failed.\n");
            exit(EXIT_FAILURE);
        }
    } else {
        fprintf(stderr, "ERR: Cannot join channel in current state.\n");
    }
}

void handle_command_rename(TCP_Client_t* client, Command_t command) {
    strcpy(client->display_name, command.display_name);
}

void handle_command_message(TCP_Client_t* client, Command_t command) {
    if (client->state == TCP_OPEN) {
        char msg[BUFFER_SIZE] = {0};
        snprintf(msg, BUFFER_SIZE, "MSG FROM %s IS %s\r\n", client->display_name, command.message_content);

        if (send(client->socket_fd, msg, strlen(msg), 0) < 0) {
            fprintf(stderr, "ERR: Send operation failed.\n");
            exit(EXIT_FAILURE);
        }
        
    } else {
        fprintf(stderr, "ERR: Cannot send message while not connected.\n");
    }
}

void handle_command_error() {
    fprintf(stderr, "ERR: Unknown command entered.\n");
}

void handle_command_help() {
    fprintf(stdout, "These are all available commands:\n");
    fprintf(stdout, "/help\n");
    fprintf(stdout, "- prints out this help message\n");
    fprintf(stdout, "/auth {Username} {Secret} {DisplayName}\n");
    fprintf(stdout, "- sends authentification request to the server, if connected\n");
    fprintf(stdout, "/join {ChannelID}\n");
    fprintf(stdout, "- sends request to change channel to the one specified\n");
    fprintf(stdout, "/rename {DisplayName}\n");
    fprintf(stdout, "- changes the publicly visible username\n");
}


// Handles incoming messages from server
void handle_incoming(TCP_Client_t* client, char* message) { // ERR, REPLY, MSG, BYE
    char buffer[MAX_ANYNAME_LEN+1] = {0};
    int i = 0;
        
    while (message[i] != ' ' && message[i] != '\n' && message[i] != '\r' && i < MAX_ANYNAME_LEN) { // load incoming message header
        buffer[i] = message[i];
        i++;
    }

    if (strcmp(buffer, "MSG") == 0) {
        handle_incoming_msg(message, i);

    } else if (strcmp(buffer, "REPLY") == 0) {
        handle_incoming_reply(client, message, i);

    } else if (strcmp(buffer, "ERR") == 0) {
        handle_incoming_err(client, message, i);

    } else if (strcmp(buffer, "BYE") == 0) {
        handle_incoming_bye(client);

    } else {
        send_bye(client);
        disconnect_from_server(client);
        exit(EXIT_FAILURE);
    }
}

void handle_incoming_msg(char* message, int i) {
    char buffer[MAX_ANYNAME_LEN+1] = {0};
    char message_buffer[MAX_MESSAGE_LEN+1] = {0};
    i = i + 6; // discard not needed part of message
    
    int j = 0;
    while (message[i] != ' ' && message[i] != '\n' && message[i] != '\r' && i < MAX_ANYNAME_LEN) { // load DisplayName to buffer
        buffer[j] = message[i];
        i++;
        j++;
    }
    i = i + 4; // discard not needed part of message

    j = 0;
    while (message[i] != '\n' && message[i] != '\r' && i < MAX_MESSAGE_LEN) { // load MessageContent to message_buffer
        message_buffer[j] = message[i];
        i++;
        j++;
    }

    fprintf(stdout, "%s: %s\n", buffer, message_buffer);
    return;
}

void handle_incoming_reply(TCP_Client_t* client, char* message, int i) {
    char buffer[MAX_ANYNAME_LEN+1] = {0};
    char message_buffer[MAX_MESSAGE_LEN+1] = {0};
    i = i + 1; // discard not needed part of message
    
    int j = 0;
    while (message[i] != ' ' && message[i] != '\n' && message[i] != '\r' && i < MAX_ANYNAME_LEN) { // load OK/NOK to buffer
        buffer[j] = message[i];
        i++;
        j++;
    }
    i = i + 4; // discard not needed part of message

    j = 0;
    while (message[i] != '\n' && message[i] != '\r' && i < MAX_MESSAGE_LEN) { // load MessageContent to message_buffer
        message_buffer[j] = message[i];
        i++;
        j++;
    }

    if (strcmp(buffer, "OK") == 0) {
        client->state = TCP_OPEN;
        fprintf(stderr, "Success: %s\n", message_buffer);
    } else {
        fprintf(stderr, "Failure: %s\n", message_buffer);
    }
    return;
}

void handle_incoming_err(TCP_Client_t* client, char* message, int i) {
    char buffer[MAX_ANYNAME_LEN+1] = {0};
    char message_buffer[MAX_MESSAGE_LEN+1] = {0};
    i = i + 6; // discard not needed part of message
    
    int j = 0;
    while (message[i] != ' ' && message[i] != '\n' && message[i] != '\r' && i < MAX_ANYNAME_LEN) { // load DisplayName to buffer
        buffer[j] = message[i];
        i++;
        j++;
    }
    i = i + 4; // discard not needed part of message

    j = 0;
    while (message[i] != '\n' && message[i] != '\r' && i < MAX_MESSAGE_LEN) { // load MessageContent to message_buffer
        message_buffer[j] = message[i];
        i++;
        j++;
    }

    fprintf(stderr, "ERR FROM %s: %s\n", buffer, message_buffer);
    send_bye(client);
    disconnect_from_server(client);
    exit(EXIT_SUCCESS);
}

void handle_incoming_bye(TCP_Client_t* client) {
    disconnect_from_server(client);
    exit(EXIT_SUCCESS);
}



void disconnect_from_server(TCP_Client_t* client) {
    if (shutdown(client->socket_fd, SHUT_RDWR) == -1) {
        fprintf(stderr, "ERR: Socket shutdown error.\n");
        exit(EXIT_FAILURE);
    }

    if (close(client->socket_fd) == -1) {
        fprintf(stderr, "ERR: Socket close error.\n");
        exit(EXIT_FAILURE);
    }
}


void send_bye(TCP_Client_t* client) {
    char bye_msg[] = "BYE\r\n";
    if (send(client->socket_fd, bye_msg, strlen(bye_msg), 0) < 0) {
        fprintf(stderr, "ERR: Send to server failed.\n");
        exit(EXIT_FAILURE);
    }
}