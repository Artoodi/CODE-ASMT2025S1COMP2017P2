#include "../libs/markdown.h"

int main(void)
{
    document* doc = markdown_init();

    markdown_insert(doc, 0, 0, "world");
    markdown_insert(doc, 0, 0, "hello ");

    char* doc_content = markdown_flatten(doc);

    markdown_increment_version(doc);

    if (doc_content)
    {
        printf("%s\n", doc_content);

        free(doc_content);
    }

    markdown_free(doc);

    return 0;
}
