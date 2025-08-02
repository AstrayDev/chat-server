#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <assert.h>
#include "helpers.h"

int main()
{
    int sv[2];
    
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1)
    {
        perror("socketpair");
        return -1;
    }

    char message[5] = "hello";
    int message_len = sizeof(message) + 1;

    int sent = 0, recv = 0;

    sent = sendall(sv[0], message, message_len, 0);
    printf("Sending %d bytes from message of size %zu\n", sent, message_len);

    char *buffer = malloc(message_len);

    recv = recvall(sv[1], buffer, message_len, 0);

    printf("Recovered %d bytes from message of size %zu. Message is %s\n", recv, message_len, buffer);

    assert(sent == recv);

    return 0;
}