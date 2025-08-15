#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <string.h>

void* print_message(void *message)
{
    strcpy((char *)message, "Hoi");
    printf("Thread changed message\n");
    return NULL;
}

int main()
{
    pthread_t thread;
    char *message = malloc(4);

    if (pthread_create(&thread, NULL, print_message, (void *)message) != 0)
    {
        printf("Thread error\n");
        return 1;
    }

    if (pthread_join(thread, NULL) != 0)
    {
        printf("Thread failed to join\n");
        return 2;
    }

    printf("Thread finished. Message is %s\n", message);
}