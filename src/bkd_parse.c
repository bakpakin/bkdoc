/*
Copyright (c) 2016 Calvin Rose

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "bkd.h"
#include "bkd_utf8.h"
#include "bkd_string.h"
#include "bkd_stretchy.h"

#include <string.h>

static inline uint8_t readHex(uint32_t codepoint) {
    if (codepoint >= '0' && codepoint <= '9')
       return (uint8_t) (codepoint - '0');
    if (codepoint >= 'A' && codepoint <= 'F')
        return 10 + (uint8_t) (codepoint - 'A');
    if (codepoint >= 'a' && codepoint <= 'f')
        return 10 + (uint8_t) (codepoint - 'a');
    return 0;
}

/* Reads and returns an escape sequence in a string */
static inline uint32_t read_escape(struct bkd_string string, uint32_t * escapeLength) {
    uint32_t codepoint, accumulator, length;
    bkd_utf8_readlen(string.data, &codepoint, string.length);
    switch (codepoint) {
        case 'b':
            *escapeLength = 1;
            return '\b';
        case 'f':
            *escapeLength = 1;
            return '\f';
        case 'v':
            *escapeLength = 1;
            return '\v';
        case 'r':
            *escapeLength = 1;
            return '\r';
        case 'n':
            *escapeLength = 1;
            return '\n';
        case 't':
            *escapeLength = 1;
            return '\t';
        case '(': /* Unicode escape */
            accumulator = 0;
            length = 1;
            while (length < string.length) {
                length += bkd_utf8_readlen(string.data + length, &codepoint, string.length - length);
                if (codepoint == ')') break;
                accumulator = 16 * accumulator + readHex(codepoint);
            }
            *escapeLength = length;
            return accumulator;
        default:
            *escapeLength = bkd_utf8_sizep(codepoint);
            return codepoint;
    }
}

struct bkd_string bkd_strescape_new(struct bkd_string string) {
    struct bkd_string ret;
    uint32_t inNext = 0;
    uint32_t retNext = 0;
    uint32_t escapeLength = 0;
    uint32_t codepoint;
    if (string.length == 0) return BKD_NULLSTR;
    ret.data = BKD_MALLOC(string.length);
    if (!ret.data) {
        BKD_ERROR(BKD_ERROR_OUT_OF_MEMORY);
        return BKD_NULLSTR;
    }
    while (inNext < string.length) {
        inNext += bkd_utf8_readlen(string.data + inNext, &codepoint, string.length - inNext);
        if (codepoint == '\\') {
            if (inNext >= string.length) /* Terminating escapes are ignored. */
                break;
            codepoint = read_escape(bkd_strsub(string, inNext, -1), &escapeLength);
            inNext += escapeLength;
        }
        retNext += bkd_utf8_write(ret.data + retNext, codepoint);
    }
    ret.data = BKD_REALLOC(ret.data, retNext);
    ret.length = retNext;
    return ret;
}

static uint32_t find_one(struct bkd_string string, const uint32_t * codepoints, uint32_t count, uint32_t * index) {
    uint32_t pos = 0;
    uint32_t testpoint = 0;
    uint32_t charsize;
    uint32_t i;
    while (pos < string.length) {
        charsize = bkd_utf8_readlen(string.data + pos, &testpoint, string.length - pos);
        if (testpoint == '\\') {
            pos += charsize;
            read_escape(bkd_strsub(string, pos, -1), &charsize);
        } else {
            for (i = 0; i < count; i++) {
                if (testpoint == codepoints[i]) {
                    *index = pos;
                    return testpoint;
                }
            }
        }
        pos += charsize;
    }
    return 0;
}

static const uint32_t brackets[] = { '[', ']' };
static const uint32_t dataclose[] = { ')' };
static const uint32_t opener[] = { '[' };

/* Convenience function for adding nodes. */
static inline struct bkd_linenode * add_node(struct bkd_linenode ** nodes, uint32_t * capacity, uint32_t * count) {
    if (*count == *capacity) {
        *capacity = 2 * (*count) + 1;
        *nodes = BKD_REALLOC(*nodes, *capacity * sizeof(struct bkd_linenode));
        if (!*nodes) {
            BKD_ERROR(BKD_ERROR_OUT_OF_MEMORY);
            return NULL;
        }
    }
	struct bkd_linenode * child = *nodes + *count;
    *count = *count + 1;
    child->markup = BKD_NONE;
    child->nodeCount = 0;
    child->data = BKD_NULLSTR;
    child->tree.leaf = BKD_NULLSTR;
	return child;
}

