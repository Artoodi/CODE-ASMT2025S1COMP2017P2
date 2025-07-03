#include "../libs/markdown.h"

int main(void)
{
    document* doc = markdown_init();

    // document_insert(doc, 0, "world");
    // document_insert(doc, 0, "Hello ");

    // document_insert(doc, 0, "BoldText");
    // document_bold(doc, 0, 8);

    // document_insert(doc, 0, "Italic");
    // document_italic(doc, 0, 6);

    // document_insert(doc, 0, "Hello, World.");
    // document_delete(doc, 0, 13);
    // document_insert(doc, 2, "Bar");
    // document_insert(doc, 1, "Foo");

    // document_insert(doc, 0, "My Title");
    // document_heading(doc, 1, 3);

    // document_insert(doc, 0, "Welcome to Markdown");
    // document_heading(doc, 1, 0);
    // document_newline(doc, 19);

    document_insert(doc, 0, "List item");
    document_unordered_list(doc, 0);
    document_newline(doc, 9);
    document_insert(doc, 10, "x = 5;");
    document_code(doc, 10, 10);

    printf("%s\n", markdown_flatten(doc));

    document_free(doc);

    return 0;
}
