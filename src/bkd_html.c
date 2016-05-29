#include "bkd.h"
#include "bkd_html.h"
#include "bkd_utf8.h"

static const char * MARKUP_TAGS[] = {
    "strong",
    "em",
    "code",
    "div",
    "img",
    "a",
    "del",
    "u"
};

/* Use numeric escapes for most things for easier generation */
static size_t html_escape_utf8(uint32_t point, uint8_t buffer[12]) {
    static const uint8_t hexDigits[] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B' ,'C', 'D', 'E', 'F'
    };
    uint8_t octets[4];
    size_t count = bkd_utf8_write(octets, point);
    if (count == 0) { /* Invalid UTF-8 */
        return 0;
    }
    uint32_t requiredLen = 4 + 2 * count;
    buffer[0] = '&';
    buffer[1] = '#';
    buffer[2] = 'x';
    for (uint32_t i = 0; i < count; i++) {
        buffer[3 + 2 * i] = hexDigits[octets[i] >> 4];
        buffer[4 + 2 * i] = hexDigits[octets[i] & 0xF];
    }
    buffer[requiredLen - 1] = ';';
    return requiredLen;
}

static size_t html_write_utf8(uint32_t point, uint8_t buffer[12]) {
    if (point == 60 || point == 62 || point == 38 || point == 34 || point == 39 ||
            point < 32 ||
            (point > 127 && point < 0x10000)) {
        return html_escape_utf8(point, buffer);
    } else {
        return bkd_utf8_write(buffer, point);
    }
}

static void print_html_utf8(struct bkd_ostream * out, uint8_t * s, uint32_t len) {
    uint8_t buffer[12];
    uint32_t codepoint;
    uint8_t * end = s + len;
    while (s < end) {
        s += bkd_utf8_read(s, &codepoint);
        size_t len = html_write_utf8(codepoint, buffer);
        bkd_putn(out, buffer, len);
    }
}

static void print_line(struct bkd_ostream * out, struct bkd_linenode * t) {
    if (t->markupType == BKD_IMAGE) {
        bkd_puts(out, "<img src=\"");
        bkd_putn(out, t->data, t->dataSize);
        bkd_puts(out, "\" alt=\"");
        if (t->nodeCount == 0)
            print_html_utf8(out, t->tree.leaf.text, t->tree.leaf.textLength);
        bkd_puts(out, "\">");
    } else {
        if (t->markupType != BKD_NONE) {
            bkd_putc(out, '<');
            bkd_puts(out, MARKUP_TAGS[t->markupType - 1]);
            if (t->markupType == BKD_LINK) {
                bkd_puts(out, " href=\"");
                bkd_putn(out, t->data, t->dataSize);
                bkd_putc(out, '\"');
            }
            bkd_putc(out, '>');
        }
        if (t->nodeCount == 0) { /* Leaf */
            print_html_utf8(out, t->tree.leaf.text, t->tree.leaf.textLength);
        } else { /* Node */
            for (uint32_t i = 0; i < t->nodeCount; i++)
                print_line(out, t->tree.node + i);
        }
        if (t->markupType != BKD_NONE) {
            bkd_putc(out, '<');
            bkd_putc(out, '/');
            bkd_puts(out, MARKUP_TAGS[t->markupType - 1]);
            bkd_putc(out, '>');
        }
    }
}

static int32_t print_node(struct bkd_ostream * out, struct bkd_node * node, struct bkd_node * parent) {
    uint32_t headerSize;
    uint8_t * text;
    switch (node->type) {
        case BKD_PARAGRAPH:
            bkd_puts(out, "<p>");
            print_line(out, &node->data.paragraph.text);
            bkd_puts(out, "</p>");
            break;
        case BKD_OLIST:
            bkd_puts(out, "<ol>");
            for (uint32_t i = 0; i < node->data.olist.itemCount; i++)
                print_node(out, node->data.olist.items + i, node);
            bkd_puts(out, "</ol>");
            break;
        case BKD_ULIST:
            bkd_puts(out, "<ul>");
            for (uint32_t i = 0; i < node->data.ulist.itemCount; i++)
                print_node(out, node->data.ulist.items + i, node);
            bkd_puts(out, "</ul>");
            break;
        case BKD_TABLE:
            /* TODO */
            break;
        case BKD_HEADER:
            headerSize = node->data.header.size;
            if (headerSize > 6) {
                headerSize = 6;
            }
            uint8_t headerdata[] = { 'h', '0' };
            headerdata[1] += headerSize;
            bkd_putc(out, '<');
            bkd_putn(out, headerdata, 2);
            bkd_putc(out, '>');
            print_line(out, &node->data.header.text);
            bkd_putc(out, '<');
            bkd_putc(out, '/');
            bkd_putn(out, headerdata, 2);
            bkd_putc(out, '>');
            break;
        case BKD_LINEBREAK:
            if (node->data.linebreak.style == BKD_DOTTED) {
                bkd_puts(out, "<hr class=\"bkd_dotted\">");
            } else {
                bkd_puts(out, "<hr class=\"bkd_solid\">");
            }
            break;
        case BKD_CODEBLOCK:
            if (node->data.codeblock.languageLength > 0) {
                bkd_puts(out, "<pre><code data-bkd-language=\"");
                bkd_putn(out, node->data.codeblock.language, node->data.codeblock.languageLength);
                bkd_puts(out, "\">");
            } else {
                bkd_puts(out, "<pre><code>");
            }
            bkd_putn(out, node->data.codeblock.text, node->data.codeblock.textLength);
            bkd_puts(out, "</code></pre>");
            break;
        case BKD_COMMENTBLOCK:
            bkd_puts(out, "<blockquote>");
            print_line(out, &node->data.commentblock.text);
            bkd_puts(out, "</blockquote>");
            break;
        case BKD_TEXT:
            if (parent && (parent->type == BKD_OLIST || parent->type == BKD_ULIST)) {
                bkd_puts(out, "<li>");
                print_line(out, &node->data.text);
                bkd_puts(out, "</li>");
            } else {
                print_line(out, &node->data.text);
            }
            break;
        default:
            return BKD_ERROR_UNKNOWN_NODE;
    }
    return 0;
}

int32_t bkd_html_fragment(struct bkd_ostream * out, struct bkd_node * node) {
    return print_node(out, node, NULL);
}

int32_t bkd_html(struct bkd_ostream * out, struct bkd_document * document) {
    int32_t error;
    bkd_puts(out, "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"></head><body>");
    for (uint32_t i = 0; i < document->itemCount; i++) {
        if ((error = print_node(out, document->items + i, NULL))) {
            BKD_ERROR(error);
            return error;
        }
    }
    bkd_puts(out, "</body></html>");
    return 0;
}
