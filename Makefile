CC = gcc
CFLAGS = -Wextra -Wall -std=c11 -Ilibs -pthread -g

SRC_DIR = source
LIB_DIR = libs

# Source files for server and client
SERVER_SRCS = $(SRC_DIR)/server.c \
              $(SRC_DIR)/document.c \
              $(SRC_DIR)/markdown.c
CLIENT_SRCS = $(SRC_DIR)/client.c \
              $(SRC_DIR)/document.c \
              $(SRC_DIR)/markdown.c

SERVER_OBJS = $(SERVER_SRCS:.c=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)

.PHONY: all clean

all: server client

server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

markdown.o: \
    source/markdown.c source/document.c \
    libs/markdown.h libs/document.h
	$(CC) $(CFLAGS) -c source/markdown.c -o markdown_main.o
	$(CC) $(CFLAGS) -c source/document.c -o document.o
	$(CC) -r markdown_main.o document.o -o $@
	rm -f markdown_main.o document.o

clean:
	rm -f $(SRC_DIR)/*.o server client markdown.o