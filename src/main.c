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
    struct bkd_node docnodes[1];
    doc.itemCount = 1;
    doc.items = docnodes;
    doc.items[0].type = BKD_PARAGRAPH;
    doc.items[0].data.paragraph.text.text = (uint8_t *) hello;
    doc.items[0].data.paragraph.text.textLength = strlen(hello);
    doc.items[0].data.paragraph.text.markups = NULL;
    doc.items[0].data.paragraph.text.markupCount = 0;

    bkd_html(BKD_STDOUT, &doc);
    return 0;
}