static struct bkd_string parse_flags(struct bkd_string string, uint32_t * flags) {
    uint32_t index = 0;
    int done = 0;
    while (!done) {
        uint32_t codepoint = 0;
        uint32_t csize = bkd_utf8_readlen(string.data + index, &codepoint, string.length - index);
        switch (codepoint) {
            case 'B': *flags |= BKD_BOLD; break;
            case 'U': *flags |= BKD_UNDERLINE; break;
            case 'I': *flags |= BKD_ITALICS; break;
            case 'S': *flags |= BKD_STRIKETHROUGH; break;
            case 'M': *flags |= BKD_MATH; break;
            case 'C': *flags |= BKD_CODEINLINE; break;
            case 'L': *flags |= BKD_LINK; break;
            case 'P': *flags |= BKD_IMAGE; break;
            case ':':
                string = bkd_strsub(string, index + 1, -1);
                done = 1; break;
            default:
                string = bkd_strsub(string, index, -1);
                done = 1; break;
        }
        index += csize;
    }
    return string;
}

/* Puts a string of utf8 text into a linenode struct. */
static struct bkd_string bkd_parse_line_impl(struct bkd_linenode * l, struct bkd_string string, int root) {
    struct bkd_string current = string;
    uint32_t codepoint;
    struct bkd_linenode * child;
    uint32_t capacity = 3;
    struct bkd_linenode * nodes = BKD_MALLOC(sizeof(struct bkd_linenode) * capacity);
    uint32_t count = 0, index = 0;

    if (!nodes) {
        BKD_ERROR(BKD_ERROR_OUT_OF_MEMORY);
        return BKD_NULLSTR;
    }

    while (current.length) {
        index = 0;
        if (root)
            codepoint = find_one(current, opener, 1, &index);
        else
            codepoint = find_one(current, brackets, 2, &index);
        if (codepoint) {
            if (index > 0) {
                child = add_node(&nodes, &capacity, &count);
                child->tree.leaf = bkd_strescape_new(bkd_strsub(current, 0, index - 1));
            }
            current = bkd_strsub(current, index + 1, -1);
        }
        if (codepoint == '[') {
            child = add_node(&nodes, &capacity, &count);
            current = parse_flags(current, &child->markup);
            current = bkd_parse_line_impl(child, current, 0);
            if (child->nodeCount == 0 && child->tree.leaf.length == 0) {
                bkd_strfree(child->tree.leaf);
                child->tree.leaf = bkd_str_new(child->data);
            }
        } else if (codepoint == ']') {
            if (current.length && current.data[0] == '(') {
                if (find_one(current, dataclose, 1, &index)) {
                    l->data = bkd_strescape_new(bkd_strsub(current, 1, index - 1));
                    current = bkd_strsub(current, index + 1, -1);
                } else {
                    l->data = bkd_strescape_new(bkd_strsub(current, 1, -1));
                    current = BKD_NULLSTR;
                }
            }
            break;
        } else {
            child = add_node(&nodes, &capacity, &count);
            child->tree.leaf = bkd_strescape_new(current);
            current = BKD_NULLSTR;
        }
    }
    if (count == 0) {
        l->nodeCount = 0;
        l->tree.leaf = BKD_NULLSTR;
        BKD_FREE(nodes);
    } else if (count == 1 && nodes[0].markup == BKD_NONE && nodes[0].data.length == 0) {
        l->nodeCount = nodes[0].nodeCount;
        l->tree = nodes[0].tree;
        BKD_FREE(nodes);
    } else {
        l->nodeCount = count;
        l->tree.node = BKD_REALLOC(nodes, sizeof(struct bkd_linenode) * count);
    }
    return current;
}

/* Puts a string of utf8 text into a linenode struct. */
struct bkd_linenode * bkd_parse_line(struct bkd_linenode * l, struct bkd_string string) {
    l->markup = BKD_NONE;
    l->data = BKD_NULLSTR;
    l->nodeCount = 0;
    l->tree.leaf = BKD_NULLSTR;
    bkd_parse_line_impl(l, string, 1);
    return l;
}

enum ps {
    PS_SUBDOC,
    PS_COLLAPSIBLE_SUBDOC,
    PS_HEADER,
    PS_PARAGRAPH,
    PS_RULE,
    PS_CODEBLOCK,
    PS_LISTITEM,
    PS_LIST,
    PS_BLOCKCOMMENT
};

