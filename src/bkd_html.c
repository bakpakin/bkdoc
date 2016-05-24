#include "bkd.h"
#include "bkd_html.h"
#include <string.h>

static const char * MARKUP_TAGS[] = {
    "strong",
    "em",
    "code",
    "div",
    "img",
    "href"
};

static size_t utf8_sizep(uint32_t codepoint) {
    if (codepoint < 0x80) {
        return 1;
    } else if (codepoint < 0x800) {
        return 2;
    } else if (codepoint < 0x10000) {
        return 3;
    } else if (codepoint < 0x200000) {
        return 4;
    } else {
        return 0;
    }
}

static size_t utf8_sizeb(uint8_t head) {
    if ((head & 0x80) == 0) {
        return 1;
    } else if ((head & 0xE0) == 0xC0) {
        return 2;
    } else if ((head & 0xF0) == 0xE0) {
        return 3;
    } else if ((head & 0xF8) == 0xF0) {
        return 4;
    } else {
        return 0;
    }
}

static size_t utf8_write(uint8_t * s, uint32_t value) {
    uint8_t octets[4];
    size_t count;
    if (value < 0x80) {
        count = 1;
        octets[0] = value;
    } else if (value < 0x800) {
        count = 2;
        octets[0] = (value >> 6) | 0xC0;
        octets[1] = (value & 0x3F) | 0x80;
    } else if (value < 0x10000) {
        count = 3;
        octets[0] = ((value >> 12) & 0x0F) | 0xE0;
        octets[1] = ((value >> 6) & 0x3F) | 0x80;
        octets[2] = (value & 0x3F) | 0x80;
    } else if (value < 0x200000) {
        count = 4;
        octets[0] = ((value >> 18) & 0x07) | 0xF0;
        octets[1] = ((value >> 12) & 0x3F) | 0x80;
        octets[2] = ((value >> 6) & 0x3F) | 0x80;
        octets[3] = (value & 0x3F) | 0x80;
    } else {
        return 0;
    }
    memcpy(s, octets, count);
    return count;
}

static size_t utf8_read(uint8_t * s, uint32_t * ret) {
    uint8_t head = *s;
    if ((head & 0x80) == 0) {
        *ret = head;
        return 1;
    } else if ((head & 0xE0) == 0xC0) {
        *ret = (s[1] & 0x3F) + (head & 0x1F) * 0x40;
        return 2;
    } else if ((head & 0xF0) == 0xE0) {
        *ret = (s[2] & 0x3F) + (s[1] & 0x3F) * 0x20 + (head & 0x0F) * 0x800;
        return 3;
    } else if ((head & 0xF8) == 0xF0) {
        *ret = (s[3] & 0x3F) + (s[2] & 0x3F) * 0x10 + (s[1] & 0x3F) * 0x400 + (head & 0x07) * 0x10000;
        return 4;
    } else {
        return 0;
    }
}

/* Use numeric escapes for most things for easier generation */
static size_t html_escape_utf8(uint32_t point, uint8_t buffer[12]) {
    static const uint8_t hexDigits[] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B' ,'C', 'D', 'E', 'F'
    };
    uint8_t octets[4];
    size_t count = utf8_write(octets, point);
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

    /*
     * Test if the codepoint is one of &, <, >, ', " or if the codepoint is not ascii or CJK character.
     * If UTF-8 character has 3 or more bytes, use normal encoding without escaping to not use too many
     * bytes in the final HTML. Also escape ASCII unprintable characters.
     */
    if (point == 60 || point == 62 || point == 38 || point == 34 || point == 39 ||
            point < 32 ||
            (point > 127 && point < 0x10000)) {
        return html_escape_utf8(point, buffer);
    } else {
        return utf8_write(buffer, point);
    }
}

static void put_markup(struct bkd_ostream * out, struct bkd_text * t) {
    uint8_t buffer[12];
    uint32_t codepoint;
    if (t->markupType != BKD_NONE) {
        bkd_putc(out, '<');
        bkd_puts(out, MARKUP_TAGS[t->markupType - 1]);
        bkd_putc(out, '>');
    }
    if (t->nodeCount == 0) { /* Leaf */
        uint8_t * c = t->tree.leaf.text;
        uint8_t * end = c + t->tree.leaf.textLength;
        while (c < end) {
            c += utf8_read(c, &codepoint);
            size_t len = html_write_utf8(codepoint, buffer);
            bkd_putn(out, buffer, len);
        }
    } else { /* Node */
        for (uint32_t i = 0; i < t->nodeCount; i++)
            put_markup(out, t->tree.node + i);
    }
    if (t->markupType != BKD_NONE) {
        bkd_putc(out, '<');
        bkd_putc(out, '/');
        bkd_puts(out, MARKUP_TAGS[t->markupType - 1]);
        bkd_putc(out, '>');
    }
}

static int32_t print_node(struct bkd_ostream * out, struct bkd_node * node, struct bkd_node * parent) {
    uint32_t headerSize;
    uint8_t * text;
    switch (node->type) {
        case BKD_PARAGRAPH:
            bkd_puts(out, "<p>");
            put_markup(out, &node->data.paragraph.text);
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
            put_markup(out, &node->data.header.text);
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
            put_markup(out, &node->data.commentblock.text);
            bkd_puts(out, "</blockquote>");
            break;
        case BKD_TEXT:
            if (parent && (parent->type == BKD_OLIST || parent->type == BKD_ULIST)) {
                bkd_puts(out, "<li>");
                put_markup(out, &node->data.text);
                bkd_puts(out, "</li>");
            } else {
                put_markup(out, &node->data.text);
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
