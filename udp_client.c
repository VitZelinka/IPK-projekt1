#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include "udp_client.h"
#include "command_parser.h"
#include "arg_parser.h"

uint16_t chars_to_uint16(char char1, char char2);
static void start_processing(UDP_Client_t* client, Cli_config_t* config);
void prepare_socket(UDP_Client_t* client, Cli_config_t* config);
static void handle_command(UDP_Client_t* client, Command_t command, Cli_config_t* config);
static void handle_incoming(UDP_Client_t* client, char* message, Cli_config_t* config);
static void handle_command_message(UDP_Client_t* client, Command_t command, Cli_config_t* config);
static void handle_command_auth(UDP_Client_t* client, Command_t command, Cli_config_t* config);
static void handle_command_join(UDP_Client_t* client, Command_t command, Cli_config_t* config);
static void handle_command_rename(UDP_Client_t* client, Command_t command);
static void handle_command_help();
static void handle_command_error();
void copy_buffer(char* dest, char* src, size_t len);
static void handle_incoming_msg(UDP_Client_t* client, char* message);
static void handle_incoming_reply(UDP_Client_t* client, char* message);
static void handle_incoming_confirm(UDP_Client_t* client, char* message, Cli_config_t* config);
static void handle_incoming_err(UDP_Client_t* client, char* message);
static void handle_incoming_bye(UDP_Client_t* client);
static void send_bye(UDP_Client_t* client);
void send_confirm(UDP_Client_t* client, uint16_t ref_id);


void UDP_client_start(UDP_Client_t* client, int* p2c_pipe, Cli_config_t* config) {
    client->state = UDP_START;
    client->p2c_pipe = p2c_pipe;
    client->msg_counter = 0;
    client->waiting = 0;
    client->retry = 0;
    client->recv_msg_id = -1;

    close(client->p2c_pipe[1]); // Close the write end of the pipe
    
    prepare_socket(client, config);

    start_processing(client, config);

    close(client->p2c_pipe[0]); // Close the read end of the pipe
}


char* UDP_resolve_hostname(char* hostname) {
    struct hostent *host;
    struct in_addr **addr_list;

    if ((host = gethostbyname(hostname)) == NULL) {
        exit(EXIT_FAILURE);
    }

    addr_list = (struct in_addr **)host->h_addr_list;

    return inet_ntoa(*addr_list[0]);
}


