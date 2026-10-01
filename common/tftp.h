#ifndef TFTP_H
#define TFTP_H

#include <stdint.h>
#include <arpa/inet.h>
#define TIMEOUT_SEC 3

/*TFTP PORT & CONSTANTS*/
#define PORT 6969
#define TFTP_DATA_SIZE 512
#define BUFFER_SIZE (TFTP_DATA_SIZE + 4)

/*TRANSFER MODES*/
#define MODE_DEFAULT   0
#define MODE_OCTET     1
#define MODE_NETASCII  2

extern int buff_size;
extern int other_os_flag;
extern int current_transfer_mode;

/*TFTP OPCODES*/
typedef enum {
    RRQ   = 1,
    WRQ   = 2,
    DATA  = 3,
    ACK   = 4,
    ERROR = 5
} tftp_opcode;

/*TFTP PACKET STRUCTURE*/
typedef struct {
    uint16_t opcode;
    union {
        struct {
            char filename[256];
            char mode[16];
        } request;

        struct {
            uint16_t block_number;
            char data[TFTP_DATA_SIZE];
        } data_packet;

        struct {
            uint16_t block_number;
        } ack_packet;

        struct {
            uint16_t error_code;
            char error_msg[256];
        } error_packet;

    } body;
} tftp_packet;

/*MODE & TRANSFER FUNCTIONS*/
void set_transfer_mode(int mode);
void mode_select(int mode);

/*NETASCII / OCTET READ/WRITE*/
ssize_t read_transfer_chunk(int fd, char *buff, size_t size, int *pending_char);
int write_transfer_chunk(int fd, char *buff, ssize_t buff_len, int *pending_cr);
int finish_transfer_write(int fd, int *pending_cr);

/* CLIENT/SERVER FILE FUNCTIONS (implemented separately)*/
void client_send_file(int sockfd, struct sockaddr_in server_addr, socklen_t server_len, char *filename);
void client_receive_file(int sockfd, struct sockaddr_in server_addr, socklen_t server_len, char *filename);

void server_send_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename);
void server_receive_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename);

/* REQUEST FUNCTION (RRQ/WRQ)*/
int send_request(int sockfd, struct sockaddr_in *addr, char *filename, int opcode);

#endif
