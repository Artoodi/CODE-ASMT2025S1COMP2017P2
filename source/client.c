#include "../libs/markdown.h"

#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <semaphore.h>

#define FIFO_C2S_FMT "FIFO_C2S_%d"
#define FIFO_S2C_FMT "FIFO_S2C_%d"

typedef struct
{
    char* user;
    char* role;
} role;

document* doc = NULL;
int write_fd;
int connected = 0;
sem_t ready;

void init_doc(const char* command)
{
    char* temp = NULL;
    asprintf(&temp, "%s", command);

    char* part[20];
    int count = 0;

    char* current = temp;

    while (1)
    {
        char* line = strchr(current, '\n');
        if (!line)
            break;

        *line = '\0';

        part[count++] = current;

        current = line + 1;
    }

    (void)part;

    free(temp);
}

void handle_server_broadcast(const char* command)
{
    char* temp = malloc(strlen(command) + 1);
    memcpy(temp, command, strlen(command));
    temp[strlen(command)] = '\0';

    char* parts[10];
    int count = 0;

    char* current = temp;

    while (1)
    {
        char* line = strchr(current, '\n');
        if (!line)
            break;

        *line = '\0';

        parts[count++] = current;

        current = line + 1;
    }

    (void)parts;

    free(temp);
}

void add_command_history(char* cmd)
{
    cmd[strlen(cmd) - 1] = '\0';

    command** commands_history_ptr = &commands_history;
    while (*commands_history_ptr)
        commands_history_ptr = &(*commands_history_ptr)->next;

    *commands_history_ptr = malloc(sizeof(command));
    (*commands_history_ptr)->command = strdup(cmd);
    (*commands_history_ptr)->user = NULL;
    (*commands_history_ptr)->next = NULL;
}

int handle_server_command(char* command)
{
    if (strcmp(command, "Reject UNAUTHORISED") == 0)
    {
        printf("Reject UNAUTHORISED\n");

        connected = 0;

        return 0;
    }

    switch (command[0])
    {
    case 'r':
    case 'w':
        init_doc(command);

        return 1;

    case 'V':
        handle_server_broadcast(command);
        add_command_history(command);

        return 1;

    // ----DEBUG----
    // case 'E':
    //     add_command_history(command);
    //
    //     return 1;

    default:
        printf("%s\n", command);
    }

    return 1;
}

void* handle_server(void* arg)
{
    (void)arg;

    const pid_t pid = getpid();

    char fifo_c2s[32], fifo_s2c[32];

    snprintf(fifo_c2s, sizeof(fifo_c2s), FIFO_C2S_FMT, pid);
    mkfifo(fifo_c2s, 0666);

    write_fd = open(fifo_c2s, O_WRONLY);
    if (write_fd == -1)
    {
        printf("Failed to open write FIFO\n");

        return NULL;
    }

    snprintf(fifo_s2c, sizeof(fifo_s2c), FIFO_S2C_FMT, pid);
    mkfifo(fifo_s2c, 0666);

    const int read_fd = open(fifo_s2c, O_RDONLY);
    if (read_fd == -1)
    {
        unlink(fifo_c2s);

        printf("Failed to open read FIFO\n");

        close(write_fd);

        return NULL;
    }

    connected = 1;

    sem_post(&ready);

    char* buffer = malloc(256);

    while (1)
    {
        const ssize_t length = read(read_fd, buffer, 256);
        if (length <= 0)
            break;

        buffer[length] = '\0';

        if (!handle_server_command(buffer))
            break;
    }

    free(buffer);

    close(read_fd);
    close(write_fd);

    unlink(fifo_c2s);
    unlink(fifo_s2c);

    sem_post(&ready);

    return NULL;
}

void sig_handler(const int sig)
{
    if (sig == SIGRTMIN + 1)
    {
        pthread_t thread;
        pthread_create(&thread, NULL, handle_server, NULL);
        pthread_detach(thread);
    }
}

int main(const int argc, char* argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <server_pid> <username>\n", argv[0]);

        return -1;
    }

    const int server_pid = atoi(argv[1]);

    char* username = NULL;
    asprintf(&username, "%s\n", argv[2]);

    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGRTMIN + 1, &sa, NULL) == -1)
        return -1;

    sem_init(&ready, 0, 0);

    if (kill(server_pid, SIGRTMIN) == -1)
    {
        perror("");

        return -1;
    }

    sem_wait(&ready);

    usleep(500);

    doc = markdown_init();

    write(write_fd, username, strlen(username));

    username[strlen(username) - 1] = '\0';

    char* input = NULL;
    size_t size = 0;

    // Client Commands
    while (connected)
    {
        const ssize_t length = getline(&input, &size, stdin);

        if (strcmp(input, "DISCONNECT\n") == 0)
        {
            write(write_fd, input, length);

            break;
        }

        if (strcmp(input, "DOC?\n") == 0)
        {
            char* content = markdown_flatten(doc);
            if (content)
            {
                printf("%s", content);

                free(content);
            }
        }
        else if (strcmp(input, "LOG?\n") == 0)
        {
            const command* commands_history_ptr = commands_history;
            while (commands_history_ptr)
            {
                printf("%s\n", commands_history_ptr->command);

                commands_history_ptr = commands_history_ptr->next;
            }
        }
        else
        {
            if (length == 1)
                continue;

            write(write_fd, input, length);
        }

        memset(input, 0, length);
    }

    sem_wait(&ready);

    free(input);

    sem_destroy(&ready);

    free(doc);

    return 0;
}
