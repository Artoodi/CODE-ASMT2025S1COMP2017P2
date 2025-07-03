#ifndef MARKDOWN_H
#define MARKDOWN_H

#include "document.h"
/*
 *
 * The given file contains all the functions you will be required to complete. You are free to and encouraged to create
 * more helper functions to help assist you when creating the document. For the automated marking you can expect unit tests
 * for the following tests, verifying if the document functionalities are correctly implemented. All the commands are explained
 * in detail in the assignment spec.
 *
 *
 * SUCCESS = 0,
 * INVALID_CURSOR_POS = -1,
 * DELETED_POSITION = -2,
 * OUTDATED_VERSION = -3,
 * Return -1 if the cursor position is invalid
*/

// Initialize and free a document
document* markdown_init();
void markdown_free(const document* doc);

void add_command(char* cmd);

// === Edit Commands ===
int markdown_insert(const document* doc, uint64_t version, size_t pos, const char* content);
int markdown_delete(const document* doc, uint64_t version, size_t pos, size_t len);

// === Formatting Commands ===
int markdown_newline(const document* doc, uint64_t version, size_t pos);
int markdown_heading(const document* doc, uint64_t version, size_t level, size_t pos);
int markdown_bold(const document* doc, uint64_t version, size_t start, size_t end);
int markdown_italic(const document* doc, uint64_t version, size_t start, size_t end);
int markdown_blockquote(const document* doc, uint64_t version, size_t pos);
int markdown_ordered_list(const document* doc, uint64_t version, size_t pos);
int markdown_unordered_list(const document* doc, uint64_t version, size_t pos);
int markdown_code(const document* doc, uint64_t version, size_t start, size_t end);
int markdown_horizontal_rule(const document* doc, uint64_t version, size_t pos);
int markdown_link(const document* doc, uint64_t version, size_t start, size_t end, const char* url);

// === Utilities ===
void markdown_print(const document* doc, FILE* stream);
char* markdown_flatten(const document* doc);

// === Versioning ===
int process_client_command(document* doc, const char* command, int from_client);
void markdown_increment_version(document* doc);

#endif //MARKDOWN_H
