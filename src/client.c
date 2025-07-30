#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "helpers.h"

char *get_input()
{
    int c, length = 0, capacity = 10;
    char *input = malloc(capacity), *tmp = NULL;

    if (!input)
    {
        printf("Failed input allocation");
        return NULL;
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

    input[length++] = '\n';
    input[length] = '\0';

    return input;
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

    char *input = NULL;

    for (;;)
    {
        if ((input = get_input()) == NULL)
        {
            printf("ERRRRR");
        }
        
        struct chat_data chat_info = {.length = strlen(input), .data = input};

        printf("char length: %d\n", chat_info.length);
        sendall(sockfd, (&chat_info) + sizeof(uint16_t), chat_info.length, 0);

        int nbytes = 0;
        char buffer[MAX_TEXT_LENGTH];

        nbytes = recvall(sockfd, buffer, sizeof(buffer), 0);

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

        buffer[nbytes] = '\0';
        printf("Client on socket %d said: %s\n", sockfd, buffer);
    }
}