void prepare_socket(UDP_Client_t* client, Cli_config_t* config) {
     struct sockaddr_in client_addr;

    if ((client->socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        fprintf(stderr, "ERR: Socket failed to create.\n");
        exit(1);
    }

    // Initialize server address structure
    memset(&client->server_addr, 0, sizeof(client->server_addr));
    client->server_addr.sin_family = AF_INET;
    client->server_addr.sin_port = htons(config->cli_port);
    inet_aton(UDP_resolve_hostname(config->cli_ip), &client->server_addr.sin_addr);

    // Initialize client address structure
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(0); // Let the OS assign a random port
    client_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to a random port
    if (bind(client->socket_fd, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1) {
        fprintf(stderr, "ERR: Socket bind failed.\n");
        exit(1);
    }
}


static void start_processing(UDP_Client_t* client, Cli_config_t* config) {
    fd_set readfds;
    client->timeout.tv_sec = 0;
    client->timeout.tv_usec = config->cli_udp_timeout * 1000;

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
        int activity = select(max_fd + 1, &readfds, NULL, NULL, &(client->timeout));

        if (activity == -1) {
            exit(EXIT_FAILURE);
        } else if (activity == 0 && client->waiting) {
            if (client->retry < config->cli_udp_retries) {
                // Timeout occurred, retry sending the message

                if (sendto(client->socket_fd, client->last_msg, client->last_msg_len, 0, (struct sockaddr *)&client->server_addr, sizeof(client->server_addr)) < 0) {
                    fprintf(stderr, "ERR: Send operation failed.\n");
                    exit(EXIT_FAILURE);
                }

                client->retry++;
                client->timeout.tv_sec = 0;
                client->timeout.tv_usec = config->cli_udp_timeout * 1000;
            } else {
                fprintf(stderr, "ERR: Connection timed out. Maximum retries reached.\n");
                exit(EXIT_FAILURE);
            }
        } else {
            // Check if client socket has data to read
            if (FD_ISSET(client->socket_fd, &readfds)) {
                // Read data from the server
                char buffer[BUFFER_SIZE] = {0};
                int bytes_received;
                struct sockaddr_in from_addr;
                socklen_t addr_len = sizeof(from_addr);

                if ((bytes_received = recvfrom(client->socket_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&from_addr, &addr_len)) > 0) {
                    if (from_addr.sin_addr.s_addr == client->server_addr.sin_addr.s_addr) { // if the server address matches, handle message
                        client->server_addr.sin_port = from_addr.sin_port;
                        buffer[bytes_received] = '\0';
                        handle_incoming(client, buffer, config);
                    }
                } else if (bytes_received == 0) {
                    exit(EXIT_FAILURE);
                } else {
                    exit(EXIT_FAILURE);
                }
            }
        }

        // Check if stdin has data to read
        if (FD_ISSET(client->p2c_pipe[0], &readfds) && !client->waiting) {
            Command_t pipe_command;
            read(client->p2c_pipe[0], &pipe_command, sizeof(pipe_command));

            handle_command(client, pipe_command, config);
        }
    }
}



static void handle_command(UDP_Client_t* client, Command_t command, Cli_config_t* config) {
    switch (command.name) {
        case MESSAGE:
            handle_command_message(client, command, config);
            break;
        
        case AUTH:
            handle_command_auth(client, command, config);
            break;

        case JOIN:
            handle_command_join(client, command, config);
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

static void handle_command_auth(UDP_Client_t* client, Command_t command, Cli_config_t* config) {
    if (client->state == UDP_AUTH || client->state == UDP_START) {
        client->state = UDP_AUTH;

        char auth_msg[BUFFER_SIZE] = {0};
        auth_msg[0] = M_UDP_AUTH;
        auth_msg[1] = (client->msg_counter >> 8) & 0xFF;
        auth_msg[2] = client->msg_counter & 0xFF;
        int writer = 3;
        strcpy(&auth_msg[writer], command.username);
        writer += strlen(command.username);
        auth_msg[writer] = '\0';
        writer++;
        strcpy(&auth_msg[writer], command.display_name);
        writer += strlen(command.display_name);
        auth_msg[writer] = '\0';
        writer++;
        strcpy(&auth_msg[writer], command.secret);
        writer += strlen(command.secret);
        auth_msg[writer] = '\0';

        size_t msg_len = 3 + strlen(command.username) + 1 + strlen(command.display_name) + 1 + strlen(command.secret) + 1;

        copy_buffer(client->last_msg, auth_msg, msg_len);
        client->last_msg_len = msg_len;

        if (sendto(client->socket_fd, auth_msg, msg_len, 0, (struct sockaddr *)&client->server_addr, sizeof(client->server_addr)) < 0) {
            fprintf(stderr, "ERR: Send operation failed.\n");
            exit(EXIT_FAILURE);
        }

        client->waiting = 1;
        client->timeout.tv_sec = 0;
        client->timeout.tv_usec = config->cli_udp_timeout * 1000;
        strcpy(client->display_name, command.display_name);
    } else {
        fprintf(stderr, "ERR: Cannot authenticate in current state.\n");
    }
}

static void handle_command_join(UDP_Client_t* client, Command_t command, Cli_config_t* config) {
    if (client->state == UDP_OPEN) {
        char msg[BUFFER_SIZE] = {0};
        msg[0] = M_UDP_MSG;
        msg[1] = (client->msg_counter >> 8) & 0xFF;
        msg[2] = client->msg_counter & 0xFF;
        int writer = 3;
        strcpy(&msg[writer], command.channel_id);
        writer += strlen(command.channel_id);
        msg[writer] = '\0';
        writer++;
        strcpy(&msg[writer], command.display_name);
        writer += strlen(command.display_name);
        msg[writer] = '\0';

        size_t msg_len = 3 + strlen(command.channel_id) + 1 + strlen(command.display_name) + 1;

        copy_buffer(client->last_msg, msg, msg_len);
        client->last_msg_len = msg_len;

        if (sendto(client->socket_fd, msg, msg_len, 0, (struct sockaddr *)&client->server_addr, sizeof(client->server_addr)) < 0) {
            fprintf(stderr, "ERR: Send operation failed.\n");
            exit(EXIT_FAILURE);
        }

        client->waiting = 1;
        client->timeout.tv_sec = 0;
        client->timeout.tv_usec = config->cli_udp_timeout * 1000;
    } else {
        fprintf(stderr, "ERR: Cannot join channel in current state.\n");
    }
}

static void handle_command_rename(UDP_Client_t* client, Command_t command) {
    strcpy(client->display_name, command.display_name);
}

static void handle_command_message(UDP_Client_t* client, Command_t command, Cli_config_t* config) {
    if (client->state == UDP_OPEN) {
        char msg[BUFFER_SIZE] = {0};
        msg[0] = M_UDP_MSG;
        msg[1] = (client->msg_counter >> 8) & 0xFF;
        msg[2] = client->msg_counter & 0xFF;
        int writer = 3;
        strcpy(&msg[writer], command.display_name);
        writer += strlen(command.display_name);
        msg[writer] = '\0';
        writer++;
        strcpy(&msg[writer], command.message_content);
        writer += strlen(command.message_content);
        msg[writer] = '\0';

        size_t msg_len = 3 + strlen(command.display_name) + 1 + strlen(command.message_content) + 1;

        copy_buffer(client->last_msg, msg, msg_len);
        client->last_msg_len = msg_len;

        if (sendto(client->socket_fd, msg, msg_len, 0, (struct sockaddr *)&client->server_addr, sizeof(client->server_addr)) < 0) {
            fprintf(stderr, "ERR: Send operation failed.\n");
            exit(EXIT_FAILURE);
        }

        client->waiting = 1;
        client->timeout.tv_sec = 0;
        client->timeout.tv_usec = config->cli_udp_timeout * 1000;
        
    } else {
        fprintf(stderr, "ERR: Cannot send message while not connected.\n");
    }
}

static void handle_command_error() {
    fprintf(stderr, "ERR: Unknown command entered.\n");
}

static void handle_command_help() {
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



static void handle_incoming(UDP_Client_t* client, char* message, Cli_config_t* config) {

    switch (message[0]) {
        case M_UDP_CONFIRM:
            handle_incoming_confirm(client, message, config);
            break;
        
        case M_UDP_REPLY:
            handle_incoming_reply(client, message);
            break;

        case M_UDP_MSG:
            handle_incoming_msg(client, message);
            break;

        default:
            if ((u_int16_t) message[0] == (u_int16_t)M_UDP_ERR) {
                handle_incoming_err(client, message);
                break;
            } else if ((u_int16_t)message[0] == (u_int16_t)M_UDP_BYE) {
                handle_incoming_bye(client);
                break;
            } else {
                send_bye(client);
                exit(EXIT_FAILURE);
                break;
            }
    }
}

static void handle_incoming_confirm(UDP_Client_t* client, char* message, Cli_config_t* config) {
    uint16_t ref_id = chars_to_uint16(message[1], message[2]);
    if (client->waiting && (ref_id == client->msg_counter)) {
        client->waiting = 0;
        client->msg_counter++;
        client->retry = 0;
        client->timeout.tv_sec = 0;
        client->timeout.tv_usec = config->cli_udp_timeout * 1000;
    }
}

static void handle_incoming_msg(UDP_Client_t* client, char* message) {
    uint16_t ref_id = chars_to_uint16(message[1], message[2]);
    if (ref_id > client->recv_msg_id) {
        client->recv_msg_id = ref_id;
        fprintf(stdout, "%s: %s\n", &(message[3]), &(message[3 + strlen(&message[3]) + 1]));
    }
    send_confirm(client, ref_id);
    return;
}

static void handle_incoming_reply(UDP_Client_t* client, char* message) {
    uint16_t ref_id = chars_to_uint16(message[4], message[5]);

    if (ref_id != (client->msg_counter - 1)) {
        send_confirm(client, ref_id);
        return;
    }

    if (message[3]) {
        client->state = UDP_OPEN;
        fprintf(stderr, "Success: %s\n", &message[6]);
    } else {
        fprintf(stderr, "Failure: %s\n", &message[6]);
    }

    send_confirm(client, ref_id);
}

static void handle_incoming_err(UDP_Client_t* client, char* message) {
    uint16_t ref_id = chars_to_uint16(message[1], message[2]);

    fprintf(stderr, "ERR FROM %s: %s\n", &(message[3]), &(message[3 + strlen(&message[3]) + 1]));
    send_confirm(client, ref_id);
    //send_bye(client);
    //close_socket()
    exit(EXIT_SUCCESS);
}

static void handle_incoming_bye(UDP_Client_t* client) {
    //disconnect_from_server(client);
    exit(EXIT_SUCCESS);
}



void close_socket(UDP_Client_t* client) {
    if (shutdown(client->socket_fd, SHUT_RDWR) == -1) {
        fprintf(stderr, "Shutdown failed\n");
        exit(EXIT_FAILURE);
    }

    if (close(client->socket_fd) == -1) {
        fprintf(stderr, "Socket closing failed\n");
        exit(EXIT_FAILURE);
    }
}


static void send_bye(UDP_Client_t* client) {
    char bye_msg[] = "BYE\r\n";
    if (send(client->socket_fd, bye_msg, strlen(bye_msg), 0) < 0) {
        fprintf(stderr, "Send failed\n");
        exit(EXIT_FAILURE);
    }
}

void send_confirm(UDP_Client_t* client, uint16_t ref_id) {
    char confirm_msg[3];
    confirm_msg[0] = M_UDP_CONFIRM;
    confirm_msg[1] = (ref_id >> 8) & 0xFF;
    confirm_msg[2] = ref_id & 0xFF;

    if (sendto(client->socket_fd, confirm_msg, 3, 0, (struct sockaddr *)&client->server_addr, sizeof(client->server_addr)) < 0) {
        fprintf(stderr, "ERR: Send operation failed.\n");
        exit(EXIT_FAILURE);
    }
}



uint16_t chars_to_uint16(char char1, char char2) {
    uint16_t result = (uint16_t)((unsigned char)char1 << 8) | (unsigned char)char2;
    return result;
}

void copy_buffer(char* dest, char* src, size_t len) {
    for (size_t i = 0; i < len; i++) {
        dest[i] = src[i];
    }
}
