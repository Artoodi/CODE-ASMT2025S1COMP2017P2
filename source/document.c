#include "../libs/document.h"

client* clients = NULL;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

command* commands = NULL;
command* commands_history = NULL;
pthread_mutex_t commands_mutex = PTHREAD_MUTEX_INITIALIZER;

previous_document* previous_doc = NULL;
pthread_mutex_t pre_doc_mutex = PTHREAD_MUTEX_INITIALIZER;

// Initialize the document content
void document_init(document* doc)
{
    doc->version = 0;
    doc->content = NULL;
    doc->max_size = 0;
    doc->original_size = 0;
    doc->size = 0;
    doc->locations = NULL;
}

void document_free(const document* doc)
{
    if (doc->content)
    {
        FILE* file = fopen("doc.md", "w");
        if (file == NULL)
        {
            printf("Failed to open file doc.md, please retry\n");

            return;
        }

        char* content = doc->content;
        if (content[doc->size - 1] == '\n' && content[doc->size - 2] == '\n')
            content[doc->size - 1] = '\0';

        fputs(content, file);

        fclose(file);

        free(doc->content);
    }

    free(doc->locations);

    while (commands)
    {
        command* target = commands;

        commands = commands->next;

        free(target->command);
        free(target->user);
        free(target);
    }

    while (commands_history)
    {
        command* target = commands_history;

        commands_history = target->next;

        free(target->command);

        if (target->user)
            free(target->user);

        free(target);
    }
}

int document_insert(document* doc, const size_t pos, const char* content)
{
    if (!doc)
        return 0;

    const uint64_t length = strlen(content);

    if (!doc->content)
    {
        doc->max_size = length * 2;

        doc->content = malloc(length * 2);
        memcpy(doc->content, content, length);
        doc->content[length] = '\0';

        doc->original_size = length;
        doc->size = length;

        doc->locations = malloc(sizeof(location) * (length + 1));

        for (size_t i = 0; i < length + 1; ++i)
        {
            doc->locations[i].after_position = i;
            doc->locations[i].deleted = 0;
        }
    }
    else
    {
        if (pos > doc->original_size)
            return -1;

        const uint64_t after_size = doc->size + length;

        if (after_size >= doc->max_size)
        {
            doc->content = realloc(doc->content, after_size * 2);
            doc->max_size *= 2;
        }

        const location position = doc->locations[pos];

        memmove(doc->content + position.after_position + length, doc->content + position.after_position,
                doc->size - position.after_position);
        memcpy(doc->content + position.after_position, content, length);
        doc->size += length;
        doc->content[doc->size] = '\0';

        for (size_t i = pos; i < doc->original_size + 1; ++i)
        {
            location* location = &doc->locations[i];

            if (!location->deleted)
                location->after_position += length;
        }
    }

    return 0;
}

int document_delete(document* doc, const size_t pos, const size_t len)
{
    if (!doc || !doc->content || pos + len > doc->original_size)
        return -1;

    char* content = doc->content;
    uint64_t* size = &doc->size;

    const location position = doc->locations[pos];

    for (size_t i = len; i > 0; --i)
    {
        location* real_position = &doc->locations[pos + i];
        real_position->deleted = 1;

        if (real_position->after_position == 0)
            memmove(content + real_position->after_position, content + real_position->after_position + 1,
                    *size - real_position->after_position);
        else
            memmove(content + real_position->after_position - 1, content + real_position->after_position,
                    *size - real_position->after_position + 1);

        *size -= 1;
    }

    for (size_t i = 0; i < doc->original_size + 1; ++i)
    {
        location* location = &doc->locations[i];

        if (location->after_position > position.after_position)
        {
            if (location->after_position <= len)
                location->after_position = 0;
            else
                location->after_position -= len;
        }
    }

    content[*size] = '\0';

    return 0;
}

int check_after_newline(document* doc, const size_t pos)
{
    if (!doc || !doc->content)
        return 1;

    if (pos == 0)
        return 1;

    return doc->content[doc->locations[pos].after_position - 1] == '\n';
}

