#include "tftp.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <arpa/inet.h>

void client_send_file(int sockfd, struct sockaddr_in server_addr, socklen_t server_len, char *filename)
{
    char path[256];
    snprintf(path, sizeof(path), "%s", filename);

    printf("Sending file from: %s\n", path);

    int fd = open(path, O_RDONLY);
    if(fd == -1){
        perror("open");
        printf("File not found in client folder: %s\n", path);
        return;
    }

    tftp_packet packet;
    int block = 1;
    int bytes;
    int byte_size = 512;
    int pending_char = -1;

    while(1){
        /* READ DATA */
        bytes = (current_transfer_mode == MODE_NETASCII)
                ? read_transfer_chunk(fd, packet.body.data_packet.data, byte_size, &pending_char)
                : read(fd, packet.body.data_packet.data, byte_size);

        if(bytes < 0){
            perror("read");
            break;
        }

        /* BUILD DATA PACKET */
        packet.opcode = htons(DATA);
        packet.body.data_packet.block_number = htons(block);

        sendto(sockfd, &packet, 4 + bytes, 0,
               (struct sockaddr *)&server_addr, server_len);

        printf("Sent %d bytes in DATA block %d\n", bytes, block);

        /* WAIT FOR ACK */
        recvfrom(sockfd, &packet, sizeof(packet), 0,
                 (struct sockaddr *)&server_addr, &server_len);

        printf("ACK %d received\n", block);

        if(bytes < byte_size)
            break;

        block++;
    }

    close(fd);
    printf("File sent successfully\n");
}

void client_receive_file(int sockfd, struct sockaddr_in server_addr, socklen_t server_len, char *filename)
{
    char path[256];
    snprintf(path, sizeof(path), "client/%s", filename);

    printf("Saving file to: %s\n", path);

    int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if(fd == -1){
        perror("open");
        return;
    }

    tftp_packet packet;
    int expected_block = 1;
    int byte_size = 512;
    int pending_cr = 0;

    while(1){
        int n = recvfrom(sockfd, &packet, sizeof(packet), 0,
                         (struct sockaddr *)&server_addr, &server_len);

        if(n < 0){
            perror("recvfrom");
            break;
        }

        /* Handle ERROR packet from server */
        if(ntohs(packet.opcode) == ERROR){
            printf("Server error: %s\n", packet.body.error_packet.error_msg);
            close(fd);
            remove(path);  // delete empty file
            return;
        }

        if(ntohs(packet.opcode) != DATA){
            printf("Expected DATA packet\n");
            break;
        }

        int data_len = n - 4;

        /* WRITE DATA */
        if(current_transfer_mode == MODE_NETASCII)
            write_transfer_chunk(fd, packet.body.data_packet.data, data_len, &pending_cr);
        else
            write(fd, packet.body.data_packet.data, data_len);

        /* SEND ACK */
        packet.opcode = htons(ACK);
        packet.body.ack_packet.block_number = htons(expected_block);

        sendto(sockfd, &packet, 4, 0,
               (struct sockaddr *)&server_addr, server_len);

        printf("Received block %d (%d bytes)\n", expected_block, data_len);

        if(data_len < byte_size)
            break;

        expected_block++;
    }

    if(current_transfer_mode == MODE_NETASCII)
        finish_transfer_write(fd, &pending_cr);

    close(fd);
    printf("File received successfully\n");
}
