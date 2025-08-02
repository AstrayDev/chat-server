#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "serialize.h"

int get_input(char *input)
{
    int c, length = 0, capacity = 10;
    char *tmp = NULL;

    if (!input)
    {
        printf("Failed input allocation");
        return -1;
    }

    while ((c = fgetc(stdin)) != '\n' && c != EOF)
    {
        // Not sure why it didn't work before but the minus seems to ensure
        // it doesn't access outside of allocated memory
        if (length >= capacity - 1)
        {
            capacity *= 2;
            tmp = realloc(input, capacity);

            if (!input)
            {
                free(tmp);
                return NULL;
            }

            input = tmp;
        }

        input[length++] = c;
    }

    input[length] = '\0';

    if (strlen(input) > MAX_TEXT_LENGTH)
    {
        return -1;
    }

    return 0;
}

int main()
{
    struct addrinfo hints, *addressinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int rv;
    int sockfd = 0;
    char address[INET6_ADDRSTRLEN];

    if ((rv = getaddrinfo("localhost", PORT, &hints, &addressinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = addressinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            char buf[INET6_ADDRSTRLEN];
            inet_ntop(p->ai_family, p->ai_addr, buf, sizeof(buf));
            printf("%s\n", buf);
            perror("client: connect");
            close(sockfd);

            return 1;
        }

        get_address(p->ai_addr, address, p->ai_addrlen);

        if (p == NULL)
        {
            fprintf(stderr, "Couldn't connect to server:\n");
            return 2;
        }
    }

    printf("Connected to server at address: %s\n", address);

    freeaddrinfo(addressinfo);

    char *input = malloc(10);

    for (;;)
    {
        if (get_input(input) == -1)
        {
            printf("ERRRRR");
        }

        struct chat_data chat_info = {.length = strlen(input) + 1, .data = input};

        printf("char length: %d\n", chat_info.length);

        size_t struct_size = sizeof(uint16_t) + chat_info.length;

        char *buffer_send = malloc(struct_size);

        serialize_struct(&chat_info, buffer_send);

        sendall(sockfd, buffer_send, struct_size, 0);

        int nbytes = 0;
        uint16_t length = 0;
        char *buffer_recv = malloc(struct_size);

        nbytes = recvall(sockfd, &length, sizeof(uint16_t), 0);
        memcpy(buffer_recv, &length, sizeof(length));
        nbytes = recvall(sockfd, buffer_recv + sizeof(uint16_t), length, 0);

        if (nbytes <= 0)
        {
            if (nbytes == 0)
            {
                printf("Connection with server closed\n");
                break;
            }

            else
            {
                perror("recv");
            }
        }

        else
        {
            memset(&chat_info, 0, sizeof(struct chat_data));
            deserialize_struct(&chat_info, buffer_recv);

            printf("Client on socket %d said: %s\n", sockfd, chat_info.data);
        }

        free(buffer_send);
        free(buffer_recv);
    }
}