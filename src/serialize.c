#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "helpers.h"


int serialize_struct(struct chat_data *_struct, char *buffer)
{
    if (!_struct || !buffer)
    {
        return -1;
    }

    uint16_t length = htons(_struct->length);
    char *data = _struct->data;
    memcpy(buffer, &length, sizeof(length));
    buffer += sizeof(length);

    memcpy(buffer, data, length);

    return 0;
}

int deserialize_struct(struct chat_data *_struct, char *buffer)
{
    if (!_struct || !buffer)
    {
        return -1;
    }

    uint16_t p;

    memcpy(&p, buffer, sizeof(p));
    buffer += sizeof(p);

    p = ntohs(p);

    _struct->length = p;
    _struct->data = malloc(p);
    memcpy(_struct->data, buffer, _struct->length);

    return 0;
}