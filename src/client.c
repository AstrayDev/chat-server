#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "serialize.h"

struct client_data
{
    int sockfd;
    pthread_mutex_t mutex;
    volatile sig_atomic_t running;
    struct chat_data chat_info;
    char client_name[MAX_NAME_LENGTH];
};

int get_input(char **input)
{
    int c, length = 0, capacity = 10;
    char *tmp = NULL;

    while ((c = fgetc(stdin)) != '\n' && c != EOF)
    {
        // Not sure why it didn't work before but the minus seems to ensure
        // it doesn't access outside of allocated memory
        if (length >= capacity - 1)
        {
            capacity *= 2;
            tmp = realloc(*input, capacity);

            if (!tmp)
            {
                return -1;
            }

            *input = tmp;
        }

        (*input)[length++] = c;
    }

    (*input)[length] = '\0';

    if (strlen(*input) > MAX_TEXT_LENGTH)
    {
        return -1;
    }

    return 0;
}

void *send_msg(void *cd)
{
    struct client_data *c_data = (struct client_data *)cd;
    char *input = malloc(10);

    while (c_data->running)
    {
        if (get_input(&input) < 0)
        {
            printf("Couldn't get text input");
        }

        if (strcmp(input, "exit") == 0)
        {
            c_data->running = 0;
            shutdown(c_data->sockfd, SHUT_RDWR);
            free(input);
            pthread_exit(NULL);
        }

        pthread_mutex_lock(&c_data->mutex);

        c_data->chat_info.length = strlen(input) + 1;
        strcpy(c_data->chat_info.name, c_data->client_name);
        c_data->chat_info.data = input;

        pthread_mutex_unlock(&c_data->mutex);

        size_t struct_size = sizeof(uint16_t) + MAX_NAME_LENGTH + c_data->chat_info.length;

        char *buffer_send = malloc(struct_size);
        if (!buffer_send)
        {
            perror("buffer_send malloc");
            free(input);
            return NULL;
        }

        serialize_struct(&c_data->chat_info, buffer_send);
        sendall(c_data->sockfd, buffer_send, struct_size, 0);

        free(buffer_send);
    }

    free(input);
    return NULL;
}

void *recv_msg(void *cd)
{
    struct client_data *c_data = (struct client_data *)cd;

    int nbytes = 0;
    uint16_t length = 0;

    while (c_data->running)
    {
        size_t offset = 0;
        
        nbytes = recvall(c_data->sockfd, &length, sizeof(uint16_t), 0);
        length = ntohs(length);

        size_t struct_size = sizeof(uint16_t) + MAX_NAME_LENGTH + length;
        char *buffer_recv = malloc(struct_size);
        
        memcpy(buffer_recv, &length, sizeof(length));
        offset += sizeof(length);
        nbytes = recvall(c_data->sockfd, buffer_recv + offset, MAX_NAME_LENGTH, 0);
        offset += MAX_NAME_LENGTH;
        nbytes = recvall(c_data->sockfd, buffer_recv + offset, length, 0);

        if (nbytes <= 0)
        {
            if (nbytes == 0)
            {
                printf("Connection with server closed\n");
                return NULL;
            }

            else
            {
                perror("recv");
                return NULL;
            }
        }

        else
        {
            pthread_mutex_lock(&c_data->mutex);

            memset(&c_data->chat_info, 0, sizeof(struct chat_data));
            deserialize_struct(&c_data->chat_info, buffer_recv);

            pthread_mutex_unlock(&c_data->mutex);

            printf("%s said: %s\n", c_data->chat_info.name, c_data->chat_info.data);
        }
        free(buffer_recv);
        free(c_data->chat_info.data);
    }

    return NULL;
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

    pthread_t input_thread;
    pthread_t recv_thread;

    struct client_data c_data = {.sockfd = sockfd, .running = 1};
    pthread_mutex_init(&c_data.mutex, NULL);

    puts("Welcome to the chat room! Enter 'exit' to leave\nWhats your name?");
    char *name = malloc(MAX_NAME_LENGTH);

    get_input(&name);
    strcpy(c_data.client_name, name);
    free(name);

    if (pthread_create(&input_thread, NULL, send_msg, (void *)&c_data) != 0)
    {
        perror("Thread Create");
        return 1;
    }

    if (pthread_create(&recv_thread, NULL, recv_msg, (void *)&c_data) != 0)
    {
        perror("Thread Create");
        return 1;
    }

    if (pthread_join(input_thread, NULL) != 0)
    {
        perror("Thread Join");
    }

    if (pthread_join(recv_thread, NULL) != 0)
    {
        perror("Thread Join");
    }

    pthread_mutex_destroy(&c_data.mutex);
    close(sockfd);
}