#pragma once

#include "helpers.h"

int serialize_struct(struct chat_data *_struct, char *buffer);
int deserialize_struct(struct chat_data *_struct, char *buffer);