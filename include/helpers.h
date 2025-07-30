#pragma once

#include <stdlib.h>
#include <stdint.h>

#define PORT "9035"
#define MAX_QUEUE 10
#define MAX_TEXT_LENGTH 256

typedef struct chat_data
{
    uint16_t length;
    char *data;
} chat_data;

const char *get_address(void *addr, char *buffer, size_t len);
int sendall(int fd, void *data, size_t data_len, int flags);
int recvall(int fd, void *data, size_t data_len, int flags);