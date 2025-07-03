#include "../libs/markdown.h"

document* markdown_init()
{
    document* doc = malloc(sizeof(document));

    document_init(doc);

    return doc;
}

void markdown_free(const document* doc)
{
    document_free(doc);
}

void add_command(char* cmd)
{
    pthread_mutex_lock(&commands_mutex);

    command** commands_ptr = &commands;
    while (*commands_ptr)
        commands_ptr = &(*commands_ptr)->next;

    *commands_ptr = malloc(sizeof(command));
    (*commands_ptr)->command = cmd;
    (*commands_ptr)->user = NULL;
    (*commands_ptr)->next = NULL;

    pthread_mutex_unlock(&commands_mutex);
}

// USAGE: INSERT POS CONTENT
int markdown_insert(const document* doc, const uint64_t version, const size_t pos, const char* content)
{
    if (doc->version != version)
        return -3;

    char* command = NULL;
    asprintf(&command, "%lu %s %lu %s", version, "INSERT", pos, content);

    add_command(command);

    return 0;
}

// USAGE: DEL POS LEN
int markdown_delete(const document* doc, const uint64_t version, const size_t pos, const size_t len)
{
    if (doc->version != version)
        return -3;

    char* command = NULL;
    asprintf(&command, "%lu %s %lu %lu", version, "DEL", pos, len);

    add_command(command);

    return 0;
}

// USAGE: NEWLINE POS
int markdown_newline(const document* doc, const uint64_t version, const size_t pos)
{
    if (doc->version != version)
        return -3;

    char* command = NULL;
    asprintf(&command, "%lu %s %lu", version, "NEWLINE", pos);

    add_command(command);

    return 0;
}

// USAGE: HEADING LEVEL POS
int markdown_heading(const document* doc, const uint64_t version, const size_t level, const size_t pos)
{
    if (doc->version != version)
        return -3;

    char* command = NULL;
    asprintf(&command, "%lu %s %lu %lu", version, "HEADING", level, pos);

    add_command(command);

    return 1;
}

// USAGE: BOLD POS_START POS_END
int markdown_bold(const document* doc, const uint64_t version, const size_t start, const size_t end)
{
    if (doc->version != version)
        return -3;

    char* command = NULL;
    asprintf(&command, "%lu %s %lu %lu", version, "BOLD", start, end);

    add_command(command);

    return 0;
}

// USAGE: ITALIC POS_START POS_END
int markdown_italic(const document* doc, const uint64_t version, const size_t start, const size_t end)
{
    if (doc->version != version)
        return -3;

    char* command = NULL;
    asprintf(&command, "%lu %s %lu %lu", version, "ITALIC", start, end);

    add_command(command);

    return 1;
}

// USAGE: BLOCKQUOTE POS
int markdown_blockquote(const document* doc, const uint64_t version, const size_t pos)
{
    if (doc->version != version)
        return -3;

    char* command = NULL;
    asprintf(&command, "%lu %s %lu", version, "BLOCKQUOTE", pos);

    add_command(command);

    return 1;
}

// USAGE: ORDERED_LIST POS
int markdown_ordered_list(const document* doc, const uint64_t version, const size_t pos)
{
    if (doc->version != version)
        return -3;

    char* command = NULL;
    asprintf(&command, "%lu %s %lu", version, "ORDERED_LIST", pos);

    add_command(command);

    return 1;
}

// USAGE: UNORDERED_LIST POS
int markdown_unordered_list(const document* doc, const uint64_t version, const size_t pos)
{
    if (doc->version != version)
        return -3;

    char* command = NULL;
    asprintf(&command, "%lu %s %lu", version, "UNORDERED_LIST", pos);

    add_command(command);

    return 1;
}

// USAGE: CODE POS_START POS_END
int markdown_code(const document* doc, const uint64_t version, const size_t start, const size_t end)
{
    if (doc->version != version)
        return -3;

    char* command = NULL;
    asprintf(&command, "%lu %s %lu %lu", version, "CODE", start, end);

    add_command(command);

    return 1;
}

// USAGE: HORIZONTAL_RULE POS
int markdown_horizontal_rule(const document* doc, const uint64_t version, const size_t pos)
{
    if (doc->version != version)
        return -3;

    char* command = NULL;
    asprintf(&command, "%lu %s %lu", version, "HORIZONTAL_RULE", pos);

    add_command(command);

    return 1;
}

//USAGE: LINK POS_START POS_END URL
int markdown_link(const document* doc, const uint64_t version, const size_t start, const size_t end, const char* url)
{
    if (doc->version != version)
        return -3;

    char* command = NULL;
    asprintf(&command, "%lu %s %lu %lu %s", version, "LINK", start, end, url);

    add_command(command);

    return 1;
}

