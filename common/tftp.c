#include "tftp.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int buff_size = 512;
int other_os_flag = 0;
int current_transfer_mode = MODE_DEFAULT;

void set_transfer_mode(int mode)
{
    if (mode != MODE_DEFAULT && mode != MODE_OCTET && mode != MODE_NETASCII)
        return;

    current_transfer_mode = mode;
    if (mode == MODE_NETASCII)
    {
        buff_size = 512;
        other_os_flag = 1;
    }
    else if (mode == MODE_OCTET)
    {
        buff_size = 512;
        other_os_flag = 0;
    }
    else
    {
        buff_size = 512;
        other_os_flag = 0;
    }
}

void mode_select(int mode)
{
    if (mode == MODE_DEFAULT)
    {
        set_transfer_mode(mode);
        printf("mode set to default, packet size adjusted to 512 bytes\n");
    }
    else if (mode == MODE_OCTET)
    {
        set_transfer_mode(mode);
        printf("mode set to octet, packet size adjusted to 512 bytes\n");
    }
    else if (mode == MODE_NETASCII)
    {
        set_transfer_mode(mode);
        printf("mode set to Net ascii, packet size adjusted to 512 bytes\n");
    }
    else
    {
        printf("invalid mode\n");
    }
    fflush(stdout);
}

ssize_t read_transfer_chunk(int fd, char *buff, size_t size, int *pending_char)
{
    size_t written = 0;
    while (written < size)
    {
        if (*pending_char >= 0)
        {
            buff[written++] = (char)*pending_char;
            *pending_char = -1;
            continue;
        }

        char ch;
        ssize_t bytes_read = read(fd, &ch, 1);
        if (bytes_read <= 0)
            return written > 0 ? (ssize_t)written : bytes_read;

        if (ch == '\n')
        {
            buff[written++] = '\r';
            if (written < size)
                buff[written++] = '\n';
            else
                *pending_char = '\n';
        }
        else if (ch == '\r')
        {
            buff[written++] = '\r';
            if (written < size)
                buff[written++] = '\0';
            else
                *pending_char = '\0';
        }
        else
        {
            buff[written++] = ch;
        }
    }

    return (ssize_t)written;
}

int write_transfer_chunk(int fd, char *buff, ssize_t buff_len, int *pending_cr)
{
    if (!other_os_flag)
        return write(fd, buff, buff_len) == buff_len;

    for (int i = 0; i < buff_len; i++)
    {
        char ch = buff[i];

        if (*pending_cr)
        {
            if (ch == '\n'){
                ch = '\n';
            }
            else if (ch == '\0'){
                ch = '\r';
            }
            else{
                char cr = '\r';
                if (write(fd, &cr, 1) != 1)
                    return 0;

                // now write X
                if (write(fd, &ch, 1) != 1)
                    return 0;

                *pending_cr = 0;
                continue;   // prevent double write
            }
            *pending_cr = 0;
        }

        if (ch == '\r')
        {
            *pending_cr = 1;
            continue;
        }

        if (write(fd, &ch, 1) != 1)
            return 0;
    }

    return 1;
}

int finish_transfer_write(int fd, int *pending_cr)
{
    if (other_os_flag && *pending_cr)
    {
        char cr = '\r';
        *pending_cr = 0;
        return write(fd, &cr, 1) == 1;
    }

    return 1;
}

int read_netascii(int fd, char *buffer, int block_size)
{
    int i = 0;
    char ch;

    while(i < block_size){      
        int n = read(fd, &ch, 1);   /*read char by char*/
        
        if(n == 0)     /*EOF*/
            break;
        
        if(n == -1)    /*error*/
            return -1;

        if(ch == '\n'){
            /*check space for two char*/
            if(i + 1 >= block_size){
                buffer[i++] = '\r';
                buffer[i++] = '\n';
            }
            else
                buffer[i++] = ch;
        }
    }
    return i;
}

int write_netascii(int fd, char *buffer, int bytes)
{
    for(int i = 0; i < bytes; i++){
        if(buffer[i] == '\r' && (i+1) < bytes
            && buffer[i+1] == '\n'){
            char ch = '\n';

            if(write(fd, &ch, 1) == -1)
                return -1;

            i++;   /*skip '\n'*/
        }
        else{
            if(write(fd, &buffer[i], 1) == -1)
                return -1;
        }
    }
    return 0;
}