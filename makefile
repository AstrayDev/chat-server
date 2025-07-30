CC       := gcc
CFLAGS   := -std=c11 -g -Wall -Wextra -O2 -D_GNU_SOURCE -Iinclude
SRC      := src/
INC      := include/
DEPS     := $(INC)helpers.h $(INC)serialize.h

SRCS     := $(wildcard $(SRC)*.c)
CLIENT_OBJS := obj/client.o obj/helpers.o obj/serialize.o
SERVER_OBJS := obj/server.o obj/helpers.o obj/serialize.o
TEST_OBJS   := obj/test.o obj/serialize.o

vpath %.c $(SRC)

.PHONY: all clean

all: client server

client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $(CLIENT_OBJS)

server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $(SERVER_OBJS)

test: $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $(TEST_OBJS)

obj/%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f client server obj/*.o
