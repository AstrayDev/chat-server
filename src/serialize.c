#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "serialize.h"


int serialize_struct(struct chat_data *_struct, char *buffer)
{
    if (!_struct || !buffer)
    {
        return -1;
    }

    size_t offset = 0;
    uint16_t length = htons(_struct->length);

    memcpy(buffer, &length, sizeof(length));
    offset += sizeof(length);
    memcpy(buffer + offset, _struct->name, MAX_NAME_LENGTH);
    offset += MAX_NAME_LENGTH;
    memcpy(buffer + offset, _struct->data, _struct->length);

    return 0;
}

int deserialize_struct(struct chat_data *_struct, char *buffer)
{
    if (!_struct || !buffer)
    {
        return -1;
    }

    uint16_t length;
    size_t offset = 0;

    memcpy(&length, buffer, sizeof(length));

    offset += sizeof(length);

    _struct->length = length;
    _struct->data = malloc(length);
    memcpy(_struct->name, buffer + offset, MAX_NAME_LENGTH);
    offset += MAX_NAME_LENGTH;
    memcpy(_struct->data, buffer + offset, length);

    return 0;
}