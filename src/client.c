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

struct thread_data
{
    int sockfd;
    pthread_mutex_t mutex;
    volatile sig_atomic_t running;
    struct chat_data chat_info;
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

void *send_msg(void *td)
{
    struct thread_data *t = (struct thread_data *)td;
    char *input = malloc(10);

    while (t->running)
    {
        if (get_input(&input) < 0)
        {
            printf("Couldn't get text input");
        }

        if (strcmp(input, "exit") == 0)
        {
            t->running = 0;
            free(input);
            pthread_exit(NULL);
        }

        pthread_mutex_lock(&t->mutex);

        t->chat_info.length = strlen(input) + 1;
        t->chat_info.data = input;

        pthread_mutex_unlock(&t->mutex);

        size_t struct_size = sizeof(uint16_t) + t->chat_info.length;

        char *buffer_send = malloc(struct_size);
        if (!buffer_send)
        {
            perror("buffer_send malloc");
            free(input);
            return NULL;
        }

        serialize_struct(&t->chat_info, buffer_send);
        sendall(t->sockfd, buffer_send, struct_size, 0);

        free(buffer_send);
    }

    return NULL;
}

void *recv_msg(void *td)
{
    struct thread_data *t = (struct thread_data *)td;

    int nbytes = 0;
    uint16_t length = 0;

    size_t struct_size = sizeof(uint16_t) + t->chat_info.length;
    char *buffer_recv = malloc(struct_size);

    while (t->running)
    {
        nbytes = recvall(t->sockfd, &length, sizeof(uint16_t), 0);
        memcpy(buffer_recv, &length, sizeof(length));
        nbytes = recvall(t->sockfd, buffer_recv + sizeof(uint16_t), length, 0);

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
            pthread_mutex_lock(&t->mutex);

            memset(&t->chat_info, 0, sizeof(struct chat_data));
            deserialize_struct(&t->chat_info, buffer_recv);

            pthread_mutex_unlock(&t->mutex);

            if (strcmp(t->chat_info.data, "exit") == 0)
            {
                exit(0);
            }

            printf("Client said: %s\n", t->chat_info.data);
        }
    }

    free(buffer_recv);
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

    struct thread_data t_data = {.sockfd = sockfd, .running = 1};
    pthread_mutex_init(&t_data.mutex, NULL);

    if (pthread_create(&input_thread, NULL, send_msg, (void *)&t_data) != 0)
    {
        perror("Thread Create");
        return 1;
    }

    if (pthread_create(&recv_thread, NULL, recv_msg, (void *)&t_data) != 0)
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

    pthread_mutex_destroy(&t_data.mutex);
    close(sockfd);
}