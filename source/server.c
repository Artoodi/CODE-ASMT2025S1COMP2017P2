#include "../libs/markdown.h"

#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#define FIFO_C2S_FMT "FIFO_C2S_%d"
#define FIFO_S2C_FMT "FIFO_S2C_%d"

typedef struct role
{
    char* user;
    char* role;
    struct role* next;
} role;

int64_t broad_time;
role* roles = NULL;
document* doc;
int running;
int64_t last_broad_time = 0;

// Read roles from roles.txt, also solve the format
int read_roles()
{
    FILE* file = fopen("roles.txt", "r");
    if (file == NULL)
    {
        perror("");

        return 0;
    }

    char* line = NULL;
    size_t size = 0;

    ssize_t length;

    role** roles_ptr = &roles;

    while ((length = getline(&line, &size, file)) != -1)
    {
        size_t index = 0;

        while (line[index] == ' ')
            ++index;

        size_t username_begin = 200, username_end = 200, role_begin = 200, role_end = 200;

        for (size_t i = index; i < (size_t)length; ++i)
        {
            if (username_begin == 200)
            {
                if (line[i] != ' ')
                    username_begin = i;
            }
            else if (username_end == 200)
            {
                if (line[i] == ' ')
                    username_end = i;
            }
            else if (role_begin == 200)
            {
                if (!(line[i] == ' ' || line[i] == '\n'))
                    role_begin = i;
            }
            else
            {
                if (line[i] == ' ' || line[i] == '\n')
                {
                    role_end = i;

                    break;
                }
            }
        }

        if (username_begin == 200 || username_end == 200 || role_begin == 200 || role_end == 200)
            continue;

        char* username = malloc(username_end - username_begin + 1);
        memcpy(username, line + username_begin, username_end - username_begin);
        username[username_end - username_begin] = '\0';

        char* role_ptr = malloc(role_end - role_begin + 1);
        memcpy(role_ptr, line + role_begin, role_end - role_begin);
        role_ptr[role_end - role_begin] = '\0';

        *roles_ptr = malloc(sizeof(role));
        (*roles_ptr)->user = username;
        (*roles_ptr)->role = role_ptr;
        (*roles_ptr)->next = NULL;

        roles_ptr = &(*roles_ptr)->next;
    }

    free(line);

    fclose(file);

    return 1;
}


//Check the roles from roles.txt
int check_role(client* client_ptr, const char* user)
{
    const role* role_ptr = roles;

    while (role_ptr)
    {
        if (strcmp(role_ptr->user, user) == 0)
        {
            client_ptr->user = role_ptr->user;
            client_ptr->role = role_ptr->role;

            pthread_mutex_lock(&pre_doc_mutex);

            char* response = NULL;
            if (previous_doc->document)
                asprintf(&response, "%s\n%lu\n%lu\n%s\n", role_ptr->role, doc->version, strlen(previous_doc->document),
                         previous_doc->document);
            else
                asprintf(&response, "%s\n%lu\n0\n", role_ptr->role, doc->version);

            write(client_ptr->write_fd, response, strlen(response));

            printf("%s", response);

            free(response);

            pthread_mutex_unlock(&pre_doc_mutex);

            return 1;
        }

        role_ptr = (role*)role_ptr->next;
    }

    write(client_ptr->write_fd, "Reject UNAUTHORISED", 19);

    return 0;
}


// Handle the client commands which should use server
int handle_client_command(const client* client_ptr, char* client_command)
{
    if (strcmp(client_command, "DISCONNECT\n") == 0)
        return 0;

    if (strcmp(client_command, "PERM?\n") == 0)
    {
        write(client_ptr->write_fd, client_ptr->role, strlen(client_ptr->role));

        return 1;
    }

    if (strcmp(client_ptr->role, "read") == 0)
    {
        client_command[strlen(client_command) - 1] = '\0';

        char* output = NULL;
        asprintf(&output, "EDIT %s %s Reject UNAUTHORISED\n", client_ptr->user, client_command);

        printf("%s", output);
        write(client_ptr->write_fd, output, strlen(output));

        free(output);

        return 1;
    }

    const size_t command_length = strlen(client_command);

    for (size_t i = 0; i < command_length - 2; ++i)
    {
        if (i < strlen(client_command) - 2 && (client_command[i] < 32 || client_command[i] > 126))
            return 1;
    }

    if (client_command[command_length - 2] == '\n' || client_command[command_length - 1] != '\n')
        return 1;

    client_command[strlen(client_command) - 1] = '\0';

    pthread_mutex_lock(&commands_mutex);

    command** commands_ptr = &commands;

    while (*commands_ptr)
        commands_ptr = &(*commands_ptr)->next;

    *commands_ptr = malloc(sizeof(command));
    (*commands_ptr)->command = strdup(client_command);
    (*commands_ptr)->user = client_ptr->user;
    (*commands_ptr)->next = NULL;
    (*commands_ptr)->write_fd = client_ptr->write_fd;
    (*commands_ptr)->from_client = 1;

    pthread_mutex_unlock(&commands_mutex);

    return 1;
}

// Disconnect 
void remove_client(const pid_t client_pid)
{
    pthread_mutex_lock(&clients_mutex);

    client** clients_ptr = &clients;

    while (*clients_ptr)
    {
        if ((*clients_ptr)->pid == client_pid)
        {
            client* client_ptr = *clients_ptr;

            *clients_ptr = (*clients_ptr)->next;

            free(client_ptr);

            break;
        }

        clients_ptr = &(*clients_ptr)->next;
    }

    pthread_mutex_unlock(&clients_mutex);
}

