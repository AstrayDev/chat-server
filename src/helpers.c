#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "helpers.h"

// Taken From Beej's poll server example
// https://beej.us/guide/bgnet/source/examples/pollserver.c
const char *get_address(void *addr, char *buffer, size_t len)
{
    struct sockaddr_storage *stored_addr = addr;
    struct sockaddr_in *sa4;
    struct sockaddr_in6 *sa6;
    void *src;

    switch (stored_addr->ss_family)
    {
    case AF_INET:
        sa4 = addr;
        src = &(sa4->sin_addr);
        break;

    case AF_INET6:
        sa6 = addr;
        src = &(sa6->sin6_addr);
        break;

    default:
        return NULL;
    }

    return inet_ntop(stored_addr->ss_family, src, buffer, len);
}

int sendall(int fd, void *data, size_t data_len, int flags)
{
    int data_sent = 0;

    while (data_len > data_sent)
    {
        int sent = 0;
        sent += send(fd, data + data_sent, data_len, flags);
        data_sent += sent;

        if (data_sent <= 0)
        {
            perror("send");
            return -1;
        }

        data_len -= sent;
    }

    return data_sent;
}

int recvall(int fd, void *data, size_t data_len, int flags)
{
    int data_sent = 0;

    while (data_len > data_sent)
    {
        int sent = 0;
        sent += recv(fd, data + data_sent, data_len, flags);
        data_sent += sent;

        if (data_sent <= 0)
        {
            perror("recv");
            return -1;
        }

        data_len -= sent;
    }

    return data_sent;
}