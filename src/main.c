/*
 * The main BKD program. Read bkd files, and outputs other kinds
 * of files.
 */
#include "bkd.h"
#include "bkd_html.h"
#include <string.h>

static const char * hello = "Hello, World!";
static const char * helloimg = "http://betanews.com/wp-content/uploads/2015/07/hello-600x400.jpg";

int main (int argc, const char ** argv) {

    /* Assemble test document */
    struct bkd_document doc;
    struct bkd_node nodes[1];
    struct bkd_linenode texts[3];
    doc.itemCount = 1;
    doc.items = nodes;
    doc.items[0].type = BKD_PARAGRAPH;

    nodes[0].data.paragraph.text.markupType = BKD_NONE;
    nodes[0].data.paragraph.text.nodeCount = 1;
    nodes[0].data.paragraph.text.tree.node = texts;
    nodes[0].data.paragraph.text.data = NULL;
    nodes[0].data.paragraph.text.dataSize = 0;

    texts[0].markupType = BKD_IMAGE;
    texts[0].nodeCount = 0;
    texts[0].data = (uint8_t *) helloimg;
    texts[0].dataSize = strlen(helloimg);
    texts[0].tree.leaf.text = (uint8_t *) hello;
    texts[0].tree.leaf.textLength = strlen(hello);

    /* bkd_html(BKD_STDOUT, &doc); */
    /* bkd_putc(BKD_STDOUT, '\n'); */
    /* char buffer[256]; */
    /* int i = 1; */

    /* while (fgets(buffer, sizeof(buffer), stdin)) { */
    /*     printf("%d %s", i, buffer); */
    /*     i++; */
    /* } */

    size_t n;
    uint8_t * line = bkd_getl(BKD_STDIN, &n);
    if (line) {
        bkd_putn(BKD_STDOUT, line, n);
    } else {
        bkd_puts(BKD_STDOUT, "No line...\n");
    }

    return 0;
}
