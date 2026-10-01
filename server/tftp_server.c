#include "tftp.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

/*globally assigning mode*/
char transfer_mode[20] = "default";

void handle_client(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, tftp_packet *packet);

int main() 
{
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    tftp_packet packet;

    /*Create UDP socket*/
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1){
        perror("socket");
        return 1;
    }

    /*Set socket timeout option*/
    // struct timeval tv;

    // tv.tv_sec = TIMEOUT_SEC;
    // tv.tv_usec = 0;

    // if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1){
    //     perror("setsockopt");
    //     return 1;
    // }
    
    /*Set up server address*/
    server_addr.sin_family =AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY; /*to accept any addr*/

    /*Bind the socket*/
    if(bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
        perror("bind");
        close(sockfd);
        return 1;
    }
    printf("TFTP Server listening on port %d...\n", PORT);

    // Main loop to handle incoming requests
    while(1){
        int n = recvfrom(sockfd, &packet, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_len);
        if (n < 0){
            perror("Receive failed or timeout occurred");
            continue;
        }

        printf("Client is connected\n");

        handle_client(sockfd, client_addr, client_len, &packet);
    }
    close(sockfd);
    return 0;
}

void handle_client(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, tftp_packet *packet) 
{
    /* Convert opcode from network byte order */
    uint16_t opcode = ntohs(packet->opcode);

    /*get filename*/
    char *filename = packet->body.request.filename;
    char *mode = filename + strlen(filename) + 1;

    printf("Filename : %s\n", filename);
    printf("Mode     : %s\n", mode);

    char path[512];

    /*READ REQUEST - GET*/
    if(opcode == RRQ){
        /* Build correct server path */
        snprintf(path, sizeof(path), "server/receive_folder/%s", filename);

        /*open file*/
        int fd = open(path, O_RDONLY);
        if(fd == -1){
            tftp_packet error_packet;
            memset(&error_packet, 0, sizeof(error_packet));

            error_packet.opcode = htons(ERROR);

            error_packet.body.error_packet.error_code = htons(1);

            strcpy(error_packet.body.error_packet.error_msg, "File not found");
            /*send error to client*/
            sendto(sockfd, &error_packet, 4 + strlen(error_packet.body.error_packet.error_msg) + 1
                , 0, (struct sockaddr *)&client_addr, client_len);

            return;
        }
        close(fd);

        printf("Sending file to client...\n");
        /*send file to client*/
        server_send_file(sockfd, client_addr, client_len, filename);

        return;
    }

    /*WRITE REQUEST - PUT*/
    else if(opcode == WRQ){
    /* Build correct server path */
        snprintf(path, sizeof(path), "server/receive_folder/%s", filename);

        /*open file and check existence*/
        int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if(fd == -1){
            tftp_packet error_packet;

            memset(&error_packet, 0, sizeof(error_packet));
            /*if error- send error packet to client and call send_file()*/
            if(errno == EACCES){
                error_packet.opcode = htons(ERROR);
                error_packet.body.error_packet.error_code = htons(2);
                
                strcpy(error_packet.body.error_packet.error_msg, "Access violation");
            }  
           
            else{
                error_packet.opcode = htons(ERROR);
                error_packet.body.error_packet.error_code = htons(3);

                strcpy(error_packet.body.error_packet.error_msg, "Cannot create file");
            }

            sendto(sockfd, &error_packet, sizeof(error_packet), 0,
                (struct sockaddr *)&client_addr, client_len);

            return;
        }
        close(fd);

        tftp_packet ack_packet;
        memset(&ack_packet, 0, sizeof(ack_packet));

        /*file opened successfully - send ACK*/
        ack_packet.opcode = htons(ACK);
        ack_packet.body.ack_packet.block_number = htons(0);

        sendto(sockfd, &ack_packet, 4, 0, 
            (struct sockaddr *)&client_addr, client_len);

        printf("server is ready, receiving data\n");
        /*receive file from client*/
        server_receive_file(sockfd, client_addr, client_len, filename);

        return;
    }
}