struct parse_frame {
    enum ps ps;
    uint32_t indent;
    struct bkd_node node;
    struct bkd_node * children;
    struct bkd_buffer buffer;
    uint32_t userflags;
    uint32_t useruint;
};

/* The parse state */
struct bkd_parsestate {
    struct bkd_istream * in;
    struct parse_frame * stack;
};

static void parse_pushstate(struct bkd_parsestate * state, uint32_t indent, enum ps ps) {
    struct parse_frame top;
    top.buffer = bkd_bufnew(80);
    top.children = NULL;
    top.indent = indent;
    top.ps = ps;
    top.node.type = BKD_LIST;
    top.node.data.list.itemCount = 0;
    top.node.data.list.items = NULL;
    top.node.data.list.style = BKD_LISTSTYLE_NONE;
    top.useruint = 0;
    top.userflags = 0;
    bkd_sbpush(state->stack, top);
}

static int parse_popstate(struct bkd_parsestate * state) {
    struct parse_frame * frame = bkd_sblastp(state->stack);
    struct bkd_node n = frame->node;
    switch (frame->ps) {
        case PS_LISTITEM:
            n.type = BKD_TEXT;
            bkd_parse_line(&n.data.text, frame->buffer.string);
            bkd_buffree(frame->buffer);
            break;
        case PS_BLOCKCOMMENT:
            n.type = BKD_COMMENTBLOCK;
            bkd_parse_line(&n.data.commentblock.text, frame->buffer.string);
            bkd_buffree(frame->buffer);
            break;
        case PS_CODEBLOCK:
            n.type = BKD_CODEBLOCK;
            n.data.codeblock.text = bkd_str_new(frame->buffer.string);
            n.data.codeblock.language = BKD_NULLSTR;
            bkd_buffree(frame->buffer);
            break;
        case PS_RULE:
            n.type = BKD_HORIZONTALRULE;
            bkd_buffree(frame->buffer);
            break;
        case PS_LIST:
        case PS_SUBDOC:
            n.type = BKD_LIST;
            n.data.list.itemCount = bkd_sbcount(frame->children);
            n.data.list.items = bkd_sbflatten(frame->children);
            bkd_buffree(frame->buffer);
            break;
        case PS_COLLAPSIBLE_SUBDOC:
            if (bkd_sbcount(frame->children) == 1) { /* If we only have one child, use that child instead */
                n = frame->children[0];
                bkd_sbfree(frame->children);
                if (n.type == BKD_PARAGRAPH)
                    n.type = BKD_TEXT;
            } else {
                n.type = BKD_LIST;
                n.data.list.itemCount = bkd_sbcount(frame->children);
                n.data.list.items = bkd_sbflatten(frame->children);
            }
            bkd_buffree(frame->buffer);
            break;
        case PS_PARAGRAPH:
            n.type = BKD_PARAGRAPH;
            bkd_parse_line(&n.data.paragraph.text, frame->buffer.string);
            bkd_buffree(frame->buffer);
            break;
        case PS_HEADER:
            bkd_buffree(frame->buffer);
            break;
    }
    if (bkd_sbcount(state->stack) > 1) {
        struct parse_frame * newtop = bkd_sblastp(state->stack) - 1;
        bkd_sbpush(newtop->children, n);
        bkd_sbpop(state->stack);
        return 1;
    } else { /* Otherwise, we are the root frame. */
        bkd_sblast(state->stack).node = n;
        return 0;
    }
}

static inline int is_list(struct bkd_string trimmed, uint32_t listtype) {
    uint8_t leaderChar;
    if (trimmed.length <= 2 || trimmed.data[1] != ' ') return 0;
    switch (listtype) {
        case BKD_LISTSTYLE_NUMBERED: leaderChar = '%'; break;
        case BKD_LISTSTYLE_BULLETS: leaderChar = '*'; break;
        default: leaderChar = '*'; break;
    }
    return trimmed.data[0] == leaderChar;
}

static inline uint32_t get_list_type(struct bkd_string trimmed) {
    if (is_list(trimmed, BKD_LISTSTYLE_NUMBERED)) return BKD_LISTSTYLE_NUMBERED;
    if (is_list(trimmed, BKD_LISTSTYLE_BULLETS)) return BKD_LISTSTYLE_BULLETS;
    return 0;
}

