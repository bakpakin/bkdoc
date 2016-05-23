#include "bkd.h"
#include "bkd_html.h"
#include <string.h>

static const char * MARKUP_TAGS[] = {
    "strong",
    "em",
    "code",
    "img",
    "href"
};

/* Use quicksort to sort tags in accending order based on start index */
static void tag_sort(struct bkd_markup * markups, uint32_t count) {
    if (count < 2) return;
    uint32_t l = 0, r = count - 1;
    uint32_t pivot = markups[count / 2].start;
    struct bkd_markup temp;
    while (l < r) {
        if (markups[r].start < markups[l].start) {
            temp = markups[l];
            markups[l] = markups[r];
            markups[r] = temp;
        }
        do l++; while (markups[l].start <= pivot);
        do r--; while (markups[r].start >= pivot);
    }
    tag_sort(markups, l - 1);
    tag_sort(markups + l, count - l);
}

static inline unsigned utf8_charsize(uint8_t head) {
    if (head & 0x80) {
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

#define utf8_charsizes(s) utf8_charsize(*(s))
#define utf8_charsizep(c) utf8_charsize((c) & 0xFF)

static int utf8_write(uint8_t ** s, uint64_t value) {
    uint8_t octets[4];
    unsigned count = utf8_charsizep(value);
    if (count == 0) {
        return 1;
    }
    octets[0] = value & 0xFF;
    if (count > 1) {
        octets[1] = (value & 0xFFFF) >> 0x8;
        if (count > 2) {
            octets[2] = (value & 0xFFFFFF) >> 0x10;
            if (count > 3)
            octets[3] = (value & 0xFFFFFFFF) >> 0x18;
        }
    }
    memcpy(*s, octets, count);
    *s += count;
    return 0;
}

static int utf8_read(uint8_t ** s, uint64_t * ret) {
    uint8_t * c = *s;
    uint8_t head = *c;
    uint64_t value;
    if (head & 0x80) {
        value = head;
        c += 1;
    } else if ((head & 0xE0) == 0xC0) {
        value = head + c[1] * 0x100;
        c += 2;
    } else if ((*c & 0xF0) == 0xE0) {
        value = head + c[1] * 0x100 + c[2] * 0x10000;
        c += 3;
    } else if ((*c & 0xF8) == 0xF0) {
        value = head + c[1] * 0x100 + c[2] * 0x10000 + c[3] * 0x1000000;
        c += 4;
    } else {
        return 1;
    }
    *s = c;
    *ret = value;
    return 0;
}

/* Use numeric escapes for most things for easier generation */
static int html_escape_utf8(uint64_t point, uint8_t *buffer, uint32_t buflen) {
    static const char hexDigits[] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B' ,'C', 'D', 'E', 'F'
    };
    uint32_t requiredLen = 4 + 2 * utf8_charsizep(point);
    if (requiredLen == 4) { /* Invalid UTF-8 */
        return 1;
    }


    return 0;
}

static uint8_t * markup(struct bkd_text * t, uint32_t * size) {
    uint32_t buflen = t->textLength * 1.1;
    uint8_t * buffer = BKD_MALLOC(buflen);

    /* Sort markup tags in a structure for sequential access and modfication */
    struct bkd_markup tags[2 * t->markupCount];
    for (uint32_t i = 0; i < t->markupCount; i++) {
        struct bkd_markup * m = t->markups + i;
        tags[2 * i] = (struct bkd_markup) {
            m->type,
            m->start,
            m->count,
            m->data,
            m->dataSize
        };
        tags[2 * i + 1] = (struct bkd_markup) {
            m->type,
            m->start + m->count,
            0,
            m->data,
            m->dataSize
        };
    }
    tag_sort(tags, 2 * t->markupCount);

    /* Put marked up text in buffer */

    memcpy(buffer, t->text, t->textLength);
    *size = t->textLength;
    return buffer;
}

static int32_t print_node(struct bkd_ostream * out, struct bkd_node * node, struct bkd_node * parent) {
    uint32_t size;
    uint32_t headerSize;
    uint8_t * text;
    switch (node->type) {
        case BKD_PARAGRAPH:
            bkd_puts(out, "<p>");
            text = markup(&node->data.paragraph.text, &size);
            bkd_putn(out, text, size);
            BKD_FREE(text);
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
            text = markup(&node->data.header.text, &size);
            bkd_putn(out, text, size);
            BKD_FREE(text);
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
            text = markup(&node->data.commentblock.text, &size);
            bkd_putn(out, text, size);
            BKD_FREE(text);
            bkd_puts(out, "</blockquote>");
            break;
        case BKD_TEXT:
            if (parent && (parent->type == BKD_OLIST || parent->type == BKD_ULIST)) {
                bkd_puts(out, "<li>");
                text = markup(&node->data.text, &size);
                bkd_putn(out, text, size);
                BKD_FREE(text);
                bkd_puts(out, "</li>");
            } else {
                text = markup(&node->data.text, &size);
                bkd_putn(out, text, size);
                BKD_FREE(text);
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
