#include "bkd.h"
#include "bkd_html.h"
#include "bkd_utf8.h"

/* Use numeric escapes for most things for easier generation */
static size_t html_escape_utf8(uint32_t point, uint8_t buffer[12]) {
    static const uint8_t hexDigits[] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B' ,'C', 'D', 'E', 'F'
    };
    uint8_t digitStack[12];
    uint32_t digitsTop = 0;
    uint32_t len = 3;
    buffer[0] = '&';
    buffer[1] = '#';
    buffer[2] = 'x';
    while (point > 0) {
        digitStack[digitsTop++] = hexDigits[point % 16];
        point /= 16;
    }
    while (digitsTop > 0) {
        buffer[len++] = digitStack[--digitsTop];
    }
    buffer[len++] = ';';
    return len;
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

static const uint64_t
    htmlflag_newline = 1 << 0;

#define CASE(codepoint, flag, string) case codepoint: if (flags & (flag)) { bkd_puts(out, string); continue; } break;

static void print_html_utf8(struct bkd_ostream * out, struct bkd_string string, uint64_t flags) {
    uint8_t buffer[12];
    uint32_t codepoint;
    uint32_t pos = 0;
    while (pos < string.length) {
        pos += bkd_utf8_read(string.data + pos, &codepoint);
        switch (codepoint) {
            CASE('\n', htmlflag_newline, "<br>")
            default:
                break;
        }
        size_t len = html_write_utf8(codepoint, buffer);
        struct bkd_string bufferstring = {
            len,
            buffer
        };
        bkd_putn(out, bufferstring);
    }
}

#undef CASE

static void print_line(struct bkd_ostream * out, struct bkd_linenode * t);

static void print_codeinline(struct bkd_ostream * out, struct bkd_linenode * t) {
    uint32_t i;
    if (t->markup & BKD_CODEINLINE) bkd_puts(out, "<code>");
    if (t->nodeCount > 0) {
        for (i = 0; i < t->nodeCount; i++) {
            print_line(out, t->tree.node + i);
        }
    } else {
        print_html_utf8(out, t->tree.leaf, 1);
    }
    if (t->markup & BKD_CODEINLINE) bkd_puts(out, "</code>");
}

static void print_image(struct bkd_ostream * out, struct bkd_linenode * t) {
    if (t->markup & BKD_IMAGE) {
        bkd_puts(out, "<img src=\"");
        print_html_utf8(out, t->data, 0);
        bkd_puts(out, "\"></img>");
    } else {
        return print_codeinline(out, t);
    }
}

static void print_math(struct bkd_ostream * out, struct bkd_linenode * t) {
    /* NYI */
    return print_image(out, t);
}

static void print_link(struct bkd_ostream * out, struct bkd_linenode * t) {
    if (t->markup & BKD_LINK) {
        bkd_puts(out, "<a href=\"");
        print_html_utf8(out, t->data, 1);
        bkd_puts(out, "\">");
        print_math(out, t);
        bkd_puts(out, "</a>");
    } else {
        return print_math(out, t);
    }
}

static void print_underline(struct bkd_ostream * out, struct bkd_linenode * t) {
    if (t->markup & BKD_UNDERLINE) bkd_puts(out, "<u>");
    print_link(out, t);
    if (t->markup & BKD_UNDERLINE) bkd_puts(out, "</u>");
}

static void print_strikethrough(struct bkd_ostream * out, struct bkd_linenode * t) {
    if (t->markup & BKD_STRIKETHROUGH) bkd_puts(out, "<del>");
    print_underline(out, t);
    if (t->markup & BKD_STRIKETHROUGH) bkd_puts(out, "</del>");
}

static void print_italics(struct bkd_ostream * out, struct bkd_linenode * t) {
    if (t->markup & BKD_ITALICS) bkd_puts(out, "<em>");
    print_strikethrough(out, t);
    if (t->markup & BKD_ITALICS) bkd_puts(out, "</em>");
}

static void print_line(struct bkd_ostream * out, struct bkd_linenode * t) {
    if (t->markup & BKD_BOLD) bkd_puts(out, "<strong>");
    print_italics(out, t);
    if (t->markup & BKD_BOLD) bkd_puts(out, "</strong>");
}

static int32_t print_node(struct bkd_ostream * out, struct bkd_node * node, struct bkd_node * parent) {
    uint32_t headerSize;
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
            uint8_t headerdata[2] = {'h', '0'};
            struct bkd_string headerString = {
                2,
                headerdata
            };
            headerdata[1] += headerSize;
            bkd_putc(out, '<');
            bkd_putn(out, headerString);
            bkd_putc(out, '>');
            print_line(out, &node->data.header.text);
            bkd_putc(out, '<');
            bkd_putc(out, '/');
            bkd_putn(out, headerString);
            bkd_putc(out, '>');
            break;
        case BKD_HORIZONTALRULE:
            if (node->data.linebreak.style == BKD_DOTTED) {
                bkd_puts(out, "<hr class=\"bkd_dotted\">");
            } else {
                bkd_puts(out, "<hr class=\"bkd_solid\">");
            }
            break;
        case BKD_CODEBLOCK:
            if (node->data.codeblock.language.length > 0) {
                bkd_puts(out, "<pre><code data-bkd-language=\"");
                print_html_utf8(out, node->data.codeblock.language, 0);
                /* bkd_putn(out, node->data.codeblock.language); */
                bkd_puts(out, "\">");
            } else {
                bkd_puts(out, "<pre><code>");
            }
            print_html_utf8(out, node->data.datastring, 0);
            /* bkd_putn(out, node->data.codeblock.text); */
            bkd_puts(out, "</code></pre>");
            break;
        case BKD_COMMENTBLOCK:
            bkd_puts(out, "<blockquote>");
            print_line(out, &node->data.commentblock.text);
            bkd_puts(out, "</blockquote>");
            break;
        case BKD_DOCUMENT:
            bkd_puts(out, "<div class=\"bkd-subdoc\">");
            for (uint32_t i = 0; i < node->data.subdoc.itemCount; i++)
                print_node(out, node->data.subdoc.items + i, node);
            bkd_puts(out, "</div>");
            break;
        case BKD_DATASTRING:
            bkd_puts(out, "<div hidden class=\"bkd-datastring\">");
            print_html_utf8(out, node->data.datastring, 0);
            bkd_puts(out, "</div>");
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