/* Dispatch a single line to the parser. Returns if the line was consumed. If so,
 * the dispatch will be next with the next line. If not, the dispath will be called
 * again with the same line (but hopefully different state) */
static int parse_dispatch(struct bkd_parsestate * state, struct bkd_string line) {
    uint32_t indent = bkd_strindent(line);
    struct parse_frame * frame = bkd_sblastp(state->stack);
    struct bkd_string trimmed;
    struct bkd_string stripped;
    int isEmpty = bkd_strempty(line);
    switch (frame->ps) {
        case PS_SUBDOC:
        case PS_COLLAPSIBLE_SUBDOC:
            if (isEmpty) return 1;
            if (indent < frame->indent) {
                parse_popstate(state);
                return 0;
            }
            if (indent > frame->indent) {
                parse_pushstate(state, indent, PS_SUBDOC);
                return 0;
            }
            /* Here is where we detect what kind of block comes next. */
            trimmed = bkd_strtrim_front(line);
            if (trimmed.data[0] == '#') {
                parse_pushstate(state, indent, PS_HEADER);
            } else if (bkd_strempty(bkd_strtrimc_front(trimmed, '-')) ||
                       bkd_strempty(bkd_strtrimc_front(trimmed, '=')) ||
                       bkd_strempty(bkd_strtrimc_front(trimmed, '`'))) {
                parse_pushstate(state, indent, PS_RULE);
            } else if (trimmed.length >= 2 && trimmed.data[0] == '>' && trimmed.data[1] == '>') {
                parse_pushstate(state, indent, PS_CODEBLOCK);
            } else if (trimmed.length >= 2 && trimmed.data[0] == '>') {
                parse_pushstate(state, indent, PS_BLOCKCOMMENT);
            } else {
                uint32_t listtype = get_list_type(trimmed);
                if (listtype) {
                    parse_pushstate(state, indent, PS_LIST);
                    bkd_sblast(state->stack).node.data.list.style = listtype;
                } else {
                    parse_pushstate(state, indent, PS_PARAGRAPH);
                }
            }
            return 0;
        case PS_LIST:
            if (isEmpty) return 1;
            if (indent > frame->indent) {
                parse_pushstate(state, indent, PS_SUBDOC);
                return 0;
            } else if (indent < frame->indent) {
                parse_popstate(state);
                return 0;
            } else if (is_list(bkd_strtrim_front(line), frame->node.data.list.style)) {
                indent += 1 + bkd_strindent(bkd_strsub(bkd_strtrim_front(line), 1, -1));
                parse_pushstate(state, indent, PS_COLLAPSIBLE_SUBDOC);
                parse_pushstate(state, indent, PS_LISTITEM);
                frame = bkd_sblastp(state->stack);
                frame->buffer = bkd_bufpush(frame->buffer, bkd_strsub(bkd_strtrim_front(line), 2, -1));
                return 1;
            } else {
                parse_popstate(state);
                return 0;
            }
        case PS_CODEBLOCK:
            stripped = bkd_strstripn_new(line, indent < frame->indent ? indent : frame->indent);
            if (frame->useruint == 0) { /* First line */
                trimmed = bkd_strtrimc_front(stripped, '>');
                frame->useruint = stripped.length - trimmed.length;
                trimmed = bkd_strtrim_both(trimmed);
                frame->node.data.codeblock.language = bkd_strescape_new(trimmed);
            } else if (stripped.length - bkd_strtrimc_front(stripped, '>').length == frame->useruint) { /* Last line */
                parse_popstate(state);
            } else {
                if (frame->userflags & 1) {
                    frame->buffer = bkd_bufpushc(frame->buffer, '\n');
                } else {
                    frame->userflags |= 1;
                }
                frame->buffer = bkd_bufpush(frame->buffer, stripped);
            }
            bkd_strfree(stripped);
            return 1;
        case PS_RULE:
            switch (line.data[0]) {
                case '-': frame->node.data.linebreak.style = BKD_SOLID; break;
                case '=': frame->node.data.linebreak.style = BKD_PAGEBREAK; break;
                default: frame->node.data.linebreak.style = BKD_DOTTED; break;
            }
            parse_popstate(state);
            return 1;
        case PS_HEADER:
            trimmed = bkd_strtrimc_front(line, '#');
            uint32_t headerSize = line.length - trimmed.length;
            frame->node.data.header.size = headerSize;
            frame->node.type = BKD_HEADER;
            bkd_parse_line(&frame->node.data.header.text, bkd_strtrim_both(trimmed));
            parse_popstate(state);
            return 1;
        case PS_PARAGRAPH:
        case PS_LISTITEM:
            if (isEmpty || indent < frame->indent) {
                parse_popstate(state);
                return isEmpty; /* Consume empty lines but not lines belonging to lower states. */
            } else if (indent > frame->indent) {
                parse_popstate(state);
                parse_pushstate(state, indent, PS_SUBDOC);
                return 0;
            }
            if (frame->userflags)
                frame->buffer = bkd_bufpushc(frame->buffer, ' ');
            stripped = bkd_strstripn_new(line, frame->indent);
            frame->buffer = bkd_bufpush(frame->buffer, stripped);
            bkd_strfree(stripped);
            frame->userflags |= 1;
            return 1;
        case PS_BLOCKCOMMENT:
            if (isEmpty || indent < frame->indent) {
                parse_popstate(state);
                return isEmpty; /* Consume empty lines but not lines belonging to lower states. */
            }
            trimmed = bkd_strtrim_front(line);
            if (trimmed.data[0] != '>') {
                parse_popstate(state);
                return 0;
            }
            trimmed = bkd_strtrim_front(bkd_strsub(trimmed, 1, -1));
            if (frame->userflags)
                frame->buffer = bkd_bufpushc(frame->buffer, '\n');
            frame->userflags |= 1;
            frame->buffer = bkd_bufpush(frame->buffer, trimmed);
            return 1;
    }
    return 1;
}