//Connect 
void* handle_client(void* arg)
{
    client* client_ptr = arg;

    const pid_t pid = client_ptr->pid;

    char fifo_c2s[32], fifo_s2c[32];

    snprintf(fifo_c2s, sizeof(fifo_c2s), FIFO_C2S_FMT, pid);
    mkfifo(fifo_c2s, 0666);

    snprintf(fifo_s2c, sizeof(fifo_s2c), FIFO_S2C_FMT, pid);
    mkfifo(fifo_s2c, 0666);

    kill(pid, SIGRTMIN + 1);

    const int read_fd = open(fifo_c2s, O_RDONLY);
    if (read_fd == -1)
    {
        unlink(fifo_c2s);
        unlink(fifo_s2c);

        printf("Failed to open read FIFO\n");

        return NULL;
    }

    client_ptr->write_fd = open(fifo_s2c, O_WRONLY);
    if (client_ptr->write_fd == -1)
    {
        unlink(fifo_c2s);
        unlink(fifo_s2c);

        printf("Failed to open write FIFO\n");

        close(read_fd);

        return NULL;
    }

    char* buffer = malloc(256);

    while (1)
    {
        const ssize_t length = read(read_fd, buffer, 256);
        if (length <= 0)
            break;

        buffer[length] = '\0';

        printf("Receive: %s", buffer);

        if (client_ptr->role)
        {
            if (buffer[length - 1] == '\n')
                if (!handle_client_command(client_ptr, buffer))
                    break;
        }
        else
        {
            buffer[length - 1] = '\0';

            if (check_role(client_ptr, buffer) == 0)
                break;
        }
    }

    printf("disconnected\n");

    free(buffer);

    close(read_fd);
    close(client_ptr->write_fd);

    unlink(fifo_c2s);
    unlink(fifo_s2c);

    remove_client(client_ptr->pid);

    return NULL;
}

void sig_handler(int sig, siginfo_t* info, void* context)
{
    (void)context;

    if (sig == SIGRTMIN)
    {
        client* client_ptr = malloc(sizeof(client));
        client_ptr->pid = info->si_pid;
        client_ptr->role = NULL;
        client_ptr->next = NULL;

        pthread_mutex_lock(&clients_mutex);

        if (!clients)
            clients = client_ptr;
        else
        {
            client** param = &clients;

            while (*param)
                param = &(*param)->next;

            *param = client_ptr;
        }

        pthread_mutex_unlock(&clients_mutex);

        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, client_ptr);
        pthread_detach(thread);
    }
}

//helper function - broadcast
void* broadcast_loop(void* arg)
{
    (void)arg;

    while (running)
    {
        markdown_increment_version(doc);

        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);

        const long long now = ts.tv_sec * 1000000LL + ts.tv_nsec / 1000LL;

        last_broad_time = now;

        usleep(broad_time * 1000LL - (now - last_broad_time));
    }

    return NULL;
}

int main(const int argc, char* argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <time interval>\n", argv[0]);
        return -1;
    }

    broad_time = atoi(argv[1]);

    if (!read_roles())
        return -1;

    if (pthread_mutex_init(&clients_mutex, NULL) != 0)
    {
        free(roles);

        return -1;
    }

    if (pthread_mutex_init(&commands_mutex, NULL) != 0)
    {
        free(roles);

        return -1;
    }

    doc = markdown_init();
    
    previous_doc = malloc(sizeof(previous_document));
    previous_doc->document = NULL;

    struct sigaction sa;
    sa.sa_sigaction = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    if (sigaction(SIGRTMIN, &sa, NULL) == -1)
    {
        free(roles);

        return -1;
    }

    printf("Server PID: %d\n", getpid());

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    // broad time
    last_broad_time = ts.tv_sec * 1000000L + ts.tv_nsec / 1000L;

    running = 1;

    pthread_t broadcast_thread;
    pthread_create(&broadcast_thread, NULL, broadcast_loop, NULL);

    char* input = malloc(128);
    memset(input, 0, 128);

    // Server commands
    while (1)
    {
        scanf("%s", input);

        if (strcmp(input, "QUIT") == 0)
        {
            if (!clients)
                break;

            const client* client_ptr = clients;

            int connected_client_count = 0;

            while (client_ptr)
            {
                ++connected_client_count;

                client_ptr = (client*)client_ptr->next;
            }

            printf("QUIT rejected, %d clients still connected.\n", connected_client_count);
        }
        else if (strcmp(input, "DOC?") == 0)
        {
            char* content = markdown_flatten(doc);

            printf("%s\n", content);

            free(content);
        }
        else if (strcmp(input, "LOG?") == 0)
        {
            const command* command_history_ptr = commands_history;
            while (command_history_ptr)
            {
                printf("%s", command_history_ptr->command);

                command_history_ptr = command_history_ptr->next;
            }
        }
    }

    running = 0;

    pthread_join(broadcast_thread, NULL);

    while (roles)
    {
        role* target = roles;

        roles = roles->next;

        free(target->user);
        free(target->role);
        free(target);
    }

    markdown_increment_version(doc);

    markdown_free(doc);

    if (input)
        free(input);

    return 0;
}