// Print the content
void markdown_print(const document* doc, FILE* stream)
{
    printf("%lu\n", doc->version);

    (void)stream;
}

// Add the '\0' at the end
char* markdown_flatten(const document* doc)
{
    char* content = NULL;

    if (!doc || !doc->content)
    {
        content = malloc(1);
        *content = '\0';

        return content;
    }

    content = malloc(doc->size + 1);
    memcpy(content, doc->content, doc->size);
    content[doc->size] = '\0';

    return content;
}

// Handle commands sent from client
int process_client_command(document* doc, const char* command, const int from_client)
{
    char* temp = NULL;
    asprintf(&temp, "%s ", command);

    char* part[100];
    int count = 0;

    char* current = temp;

    while (1)
    {
        char* line = strchr(current, ' ');
        if (!line)
            break;

        if (from_client == 1)
        {
            if (count < 2)
                *line = '\0';
        }
        else
        {
            if (count < 3)
                *line = '\0';
        }

        part[count++] = current;

        current = line + 1;
    }

    int result = -4;

    if (from_client == 1)
    {
        switch (part[0][0])
        {
        case 'I':
            if (strcmp(part[0], "INSERT") == 0)
            {
                part[2][strlen(part[2]) - 1] = '\0';

                result = document_insert(doc, atoi(part[1]), part[2]);
            }
            else if (strcmp(part[0], "ITALIC") == 0)
                result = document_italic(doc, atoi(part[1]), atoi(part[2]));

            break;

        case 'D':
            if (strcmp(part[0], "DEL") == 0)
                result = document_delete(doc, atoi(part[1]), atoi(part[2]));

            break;

        case 'B':
            if (strcmp(part[0], "BOLD") == 0)
                result = document_bold(doc, atoi(part[1]), atoi(part[2]));
            else if (strcmp(part[0], "BLOCKQUOTE") == 0)
                result = document_blockquote(doc, atoi(part[1]));

            break;

        case 'H':
            if (strcmp(part[0], "HEADING") == 0)
                result = document_heading(doc, atoi(part[1]), atoi(part[2]));
            else if (strcmp(part[0], "HORIZONTAL_RULE") == 0)
                result = document_horizontal_rule(doc, atoi(part[1]));

            break;

        case 'N':
            if (strcmp(part[0], "NEWLINE") == 0)
                result = document_newline(doc, atoi(part[1]));

            break;

        case 'U':
            if (strcmp(part[0], "UNORDERED_LIST") == 0)
                result = document_unordered_list(doc, atoi(part[1]));

            break;

        case 'C':
            if (strcmp(part[0], "CODE") == 0)
                result = document_code(doc, atoi(part[1]), atoi(part[2]));

            break;

        case 'L':
            if (strcmp(part[0], "LINK") == 0)
            {
                part[3][strlen(part[3]) - 1] = '\0';

                result = document_link(doc, atoi(part[1]), atoi(part[2]), part[3]);
            }

            break;

        case 'O':
            if (strcmp(part[0], "ORDERED_LIST") == 0)
                result = document_ordered_list(doc, atoi(part[1]));

            break;
        }
    }
    else
    {
        switch (part[1][0])
        {
        case 'I':
            if (strcmp(part[1], "INSERT") == 0)
            {
                part[3][strlen(part[3]) - 1] = '\0';

                result = document_insert(doc, atoi(part[2]), part[3]);
            }
            else if (strcmp(part[1], "ITALIC") == 0)
                result = document_italic(doc, atoi(part[2]), atoi(part[3]));

            break;

        case 'D':
            if (strcmp(part[1], "DEL") == 0)
                result = document_delete(doc, atoi(part[2]), atoi(part[3]));

            break;

        case 'B':
            if (strcmp(part[1], "BOLD") == 0)
                result = document_bold(doc, atoi(part[2]), atoi(part[3]));
            else if (strcmp(part[1], "BLOCKQUOTE") == 0)
                result = document_blockquote(doc, atoi(part[2]));

            break;

        case 'H':
            if (strcmp(part[1], "HEADING") == 0)
                result = document_heading(doc, atoi(part[2]), atoi(part[3]));
            else if (strcmp(part[1], "HORIZONTAL_RULE") == 0)
                result = document_horizontal_rule(doc, atoi(part[2]));

            break;

        case 'N':
            if (strcmp(part[1], "NEWLINE") == 0)
                result = document_newline(doc, atoi(part[2]));

            break;

        case 'U':
            if (strcmp(part[1], "UNORDERED_LIST") == 0)
                result = document_unordered_list(doc, atoi(part[2]));

            break;

        case 'C':
            if (strcmp(part[1], "CODE") == 0)
                result = document_code(doc, atoi(part[2]), atoi(part[3]));

            break;

        case 'L':
            if (strcmp(part[1], "LINK") == 0)
            {
                part[4][strlen(part[4]) - 1] = '\0';

                result = document_link(doc, atoi(part[2]), atoi(part[3]), part[4]);
            }

            break;

        case 'O':
            if (strcmp(part[1], "ORDERED_LIST") == 0)
                result = document_ordered_list(doc, atoi(part[2]));

            break;
        }
    }

    free(temp);

    return result;
}

