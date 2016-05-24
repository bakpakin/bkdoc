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
    struct bkd_markup markups[1];
    doc.itemCount = 1;
    doc.items = docnodes;
    doc.items[0].type = BKD_PARAGRAPH;
    doc.items[0].data.paragraph.text.text = (uint8_t *) hello;
    doc.items[0].data.paragraph.text.textLength = strlen(hello);
    doc.items[0].data.paragraph.text.markups = markups;
    doc.items[0].data.paragraph.text.markupCount = 1;

    markups[0].start = 0;
    markups[0].count = 5;
    markups[0].data = NULL;
    markups[0].dataSize = 0;
    markups[0].type = BKD_BOLD;

    bkd_html(BKD_STDOUT, &doc);
    return 0;
}