/* Dispatch to a given parse state based on the current line. */
static inline void parse_main(struct bkd_parsestate * state) {
    while (!state->in->done) {
        struct bkd_string line = bkd_getl(state->in);
        /* Repeatedly dispatch until consumed */
        while (!parse_dispatch(state, line))
            ;
    }
}

/* Parse a BKDoc input stream and create an AST. */
struct bkd_list * bkd_parse(struct bkd_istream * in) {

    struct bkd_parsestate state;
    state.in = in;
    state.stack = NULL;

    parse_pushstate(&state, 0, PS_SUBDOC);
    parse_main(&state);

    /* Resolve internal links and anchors */

    /* Set up document */
    while (parse_popstate(&state))
        ;

    struct bkd_list * document = BKD_MALLOC(sizeof(struct bkd_list));
    *document = state.stack[0].node.data.list;
    bkd_sbfree(state.stack);
    return document;
}

/* recursivley free line nodes */
static void cleanup_linenode(struct bkd_linenode * l) {
    if (l->nodeCount > 0) {
        for (unsigned i = 0; i < l->nodeCount; i++) {
            cleanup_linenode(l->tree.node + i);
        }
        BKD_FREE(l->tree.node);
    } else {
        bkd_strfree(l->tree.leaf);
    }
    bkd_strfree(l->data);
}

/* Recursively cleanup nodes */
static void cleanup_node(struct bkd_node * node) {
    uint32_t max, i;
    switch (node->type) {
        case BKD_PARAGRAPH:
            cleanup_linenode(&node->data.paragraph.text);
            break;
        case BKD_LIST:
            max = node->data.list.itemCount;
            for (i = 0; i < max; i++) {
                cleanup_node(node->data.list.items + i);
            }
            BKD_FREE(node->data.list.items);
            break;
        case BKD_TABLE:
            max = node->data.table.cols * node->data.table.rows;
            for (i = 0; i < max; i++) {
                cleanup_node(node->data.table.items + i);
            }
            BKD_FREE(node->data.table.items);
            break;
        case BKD_HEADER:
            cleanup_linenode(&node->data.header.text);
            break;
        case BKD_CODEBLOCK:
            BKD_FREE(node->data.codeblock.text.data);
            if (node->data.codeblock.language.length > 0)
                BKD_FREE(node->data.codeblock.language.data);
            break;
        case BKD_COMMENTBLOCK:
            cleanup_linenode(&node->data.commentblock.text);
            break;
        case BKD_DATASTRING:
            bkd_strfree(node->data.datastring);
            break;
        case BKD_TEXT:
            cleanup_linenode(&node->data.text);
            break;
        default:
            break;
    }
}

void bkd_docfree(struct bkd_list * document) {
    for (uint32_t i = 0; i < document->itemCount; i++) {
        cleanup_node(document->items + i);
    }
    BKD_FREE(document->items);
    BKD_FREE(document);
}
