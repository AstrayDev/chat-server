#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "serialize.h"

#define MAX_QUEUE 10

int init_listener()
{
    struct addrinfo hints, *addressinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int rv;
    int listener = 0;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &addressinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = addressinfo; p != NULL; p = p->ai_next)
    {
        if ((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            continue;
        }

        int optval = 1;

        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optval, sizeof(optval)) < 0)
        {
            perror("setsockopt");
            return 2;
        }

        if (bind(listener, p->ai_addr, p->ai_addrlen) == 0)
        {
            break;
        }

        close(listener);
    }

    if (p == NULL)
    {
        freeaddrinfo(addressinfo);
        fprintf(stderr, "Couldn't bind\n");
        return -1;
    }

    listen(listener, MAX_QUEUE);
    freeaddrinfo(addressinfo);

    char ipbuf[INET6_ADDRSTRLEN];
    get_address(p->ai_addr, ipbuf, p->ai_addrlen);
    printf("Server bound to: %s\n", ipbuf);

    return listener;
}

void handle_new_connection(int listener, uint8_t *poll_size, uint8_t *fd_count, struct pollfd **fds)
{
    struct sockaddr_storage client;
    socklen_t client_len = sizeof(client);
    int newfd;

    if ((newfd = accept(listener, (struct sockaddr *)&client, &client_len)) != -1)
    {
        if (fd_count >= poll_size)
        {
            *poll_size *= 2;
            *fds = realloc(*fds, sizeof(**fds) * (*poll_size));

            if (*fds == NULL)
            {
                printf("pollfd realloc failed\n");
                return;
            }
        }

        (*fds)[*fd_count].fd = newfd;
        (*fds)[*fd_count].events = POLLIN;
        (*fds)[*fd_count].revents = 0;

        (*fd_count)++;

        char buffer[INET6_ADDRSTRLEN];
        get_address(&client, buffer, client_len);
        printf("New connection coming from address: %s\n", buffer);
    }
}

void handle_client_data(int listener, struct pollfd *fds, uint8_t *fd_index, uint8_t *fd_count)
{
    int nbytes = 0;
    struct chat_data *chat_info;
    char buffer[MAX_TEXT_LENGTH];

    nbytes = recvall(fds[*fd_index].fd, chat_info, sizeof(uint16_t), 0);

    chat_info->length = nbytes;

    nbytes = recvall(fds[*fd_index].fd, chat_info + sizeof(uint16_t), chat_info->length, 0);

    if (nbytes <= 0)
    {
        if (nbytes == 0)
        {
            printf("Connection Hung Up\n");
        }

        else
        {
            perror("recv");
        }

        close(fds[*fd_index].fd);

        fds[*fd_index] = fds[*fd_count - 1];

        (*fd_count)--;
        (*fd_index)--;
    }

    else
    {
        printf("Recovered bytes from client: %d\n", nbytes);
        memcpy(buffer, chat_info->data, chat_info->length);
        buffer[nbytes] = '\0';
        for (int i = 0; i < *fd_count; i++)
        {
            if (fds[i].fd != listener && fds[i].fd != fds[*fd_index].fd)
            {
                sendall(fds[i].fd, buffer, sizeof(buffer), 0);
            }
        }
        printf("Client said: %s\n", buffer);
    }
}

void handle_connections(int listener, uint8_t *poll_size, uint8_t *fd_count, struct pollfd **fds)
{
    for (uint8_t i = 0; i < *fd_count; i++)
    {
        if ((*fds)[i].revents & (POLLIN | POLLHUP))
        {
            if ((*fds)[i].fd == listener)
            {
                handle_new_connection(listener, poll_size, fd_count, fds);
            }

            else
            {
                handle_client_data(listener, *fds, &i, fd_count);
            }
        }
    }
}

int main()
{
    uint8_t fd_count = 0;
    uint8_t poll_size = 5;
    struct pollfd *fds = malloc(sizeof(*fds) * poll_size);

    int listener = init_listener();

    if (listener == -1)
    {
        printf("Couldn't initialize listener\n");
        return -1;
    }

    fds[0].fd = listener;
    fds[0].events = POLLIN;

    fd_count++;

    for (;;)
    {
        int poll_count = poll(fds, fd_count, -1);

        if (poll_count == -1)
        {
            perror("poll");
            exit(1);
        }

        handle_connections(listener, &poll_size, &fd_count, &fds);
    }
}