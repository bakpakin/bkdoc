/*
 * The main bakdown program. Read bakdown files, and outputs other kinds
 * of files.
 */
#include "bkd.h"
#include "bkd_html.h"
#include <string.h>

static const char * hello = "Hello, World!";

int main (int argc, const char ** argv) {

    /* Assemble test document */
    struct bkd_document doc;
    struct bkd_node nodes[1];
    struct bkd_text texts[3];
    doc.itemCount = 1;
    doc.items = nodes;
    doc.items[0].type = BKD_PARAGRAPH;

    nodes[0].data.paragraph.text.markupType = BKD_NONE;
    nodes[0].data.paragraph.text.nodeCount = 1;
    nodes[0].data.paragraph.text.tree.node = texts;
    nodes[0].data.paragraph.text.data = NULL;
    nodes[0].data.paragraph.text.dataSize = 0;

    texts[0].markupType = BKD_BOLD;
    texts[0].nodeCount = 0;
    texts[0].data = NULL;
    texts[0].dataSize = 0;
    texts[0].tree.leaf.text = (uint8_t *) hello;
    texts[0].tree.leaf.textLength = strlen(hello);

    bkd_html(BKD_STDOUT, &doc);
    return 0;
}
