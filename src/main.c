/*
 * The main BKD program. Read bkd files, and outputs other kinds
 * of files.
 */
#include "bkd.h"
#include "bkd_html.h"
#include <string.h>

static const char * hello = "![image](http://www.covchurch.org/wp-content/uploads/sites/18/2013/01/Hello-Friends-logo-300x209.png)";
static const char * hello2 = "![image]";
static const char * hello3 = "_Hello_ *World!* ~Mud mud~.";

int main (int argc, const char ** argv) {

    /* Assemble test document */
    struct bkd_document doc;
    struct bkd_node nodes[1];
    struct bkd_linenode texts[3];
    doc.itemCount = 1;
    doc.items = nodes;
    doc.items[0].type = BKD_PARAGRAPH;

    bkd_parse_line(&nodes[0].data.paragraph.text, (uint8_t *) hello, strlen(hello));

    bkd_html(BKD_STDOUT, &doc);
    bkd_putc(BKD_STDOUT, '\n');

    return 0;
}
