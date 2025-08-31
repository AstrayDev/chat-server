#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "serialize.h"

int main()
{
    struct chat_data cd = {5, "Joe", strdup("hello")};

    printf("Before Serialization: Length = %d, Name = %s, Data = %s\n", cd.length, cd.name, cd.data);

    char *buffer = malloc(sizeof(uint16_t) + MAX_NAME_LENGTH + cd.length);

    struct chat_data *cd2 = malloc(sizeof(struct chat_data));

    serialize_struct(&cd, buffer);
    deserialize_struct(cd2, buffer);

    assert(cd2->length == 5);
    assert(strcmp(cd2->data, "hello") == 0);
    assert(strcmp(cd2->name, "Joe") == 0);

    printf("After Serialization: Length = %d, Name = %s, Data = %s\n", cd2->length, cd2->name, cd2->data);

    free(buffer);
    free(cd.data);
    free(cd2->data);
    free(cd2);

    printf("Test Succeeded!\n");
}