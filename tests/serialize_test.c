#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "serialize.h"

int main()
{
    struct chat_data cd = {5, strdup("hello")};

    printf("Before Serialization: Length = %d, Data = %s\n", cd.length, cd.data);

    char *buffer = malloc(sizeof(uint16_t) + cd.length);

    struct chat_data *cd2 = malloc(sizeof(struct chat_data));

    serialize_struct(&cd, buffer);
    deserialize_struct(cd2, buffer);

    assert(cd2->length == 5 && strcmp(cd2->data, "hello") == 0);

    printf("After Serialization: Length = %d, Data = %s\n", cd2->length, cd2->data);

    free(buffer);
    free(cd.data);
    free(cd2->data);
    free(cd2);

    printf("Test Succeeded!\n");
}