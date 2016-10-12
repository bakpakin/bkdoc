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

struct bkd_string bkd_strescape_new(struct bkd_string string) {
    struct bkd_string ret;
    uint32_t inNext = 0;
    uint32_t retNext = 0;
    uint32_t codepoint, accumulator;
    ret.data = BKD_MALLOC(string.length);
    while (inNext < string.length) {
        inNext += bkd_utf8_readlen(string.data + inNext, &codepoint, string.length - inNext);
        if (codepoint == '\\') {
            if (inNext >= string.length) { /* Terminating escapes signify a new line. */
                ret.data[retNext++] = '\n';
                break;
            }
            inNext += bkd_utf8_readlen(string.data + inNext, &codepoint, string.length - inNext);
            switch (codepoint) {
                case 'n':
                    ret.data[retNext++] = '\n';
                    break;
                case 't':
                    ret.data[retNext++] = '\t';
                    break;
                case '(': /* Unicode character */
                    accumulator = 0;
                    while (inNext < string.length) {
                        inNext += bkd_utf8_readlen(string.data + inNext, &codepoint, string.length - inNext);
                        if (codepoint == ')') break;
                        accumulator = 16 * accumulator + readHex(codepoint);
                    }
                    retNext += bkd_utf8_write(ret.data + retNext, accumulator);
                    break;
                default:
                    retNext += bkd_utf8_write(ret.data + retNext, codepoint);
                    break;
            }
        } else {
            retNext += bkd_utf8_write(ret.data + retNext, codepoint);
        }
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
    int foundPipe;
    uint32_t pipeIndex = 0;
    while (pos < string.length) {
        charsize = bkd_utf8_readlen(string.data + pos, &testpoint, string.length - pos);
        if (testpoint == '\\') {
            foundPipe = bkd_strfind(bkd_strsub(string, pos + 1, -1), '\\', &pipeIndex);
            if (foundPipe) {
                pos += pipeIndex + 2;
            } else {
                pos++;
            }
        } else {
            for (i = 0; i < count; i++) {
                if (testpoint == codepoints[i]) {
                    *index = pos;
                    return testpoint;
                }
            }
            pos += charsize;
        }
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
            case 'B': case '*': *flags |= BKD_BOLD; break;
            case 'U': case '_': *flags |= BKD_UNDERLINE; break;
            case 'I': case '/': *flags |= BKD_ITALICS; break;
            case 'S': case '-': *flags |= BKD_STRIKETHROUGH; break;
            case 'M': case '$': *flags |= BKD_MATH; break;
            case 'C': case '#': *flags |= BKD_CODEINLINE; break;
            case 'L': case '!': *flags |= BKD_LINK; break;
            case 'P': case '@': *flags |= BKD_IMAGE; break;
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
    uint32_t count = 0;
    while (current.length) {
        uint32_t index = 0;
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
    PS_DOCUMENT,
    PS_HEADER,
    PS_PARAGRAPH,
    PS_RULE,
    PS_CODEBLOCK
};

struct parse_frame {
    enum ps ps;
    int32_t indent;
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
    top.node.type = BKD_DOCUMENT;
    top.node.data.subdoc.itemCount = 0;
    top.node.data.subdoc.items = NULL;
    top.useruint = 0;
    top.userflags = 0;
    bkd_sbpush(state->stack, top);
}

static void parse_popstate(struct bkd_parsestate * state) {
    struct parse_frame * frame = bkd_sblastp(state->stack);
    struct bkd_node n = frame->node;
    switch (frame->ps) {
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
        case PS_DOCUMENT:
            n.type = BKD_DOCUMENT;
            n.data.subdoc.itemCount = bkd_sbcount(frame->children);
            n.data.subdoc.items = bkd_sbflatten(frame->children);
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
    } else { /* Otherwise, we are the root frame. */
        BKD_ERROR(BKD_ERROR_UNKNOWN);
    }
    bkd_sbpop(state->stack);
}

/* Dispatch a single line to the parser. Returns if the line was consumed. If so,
 * the dispatch will be next with the next line. If not, the dispath will be called
 * again with the same line (but hopefully different state) */
static int parse_dispatch(struct bkd_parsestate * state, struct bkd_string line) {
    int32_t indent = bkd_strindent(line);
    struct parse_frame * frame = bkd_sblastp(state->stack);
    struct bkd_string trimmed;
    struct bkd_string stripped;
    int isEmpty = bkd_strempty(line);
    switch (frame->ps) {
        case PS_DOCUMENT:
            if (isEmpty) return 1;
            if (indent < frame->indent) {
                parse_popstate(state);
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
            } else {
                parse_pushstate(state, indent, PS_PARAGRAPH);
            }
            return 0;
        case PS_CODEBLOCK:
            stripped = bkd_strstripn_new(line, indent < frame->indent ? indent : frame->indent);
            if (frame->useruint == 0) { /* First line */
                frame->useruint = stripped.length - bkd_strtrimc_front(stripped, '>').length;
                /* TODO: get language */
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
            if (isEmpty || indent < frame->indent) {
                parse_popstate(state);
                return isEmpty; /* Consume empty lines but not lines belonging to lower states. */
            }
            if (frame->buffer.string.length > 0)
                frame->buffer = bkd_bufpushc(frame->buffer, ' ');
            stripped = bkd_strstripn_new(line, frame->indent);
            frame->buffer = bkd_bufpush(frame->buffer, stripped);
            bkd_strfree(stripped);
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
struct bkd_document * bkd_parse(struct bkd_istream * in) {

    struct bkd_parsestate state;
    state.in = in;
    state.stack = NULL;

    /* Set indent of the root state to -1 so that nothing is ever adjacent */
    parse_pushstate(&state, -1, PS_DOCUMENT);
    parse_main(&state);

    /* Resolve internal links and anchors */

    /* Set up document */
    while (bkd_sbcount(state.stack) > 1)
        parse_popstate(&state);
    struct bkd_document * document = BKD_MALLOC(sizeof(struct bkd_document));
    document->itemCount = bkd_sbcount(state.stack[0].children);
    document->items = bkd_sbflatten(state.stack[0].children);
    bkd_buffree(state.stack[0].buffer);
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
        case BKD_OLIST:
            max = node->data.olist.itemCount;
            for (i = 0; i < max; i++) {
                cleanup_node(node->data.olist.items + i);
            }
            BKD_FREE(&node->data.olist.items);
            break;
        case BKD_ULIST:
            max = node->data.ulist.itemCount;
            for (i = 0; i < max; i++) {
                cleanup_node(node->data.ulist.items + i);
            }
            BKD_FREE(&node->data.ulist.items);
            break;
        case BKD_TABLE:
            max = node->data.table.cols * node->data.table.rows;
            for (i = 0; i < max; i++) {
                cleanup_node(node->data.table.items + i);
            }
            BKD_FREE(&node->data.table.items);
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

void bkd_docfree(struct bkd_document * document) {
    for (uint32_t i = 0; i < document->itemCount; i++) {
        cleanup_node(document->items + i);
    }
    BKD_FREE(document->items);
    BKD_FREE(document);
}
