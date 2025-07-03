#ifndef DOCUMENT_H
#define DOCUMENT_H

#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

/**
 * This file is the header file for all the document functions. You will be tested on the functions inside markdown.h
 * You are allowed to and encouraged multiple helper functions and data structures, and make your code as modular as possible.
 * Ensure you DO NOT change the name of document struct.
*/

typedef struct location
{
    size_t after_position;
    int deleted;
} location;

typedef struct chunk
{
    char* content;
    struct chunk* next;
    location* locations;
    uint64_t size;
    uint64_t original_size;
} chunk;

// typedef struct document
// {
//     uint64_t version;
//     chunk* chunks;
// } document;

typedef struct document
{
    uint64_t version;
    char* content;
    uint64_t max_size;
    uint64_t original_size;
    uint64_t size;
    location* locations;
} document;

typedef struct
{
    char* document;
} previous_document;

typedef struct client
{
    pid_t pid;
    int read_fd;
    int write_fd;
    char* user;
    char* role;
    struct client* next;
} client;

typedef struct command
{
    char* command;
    char* user;
    struct command* next;
    int write_fd;
    int from_client;
} command;

extern client* clients;
extern pthread_mutex_t clients_mutex;

extern command* commands;
extern command* commands_history;
extern pthread_mutex_t commands_mutex;

extern previous_document* previous_doc;
extern pthread_mutex_t pre_doc_mutex;

// Functions from here onwards.
void document_init(document* doc);
void document_free(const document* doc);

int document_insert(document* doc, size_t pos, const char* content);
int document_delete(document* doc, size_t pos, size_t len);

int check_after_newline(document* doc, size_t pos);

int document_newline(document* doc, size_t pos);
int document_heading(document* doc, int level, size_t pos);
int document_bold(document* doc, size_t start, size_t end);
int document_italic(document* doc, size_t start, size_t end);
int document_blockquote(document* doc, size_t pos);
int document_ordered_list(document* doc, size_t pos);
int document_unordered_list(document* doc, size_t pos);
int document_code(document* doc, size_t start, size_t end);
int document_horizontal_rule(document* doc, size_t pos);
int document_link(document* doc, size_t start, size_t end, const char* url);

#endif //DOCUMENT_H