// Version control
void markdown_increment_version(document* doc)
{
    if (!doc)
        return;

    char* broad = malloc(300);
    snprintf(broad, 300, "VERSION %lu\n", doc->version);

    size_t size = strlen(broad);

    doc->original_size = doc->size;

    doc->locations = realloc(doc->locations, sizeof(location) * (doc->original_size + 1));

    for (size_t i = 0; i < doc->original_size + 1; ++i)
    {
        doc->locations[i].after_position = i;
        doc->locations[i].deleted = 0;
    }

    const int has_command = commands != NULL;

    if (has_command)
    {
        pthread_mutex_lock(&commands_mutex);
        command* commands_ptr = commands;

        command** commands_history_ptr = &commands_history;
        while (*commands_history_ptr)
            commands_history_ptr = &(*commands_history_ptr)->next;

        while (commands_ptr)
        {
            command* target = commands_ptr;

            const int result = process_client_command(doc, target->command, target->from_client);

            *commands_history_ptr = malloc(sizeof(command));

            if (target->user)
            {
                switch (result)
                {
                case 0:
                    asprintf(&(*commands_history_ptr)->command, "EDIT %s %s SUCCESS\n", target->user, target->command);

                    break;

                case -1:
                    asprintf(&(*commands_history_ptr)->command, "EDIT %s %s Reject INVALID_POSITION\n", target->user,
                             target->command);

                    break;

                case -2:
                    asprintf(&(*commands_history_ptr)->command, "EDIT %s %s Reject DELETED_POSITION\n", target->user,
                             target->command);

                    break;

                case -3:
                    asprintf(&(*commands_history_ptr)->command, "EDIT %s %s Reject OUTDATED_VERSION\n", target->user,
                             target->command);

                    break;

                default:
                    asprintf(&(*commands_history_ptr)->command, "EDIT %s %s Reject UNKNOWN_ERROR\n", target->user,
                             target->command);

                    break;
                }
            }
            else
            {
                switch (result)
                {
                case 0:
                    asprintf(&(*commands_history_ptr)->command, "EDIT %s SUCCESS\n", target->command);

                    break;

                case -1:
                    asprintf(&(*commands_history_ptr)->command, "EDIT %s Reject INVALID_POSITION\n", target->command);

                    break;

                case -2:
                    asprintf(&(*commands_history_ptr)->command, "EDIT %s Reject DELETED_POSITION\n", target->command);

                    break;

                case -3:
                    asprintf(&(*commands_history_ptr)->command, "EDIT %s Reject OUTDATED_VERSION\n", target->command);

                    break;

                default:
                    asprintf(&(*commands_history_ptr)->command, "EDIT %s Reject UNKNOWN_ERROR\n", target->command);

                    break;
                }
            }

            write(commands_ptr->write_fd, (*commands_history_ptr)->command, strlen((*commands_history_ptr)->command));

            (*commands_history_ptr)->user = NULL;
            (*commands_history_ptr)->next = NULL;

            memcpy(broad + size, (*commands_history_ptr)->command, strlen((*commands_history_ptr)->command));

            size += strlen((*commands_history_ptr)->command);

            commands_history_ptr = &(*commands_history_ptr)->next;

            commands_ptr = target->next;

            free(target->command);
            free(target);
        }

        commands = NULL;

        ++doc->version;

        pthread_mutex_unlock(&commands_mutex);
    }

    memcpy(broad + size, "END\n", 4);

    size += 4;

    broad[size] = '\0';

    pthread_mutex_lock(&clients_mutex);

    const client* clients_ptr = clients;
    while (clients_ptr)
    {
        write(clients_ptr->write_fd, broad, strlen(broad));

        clients_ptr = clients_ptr->next;
    }

    pthread_mutex_unlock(&clients_mutex);

    free(broad);
}
