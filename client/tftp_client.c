#include "tftp.h"
#include "tftp_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>

/*globally assigning mode*/
char transfer_mode[20] = "default";
int  connect_flag = 0; 

void print_help(void)
{
    printf("\nAvailable Commands:\n");
    printf("-----------------------------------------\n");
    printf("help                 - Show this help menu\n");
    printf("connect <ip>         - Connect to TFTP server\n");
    printf("put <filename>       - Upload file to server\n");
    printf("get <filename>       - Download file from server\n");
    printf("mode octet           - Set transfer mode to octet\n");
    printf("mode netascii        - Set transfer mode to netascii\n");
    printf("bye                  - Disconnect and exit\n");
    printf("-----------------------------------------\n\n");
}

int main() 
{
    char command[256];

    tftp_client_t client;
    memset(&client, 0, sizeof(client));  // Initialize client structure
    client.sockfd = -1;

    /*Main loop for command-line interface*/
    while (1){
        printf("\ntftp> ");
        fflush(stdout);

        if(fgets(command, sizeof(command), stdin) == NULL)
            break;
        /*Remove newline character*/
        command[strcspn(command, "\n")] = '\0';

        /* Ignore empty command */
        if (strlen(command) == 0)
            continue;

        /*Process the command*/
        process_command(&client, command);
    }

    return 0;
}

// Function to process commands
void process_command(tftp_client_t *client, char *command) 
{
    char cmd[30], arg[256];
    memset(cmd, 0, sizeof(cmd));
    memset(arg, 0, sizeof(arg));

    if(sscanf(command, "%s %s", cmd, arg) < 1)
        return;

    if(strcmp(cmd, "help") == 0){
        print_help();
    }
    else if(strcmp(cmd, "connect") == 0){
        if (strlen(arg) == 0)
        {
            printf("Usage: connect <ip>\n");
            return;
        }
        connect_to_server(client, arg, PORT);
        connect_flag = 1; 
    }
    /*client uploads file*/
    else if(strcmp(cmd, "put") == 0){
        if(connect_flag == 0) 
        { 
            printf("connection to server not done\n"); 
            return; 
        } 

        if (strlen(arg) == 0)
        {
            printf("Usage: put <filename>\n");
            return;
        }
        put_file(client, arg);
    }
    /*client downloads file*/
    else if(strcmp(cmd, "get") == 0){
        if(connect_flag == 0) 
        { 
            printf("connection to server not done\n"); 
            return; 
        } 

        if (strlen(arg) == 0)
        {
            printf("Usage: get <filename>\n");
            return;
        }
        get_file(client, arg);
    }
    /*to change mode*/
    else if(strcmp(cmd, "mode") == 0){
        if (strlen(arg) == 0)
        {
            printf("Usage: mode <octet/netascii>\n");
            return;
        }
        check_mode(arg);
    }
    /*to disconnect the connection*/
    else if(strcmp(cmd, "bye") == 0 || strcmp(cmd, "quit") == 0){
        disconnect(client);
        exit(0);
    }
    else{
        printf("Unknown command. Try 'help' \n");
    }
}

