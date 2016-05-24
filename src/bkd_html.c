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
static size_t html_escape_utf8(uint32_t point, uint8_t *buffer, uint32_t buflen) {
    static const uint8_t hexDigits[] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B' ,'C', 'D', 'E', 'F'
    };
    uint8_t octets[4];
    size_t count = utf8_write(octets, point);
    if (count == 0) { /* Invalid UTF-8 */
        return 0;
    }
    uint32_t requiredLen = 4 + 2 * count;
    if (requiredLen > buflen) {
        return buflen;
    }
    buffer[0] = '&';
    buffer[1] = '#';
    buffer[2] = 'x';
    for (uint32_t i = 0; i < count; i++) {
        uint8_t octet = octets[i];
        buffer[3 + 2 * i] = hexDigits[octet >> 4];
        buffer[4 + 2 * i] = hexDigits[octet & 0xF];
    }
    buffer[requiredLen - 1] = ';';
    return requiredLen;
}

static size_t html_write_utf8(uint32_t point, uint8_t *buffer, size_t buflen) {

    /*
     * Test if the codepoint is one of &, <, >, ', " or if the codepoint is not ascii or CJK character.
     * If UTF-8 character has 3 or more bytes, use normal encoding without escaping to not use too many
     * bytes in the final HTML. Also escape ASCII unprintable characters.
     */
    if (point == 60 || point == 62 || point == 38 || point == 34 || point == 39 ||
            point < 32 ||
            (point > 127 && point < 0x10000)) {
        return html_escape_utf8(point, buffer, buflen);
    } else {
        size_t size = utf8_sizep(point);
        if (size > buflen) {
            return size;
        } else {
            return utf8_write(buffer, point);
        }
    }
}

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
        do l++; while (markups[l].start < pivot);
        do r--; while (markups[r].start >= pivot);
    }
    tag_sort(markups, l - 1);
    tag_sort(markups + l, count - l);
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
    uint32_t next;
    uint32_t nextTag = 0;
    size_t bytesWritten, bytesRead;
    uint32_t readPos = 0;
    uint32_t writePos = 0;
    while ((readPos < t->textLength) && (bytesRead = utf8_read(t->text + readPos, &next))) {
        readPos += bytesRead;
        if (buflen < writePos + 4)
            buffer = BKD_REALLOC(buffer, (buflen = (writePos + 4) * 1.1));
        if(nextTag < 2 * t->markupCount && readPos > tags[nextTag].start) {
            if (tags[nextTag].count == 0) { /* Closing tag */
                buffer[writePos++] = '<';
                buffer[writePos++] = '/';
            } else { /* Opening tag */
                buffer[writePos++] = '<';
            }
            const char * tag = MARKUP_TAGS[tags[nextTag].type];
            size_t taglen = strlen(tag);
            if (buflen < writePos + taglen + 1)
                buffer = BKD_REALLOC(buffer, (buflen = (writePos + taglen + 1) * 1.1));
            memcpy((char *) buffer + writePos, tag, taglen);
            writePos += taglen;
            buffer[writePos++] = '>';
            nextTag++;
        }
        while ((bytesWritten = html_write_utf8(next, buffer + writePos, buflen - writePos)) > buflen - writePos){
            buffer = BKD_REALLOC(buffer, (buflen = (writePos + bytesWritten) * 1.1));
        }
        writePos += bytesWritten;
    }
    *size = writePos;
    return buffer;
}

static inline void put_markup(struct bkd_ostream * out, struct bkd_text * t) {
    uint32_t size;
    uint8_t * chars = markup(t, &size);
    bkd_putn(out, chars, size);
    BKD_FREE(chars);
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