int document_newline(document* doc, const size_t pos)
{
    if (!doc || !doc->content)
        return -1;

    document_insert(doc, pos, "\n");

    return 0;
}

int document_heading(document* doc, const int level, const size_t pos)
{
    if (!doc || !doc->content)
        return -1;

    const int after_newline = check_after_newline(doc, pos);

    switch (level)
    {
    case 1:
        if (after_newline)
            return document_insert(doc, pos, "# ");

        return document_insert(doc, pos, "\n# ");
    case 2:
        if (after_newline)
            return document_insert(doc, pos, "## ");

        return document_insert(doc, pos, "\n## ");
    case 3:
        if (after_newline)
            return document_insert(doc, pos, "### ");

        return document_insert(doc, pos, "\n### ");

    default:
        return -1;
    }
}

int document_bold(document* doc, const size_t start, const size_t end)
{
    if (!doc || !doc->content)
        return -1;

    document_insert(doc, start, "**");

    return document_insert(doc, end, "**");
}

int document_italic(document* doc, const size_t start, const size_t end)
{
    if (!doc || !doc->content)
        return -1;

    document_insert(doc, start, "*");

    return document_insert(doc, end, "*");
}

int document_blockquote(document* doc, const size_t pos)
{
    if (!doc || !doc->content)
        return -1;

    const int after_new_line = check_after_newline(doc, pos);

    if (after_new_line)
        return document_insert(doc, pos, "> ");

    return document_insert(doc, pos, "\n> ");
}

int document_ordered_list(document* doc, const size_t pos)
{
    if (!doc || !doc->content)
        return -1;

    char* content = NULL;
    asprintf(&content, "%s\n", doc->content);

    char* part[30];
    int count = 0;

    char* current = content;

    while (1)
    {
        char* line = strchr(current, '\n');
        if (!line)
            break;

        *line = '\0';

        part[count++] = current;

        current = line + 1;
    }

    size_t index = 0;

    int serial = 0;

    for (int i = 0; i < count; ++i)
    {
        index += strlen(part[i] + 1);

        if (index >= pos)
        {
            if (i != 0)
            {
                char* number = strchr(part[i - 1], '.');
                if (!number)
                    break;

                *number = '\0';

                serial = atoi(part[i - 1]);

                break;
            }
        }
    }

    char* serial_string = NULL;

    const int after_newline = check_after_newline(doc, pos);

    if (serial == 0)
    {
        if (after_newline)
            asprintf(&serial_string, "%d. ", 1);
        else
            asprintf(&serial_string, "\n%d. ", 1);
    }
    else
    {
        if (after_newline)
            asprintf(&serial_string, "%d. ", ++serial);
        else
            asprintf(&serial_string, "\n%d. ", ++serial);
    }

    // if (after_newline)
    //     return document_insert(doc, pos, "1. ");
    //
    // return document_insert(doc, pos, "\n1. ");

    return document_insert(doc, pos, serial_string);
}

int document_unordered_list(document* doc, const size_t pos)
{
    if (!doc || !doc->content)
        return -1;

    const int after_newline = check_after_newline(doc, pos);
    if (after_newline)
        return document_insert(doc, pos, "- ");

    return document_insert(doc, pos, "\n- ");
}

int document_code(document* doc, const size_t start, const size_t end)
{
    if (!doc || !doc->content)
        return -1;

    document_insert(doc, start, "`");

    return document_insert(doc, end, "`");
}

int document_horizontal_rule(document* doc, const size_t pos)
{
    if (!doc || !doc->content)
        return -1;

    const int after_new_line = check_after_newline(doc, pos);

    if (after_new_line)
        return document_insert(doc, pos, "---\n");

    return document_insert(doc, pos, "\n---\n");
}

int document_link(document* doc, const size_t start, const size_t end, const char* url)
{
    if (!doc || !doc->content)
        return -1;

    document_insert(doc, start, "[");
    document_insert(doc, end, "]");
    document_insert(doc, end, "(");
    document_insert(doc, end, url);
    document_insert(doc, end, ")");

    return 0;
}