// Function to initialize socket with given server IP, no packets sent to server in this function
void connect_to_server(tftp_client_t *client, char *ip, int port) 
{
    memset(&client->server_addr, 0, sizeof(client->server_addr));

    /*validate IP address*/
    if(inet_pton(AF_INET, ip, &client->server_addr.sin_addr) <= 0){
        printf("Invalid IP address\n");
        return;
    }
    /*Create UDP socket*/
    client->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(client->sockfd == -1){
        perror("socket");
        return;
    }
    /*Set socket timeout option*/
    struct timeval tv;

    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = 0;

    if(setsockopt(client->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1){
        perror("setsockopt");
        close(client->sockfd);
        client->sockfd = -1;
        return;
    }

    /*Set up server address*/
    client->server_addr.sin_family = AF_INET;
    client->server_addr.sin_port = htons(port);

    client->server_len = sizeof(client->server_addr);

    strcpy(client->server_ip, ip);

    printf("Client Configured for server %s : %d\n",ip,port);
}

void put_file(tftp_client_t *client, char *filename) 
{
    char path[256];
    snprintf(path, sizeof(path), "client/%s", filename);

    /*check whether file exists*/
    int fd = open(path, O_RDONLY);
    if(fd == -1){
        perror("open");
        printf("File not found in client folder: %s\n", path);

        printf("Creating empty file and uploading...\n");

        /* Create empty file */
        int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if(fd == -1){
            perror("open");
            return;
        }
    }
    close(fd);

    /*Send WRQ request and waits for ACK*/
    if(send_request(client->sockfd, &client->server_addr, filename, WRQ) == 0)
        return;

    /*then send data packets*/
    client_send_file(client->sockfd, client->server_addr, client->server_len, path);
}

void get_file(tftp_client_t *client, char *filename) 
{
    char path[256];
    snprintf(path, sizeof(path), "client/%s", filename);

    /*Send RRQ and recive file*/
    if(send_request(client->sockfd, &client->server_addr, filename, RRQ) == 0)
        return;

    /*receive data packets*/
    client_receive_file(client->sockfd, client->server_addr, client->server_len, filename);
}

int send_request(int sockfd, struct sockaddr_in *server_addr, char *filename, int opcode)
{
    char buffer[516];
    int offset = 0;
    uint16_t net_opcode;

    /*add opcode*/
    net_opcode = htons(opcode);

    memcpy(buffer + offset, &net_opcode, sizeof(net_opcode));

    offset += sizeof(net_opcode);

    /*add filename*/
    strcpy(buffer + offset, filename);

    offset += strlen(filename) + 1;

    /*add mode*/
    strcpy(buffer + offset, transfer_mode);

    offset += strlen(transfer_mode) + 1;

    /*send actual request length*/
    if (sendto(sockfd, buffer, offset, 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) == -1)
    {
        perror("sendto");
        return 0;
    }
    printf("Request sent\n");

    /*if RRQ, dont wait for ACK< it starts sending*/
    if (opcode == RRQ)
    {
        return 1;
    }

    /*if WRQ requres ACK*/
    if (opcode == WRQ)
    {
        tftp_packet packet;
        struct sockaddr_in response_addr;
        socklen_t response_len = sizeof(response_addr);

        int n = recvfrom(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&response_addr, &response_len);

        if (n < 0){
            perror("recvfrom");
            return 0;
        }

        /*Convert opcode from network byte order*/
        uint16_t received_opcode = ntohs(packet.opcode);

        /*server send ack*/
        if (received_opcode == ACK)
        {
            uint16_t block = ntohs(packet.body.ack_packet.block_number);
            if (block == 0){
                printf("ACK 0 received - server ready\n");

                *server_addr = response_addr;
                return 1;
            }

            printf("Invalid ACK block number: %u\n", block);
            return 0;
        }

        /*server send ERROR*/
        if (received_opcode == ERROR)
        {
            uint16_t error_code = ntohs(packet.body.error_packet.error_code);

            printf("Server returned ERROR %u: %s\n", error_code, packet.body.error_packet.error_msg);
            return 0;
        }
        printf("Invalid response from server\n");
        return 0;
    }

    return 0;
}

void check_mode(char *mode)
{
    if (strcmp(mode, "octet") == 0){
        strcpy(transfer_mode, "octet");
        current_transfer_mode = MODE_OCTET;
        other_os_flag = 0;
        printf("Transfer mode set to octet\n");
    }

    else if (strcmp(mode, "netascii") == 0){
        strcpy(transfer_mode, "netascii");
        current_transfer_mode = MODE_NETASCII;
        other_os_flag = 1;
        printf("Transfer mode set to netascii\n");
    }

    else{
        printf("Invalid mode\n");
        printf("Use: mode octet\n");
        printf("or:  mode netascii\n");
    }
}

void disconnect(tftp_client_t *client)
{
    if (client->sockfd != -1){
        close(client->sockfd);
        client->sockfd = -1;
    }

    printf("Disconnected from server.\n");
}
    