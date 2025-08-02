CC       := gcc
CFLAGS   := -std=c11 -g -Wall -Wextra -O2 -D_GNU_SOURCE -Iinclude
SRC      := src/
TESTS    := tests/
INC      := include/
DEPS     := $(INC)helpers.h $(INC)serialize.h

SRCS     := $(wildcard $(SRC)*.c)
TESTSRC := $(wildcard $(TESTS)*_test.c)
TESTBINS := $(notdir $(TESTSRC:.c=))
CLIENT_OBJS := obj/client.o obj/helpers.o obj/serialize.o
SERVER_OBJS := obj/server.o obj/helpers.o obj/serialize.o

vpath %.c $(SRC) $(TESTS)

.PHONY: all clean test

all: obj client server $(TESTBINS)

client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $(CLIENT_OBJS)

server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $(SERVER_OBJS)

obj/%.o: %.c $(DEPS) | obj
	$(CC) $(CFLAGS) -c $< -o $@

%: obj/%.o obj/serialize.o
	$(CC) $(CFLAGS) -o $@ $^

%_test: tests/%_test.c obj/helpers.o obj/serialize.o
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f client server *test test obj/*.o