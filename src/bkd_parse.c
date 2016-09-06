#include "bkd.h"
#include "bkd_utf8.h"

#include <string.h>

enum markupchar {
    MUC_NONE = 0,
    MUC_STAR,
    MUC_UNDERSCORE,
    MUC_TILDA,
    MUC_PLUS,
    MUC_DOLLAR,
    MUC_BACKTICK,
    MUC_BANGBRACKET,
    MUC_OPENBRACKET,
    MUC_CLOSEBRACKET,
    MUC_OPENPAREN,
    MUC_CLOSEPAREN,
};

#define MUCBIT_SYMETRIC 0x01
#define MUCBIT_RECURSIVE 0x02
#define MUCBIT_DELIMITER 0x04
#define MUCBIT_CLOSER 0x08
#define MUCBIT_OPENER 0x0F

static const enum markupchar tokendata[] = {
    /* None         */ 0,
    /* Star         */ MUCBIT_DELIMITER | MUCBIT_SYMETRIC | MUCBIT_RECURSIVE | MUCBIT_OPENER | MUCBIT_CLOSER,
    /* Underscore   */ MUCBIT_DELIMITER | MUCBIT_SYMETRIC | MUCBIT_RECURSIVE | MUCBIT_OPENER | MUCBIT_CLOSER,
    /* Tilda        */ MUCBIT_DELIMITER | MUCBIT_SYMETRIC | MUCBIT_RECURSIVE | MUCBIT_OPENER | MUCBIT_CLOSER,
    /* Plus         */ MUCBIT_DELIMITER | MUCBIT_SYMETRIC | MUCBIT_RECURSIVE | MUCBIT_OPENER | MUCBIT_CLOSER,
    /* Dollar       */ MUCBIT_DELIMITER | MUCBIT_SYMETRIC | MUCBIT_OPENER | MUCBIT_CLOSER,
    /* Backtick     */ MUCBIT_DELIMITER | MUCBIT_SYMETRIC | MUCBIT_OPENER | MUCBIT_CLOSER,
    /* Bangbracket  */ MUCBIT_DELIMITER | MUCBIT_OPENER,
    /* Openbracket  */ MUCBIT_DELIMITER | MUCBIT_OPENER,
    /* Closebracket */ MUCBIT_DELIMITER | MUCBIT_CLOSER,
    /* Openparen    */ MUCBIT_DELIMITER | MUCBIT_OPENER,
    /* Closeparen   */ MUCBIT_DELIMITER | MUCBIT_CLOSER
};

/**
 * Checks if a unicode codepoint is whitespace.
 */
static inline int isWhitespace(uint32_t codepoint) {
    /* There might be some other unicode to consider later, in the higher regions. */
    return (codepoint > 8 && codepoint < 14) || codepoint == 32;
}

/* Tokenize the input from a line into only the tokens we need. */
static enum markupchar charCheckMarkup(const struct bkd_string string, uint32_t * tokenLength) {
    if (string.length == 0) {
        *tokenLength = 0;
        return MUC_NONE;
    }
    uint32_t length = 1;
    enum markupchar ret = MUC_NONE;
    switch (string.data[0]) {
        case '*':
            ret = MUC_STAR;
            break;
        case '_':
            ret = MUC_UNDERSCORE;
            break;
        case '~':
            ret = MUC_TILDA;
            break;
        case '$':
            ret = MUC_DOLLAR;
            break;
        case '+':
            ret = MUC_PLUS;
            break;
        case '!':
            if (string.length > 1 && string.data[1] == '[') {
                length = 2;
                ret = MUC_BANGBRACKET;
            } else {
                ret = MUC_NONE;
            }
            break;
        case '(':
            ret = MUC_OPENPAREN;
            break;
        case ')':
            ret = MUC_CLOSEPAREN;
            break;
        case '[':
            ret = MUC_OPENBRACKET;
            break;
        case ']':
            ret = MUC_CLOSEBRACKET;
            break;
        case '`':
            ret = MUC_BACKTICK;
            break;
        case '\\':
            if (string.length == 1) {
                length = 1;
            } else {
                length =  bkd_utf8_sizeb(string.data[1]) + 1;
                if (length == 1)
                    length = 2;
                if (length > string.length)
                    length = string.length;
            }
            ret = MUC_NONE;
            break;
        default:
            length = bkd_utf8_sizeb(*string.data);
            if (length == 0)
                length = 1;
            if (length > string.length)
                length = string.length;
            break;
    }
    *tokenLength = length;
    return ret;
}

/* recursivley free line nodes */
static void cleanup_linenode(struct bkd_linenode * l) {
    if (l->nodeCount > 0) {
        for (unsigned i = 0; i < l->nodeCount; i++) {
            cleanup_linenode(l->tree.node + i);
        }
        BKD_FREE(l->tree.node);
    } else {
        BKD_FREE(l->tree.leaf.data);
    }
    if (l->data.length > 0 && l->data.data != NULL)
        BKD_FREE(l->data.data);
}

/* Recursively cleanup nodes */
static void cleanup_node(struct bkd_node * node) {
    uint32_t max;
    switch (node->type) {
        case BKD_PARAGRAPH:
            cleanup_linenode(&node->data.paragraph.text);
            break;
        case BKD_OLIST:
            max = node->data.olist.itemCount;
            for (uint32_t i = 0; i < max; i++) {
                cleanup_node(node->data.olist.items + i);
            }
            BKD_FREE(&node->data.olist.items);
            break;
        case BKD_ULIST:
            max = node->data.ulist.itemCount;
            for (uint32_t i = 0; i < max; i++) {
                cleanup_node(node->data.ulist.items + i);
            }
            BKD_FREE(&node->data.ulist.items);
            break;
        case BKD_TABLE:
            max = node->data.table.cols * node->data.table.rows;
            for (uint32_t i = 0; i < max; i++) {
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
        default:
            break;
    }
}

/* Returns the next opening delimiter token, and puts its location in nextPos.
 * If can't find an opening delimiter, returns MUC_NONE. */
static enum markupchar find_next(struct bkd_string string, uint32_t * nextPos, uint32_t * nextLength) {
    uint32_t pos = 0;
    uint32_t tokenLength = 0;
    enum markupchar current;
    while(pos < string.length) {
        current = charCheckMarkup(bkd_strsub(string, pos, -1), &tokenLength);
        if (tokendata[current] & MUCBIT_OPENER) {
            if (nextPos != NULL)
                *nextPos = pos;
            if (nextLength != NULL)
                *nextLength = tokenLength;
            return current;
        }
        if (tokenLength == 0) {
            break;
        }
        pos += tokenLength;
    }
    return MUC_NONE;
}

/* Tries to find the balancing delimiter for string utf8. Returns -1 if not found, else returns the
 * the size of the closing delimiter. */
static int32_t find_balanced(struct bkd_string string, struct bkd_string * out) {

    uint32_t pos = 0;
    uint32_t tokenLength = 0;
    enum markupchar open = charCheckMarkup(string, &pos);
    uint32_t firstLength = pos;
    if (!(tokendata[open] & MUCBIT_OPENER)) return -1;
    enum markupchar close = open;
    enum markupchar current = MUC_NONE;

    /* Get closing delimiter for non-symmetric delimiters */
    if (close == MUC_OPENPAREN)
        close = MUC_CLOSEPAREN;
    else if (close == MUC_OPENBRACKET)
        close = MUC_CLOSEBRACKET;
    else if (close == MUC_BANGBRACKET)
        close = MUC_CLOSEBRACKET;

    while (pos < string.length) {
        struct bkd_string currentString = bkd_strsub(string, pos, -1);
        current = charCheckMarkup(currentString, &tokenLength);
        if (current == close) {
            if (out != NULL) {
                *out = bkd_strsub(string, firstLength, -1 - tokenLength);
            }
            return tokenLength;
        }
        if ((tokendata[open] & MUCBIT_RECURSIVE) && (tokendata[current] & MUCBIT_OPENER)) {
            struct bkd_string temp;
            int32_t result = find_balanced(currentString, &temp);
            if (result == -1) { /* Could not balance inner opener */
                pos += tokenLength;
            } else {
                pos += tokenLength + temp.length + result;
            }
        } else {
            pos += tokenLength;
        }
    }

    return -1;
}

/* Convenience function for adding nodes. */
static inline struct bkd_linenode * add_node(struct bkd_linenode ** nodes, uint32_t * capacity, uint32_t * count) {
    if (*count == *capacity) {
        *capacity = 2 * (*count) + 1;
        *nodes = BKD_REALLOC(*nodes, *capacity * sizeof(struct bkd_linenode));
    }
	struct bkd_linenode * child = *nodes + *count;
    *count = *count + 1;
	child->data = BKD_NULLSTR;
	child->nodeCount = 0;
	child->tree.leaf = BKD_NULLSTR;
	child->markupType = BKD_NONE;
	return child;
}

/* Escapes special bkdoc characters in fragment of text. Returns a new piece of memory. */
/* TODO: Add unicode escapes and non printable escapes. */
static struct bkd_string escape(struct bkd_string string) {
    return bkd_strclone(string);
}

/* Puts a string of utf8 text into a linenode struct. */
struct bkd_linenode * bkd_parse_line(struct bkd_linenode * l, struct bkd_string string) {

    uint32_t nodeCount = 0;
    uint32_t nodeCapacity = 3;
    struct bkd_linenode * nodes = BKD_MALLOC(nodeCapacity * sizeof(struct bkd_linenode));
    struct bkd_linenode * child;

    enum markupchar m = MUC_NONE;
    struct bkd_string current;
    struct bkd_string innerText;
    uint32_t openerLength = 0;
    uint32_t next = 0;
    uint32_t last = 0;
    uint32_t pos = 0;

    /* Read all of string */
    while (1) {

        current = bkd_strsub(string, pos, -1);
        m = find_next(current, &next, &openerLength);

        /* We're done if no next special */
        if (m == MUC_NONE) {
            if (last < string.length) {
                child = add_node(&nodes, &nodeCapacity, &nodeCount);
                child->tree.leaf = escape(bkd_strsub(string, last, string.length));
            }
            break;
        }

        pos += next;
        int32_t closerPosAfter = -1;
        if (m != MUC_OPENPAREN) {
            closerPosAfter = find_balanced(current, &innerText);
        }
        if (closerPosAfter > 0) {
            if (last < pos) {
                child = add_node(&nodes, &nodeCapacity, &nodeCount);
                child->tree.leaf = escape(bkd_strsub(string, last, pos));
                last = pos;
            }
            child = add_node(&nodes, &nodeCapacity, &nodeCount);
            if (tokendata[m] & MUCBIT_RECURSIVE) {
                bkd_parse_line(child, innerText);
            } else {
                child->tree.leaf = escape(innerText);
            }
            /* Set the child's markup type */
            switch (m) {
                case MUC_STAR:
                    child->markupType = BKD_ITALICS;
                    break;
                case MUC_UNDERSCORE:
                    child->markupType = BKD_BOLD;
                    break;
                case MUC_TILDA:
                    child->markupType = BKD_STRIKETHROUGH;
                    break;
                case MUC_PLUS:
                    child->markupType = BKD_UNDERLINE;
                    break;
                case MUC_DOLLAR:
                    child->markupType = BKD_MATH;
                    break;
                case MUC_BACKTICK:
                    child->markupType = BKD_CODEINLINE;
                    break;
                case MUC_BANGBRACKET:
                    child->markupType = BKD_IMAGE;
                    break;
                case MUC_OPENBRACKET:
                    child->markupType = BKD_LINK;
                    break;
                default:
                    child->markupType = BKD_NONE;
                    /* Error? */
                    break;
            }
            pos += closerPosAfter;
            /* Test for image and link suffix */
            if ((child->markupType == BKD_IMAGE || child->markupType == BKD_LINK) &&
                string.data[pos] == '(' &&
                (closerPosAfter = find_balanced(current, NULL)) != -1) {
                child->data.data = BKD_MALLOC(closerPosAfter - 2);
                child->data.length = closerPosAfter - 2;
                memcpy(child->data.data, string.data + pos + 1, closerPosAfter - 2);
                pos += closerPosAfter;
            }
            last = pos;
        } else {
            pos += openerLength;
        }
    }
    nodes = BKD_REALLOC(nodes, sizeof(struct bkd_linenode) * nodeCount);
    if (!nodes) {
        l->data = BKD_NULLSTR;
        l->markupType = BKD_NONE;
        l->tree.node = NULL;
        l->nodeCount = 0;
    } else if (nodeCount < 2 && nodes[0].markupType == BKD_NONE) {
        *l = *nodes;
    } else {
        l->data = BKD_NULLSTR;
        l->markupType = BKD_NONE;
        l->tree.node = nodes;
        l->nodeCount = nodeCount;
    }
    return l;
}

/* Gets the indentation level of a line. */
static uint32_t get_indent(struct bkd_string line) {
    uint32_t indent = 0;
    uint32_t pos = 0;
    uint32_t codepoint = 0;
    while (pos < line.length) {
        if (line.data[pos] == ' ') {
            indent++;
            pos++;
        } else if (line.data[pos] == '\t') {
            indent += 4;
            pos++;
        } else {
            bkd_utf8_readlen(line.data + pos, &codepoint, line.length - pos);
            if (!isWhitespace(codepoint)) {
                break;
            }
        }
    }
    if (pos == line.length)
        return 0;
    return indent;
}

/* Strip off n virtual spaces in front of text, as well as trailing whitespace.
 * Mallocs a new string. (In order to accomadate the potential of padding). */
static struct bkd_string stripn(struct bkd_string string, uint32_t n) {
    uint32_t trailing = 0;
    uint32_t end = string.length - 1;
    uint32_t leading = 0;
    uint32_t pos = 0;
    uint32_t padding = 0;
    while (string.data[end] == ' ' || string.data[end] == '\t') {
        end--;
        trailing++;
    }
    while (leading < n) {
        if (string.data[pos] == ' ') {
            leading++;
        } else if (string.data[pos] == '\t') {
            leading += 4;
        } else {
            break;
        }
        pos++;
    }
    padding = leading - n;
    uint32_t newlen = string.length - trailing - pos + padding;
    uint8_t * newstring = BKD_MALLOC(newlen);
    for (uint32_t i = 0; i < padding; i++)
        newstring[i] = ' ';
    memcpy(newstring + padding, string.data + pos, newlen - padding);
    return (struct bkd_string) {
        newlen,
        newstring
    };
}

/* Checks if a line is empty */
static int is_empty(struct bkd_string string) {
    for (uint32_t i = 0; i < string.length; i++) {
        if (string.data[i] != ' ' && string.data[i] != '\t')
            return 0;
    }
    return 1;
}

/* The parse state */
struct bkd_parsestate {
    struct bkd_istream * in;
    struct bkd_document * document;
    uint32_t indent;

    /* Buffer for holding multiple lines */
    struct bkd_buffer buffer;

    /* Buffer for nodes */
    uint32_t nodesCapacity;
    uint32_t nodesLength;
    struct bkd_node * nodes;
};

/* Allow deep recursion. Forward declaration of the root level parse state. */
static int parse_main(struct bkd_parsestate * state);

/* Get the type of line to help dispacth. */
static uint32_t linetype(struct bkd_string line) {
    if (is_empty(line))
        return BKD_COUNT_TYPE;
    while ((line.length > 0) && (line.data[0] == ' ' || line.data[0] == '\t')) {
        line.data++;
        line.length--;
    }
    if (line.data[0] == '=') {
        return BKD_HEADER;
    }
    return BKD_PARAGRAPH;
}

/* Returns a pointer to a child node of a document. Use this only to construct
 * child node before calling again. */
static struct bkd_node * add_docnode(struct bkd_parsestate * state) {
    uint32_t oldlen = state->nodesLength;
    uint32_t newlen = oldlen + 1;
    if (newlen >= state->nodesCapacity) {
        state->nodesCapacity = 2 * newlen;
        state->nodes = BKD_REALLOC(state->nodes, sizeof(struct bkd_node) * state->nodesCapacity);
    }
    state->nodesLength = newlen;
    return state->nodes + oldlen;
}

/* Appends text to the buffer in state */
static int append_text(struct bkd_parsestate * state, struct bkd_string string) {
    state->buffer = bkd_bufpush(state->buffer, string);
    return 1;
}

/* Parsing state function for paragraphs */
static void parse_paragraph(struct bkd_parsestate * state) {
    struct bkd_string line;
    uint32_t indent;
    uint32_t startIndent = state->indent;
    uint8_t space[1] = {' '};
    struct bkd_string spaaace = {
        1,
        NULL
    };
    spaaace.data = space;
    line = bkd_lastl(state->in);
    while (1) {
        line = stripn(line, startIndent);
        bkd_putn(BKD_STDOUT, line);
        bkd_putc(BKD_STDOUT, '\n');
        append_text(state, line);
        BKD_FREE(line.data);
        /* Prepare for next */
        line = bkd_getl(state->in);
        if (state->in->done) {
            break;
        }
        if (is_empty(line)) {
            break;
        }
        indent = get_indent(line);
        if (indent < startIndent) {
            break;
        }
        append_text(state, spaaace);
    }
    struct bkd_node * node = add_docnode(state);
    node->type = BKD_PARAGRAPH;
    bkd_parse_line(&node->data.paragraph.text, state->buffer.string);
}

/* parsing state function for headers */
static void parse_header(struct bkd_parsestate * state) {
    struct bkd_string line = bkd_lastl(state->in);
    line = stripn(line, state->indent);
    struct bkd_string hline = line;
    uint32_t hnum = 0;
    while (hline.data[0] == '=') {
        hline.data++;
        hline.length--;
        hnum++;
    }
    while (hline.data[0] == ' ' || hline.data[0] == '\t') {
        hline.data++;
        hline.length--;
        if (hline.length <= 0) {
            break;
        }
    }
    struct bkd_node * node = add_docnode(state);
    node->type = BKD_HEADER;
    node->data.header.size = hnum;
    bkd_parse_line(&node->data.header.text, hline);
    BKD_FREE(line.data);
    bkd_getl(state->in);
}

/* Dispatch to a given parse state based on the last line. */
static int parse_main(struct bkd_parsestate * state) {
    state->buffer.string.length = 0;
    uint32_t startIndent = state->indent;
    struct bkd_string line = bkd_lastl(state->in);
    if (state->in->done)
        return 0;
    state->indent = get_indent(line);
    uint32_t type = linetype(line);
    switch (type) {
        case BKD_COUNT_TYPE: /* Empty */
            bkd_getl(state->in);
            break;
        case BKD_HEADER:
            parse_header(state);
            break;
        case BKD_PARAGRAPH:
        default:
            parse_paragraph(state);
            break;
    }
    state->buffer.string.length = 0;
    state->indent = startIndent;
    return 1;
}

/* Parse a BKDoc input stream and create an AST. */
struct bkd_document * bkd_parse(struct bkd_istream * in) {

    struct bkd_parsestate state;
    state.in = in;
    state.indent = 0;
    state.buffer = bkd_bufnew(80);
    state.nodesCapacity = 2;
    state.nodesLength = 0;
    state.nodes = BKD_MALLOC(state.nodesCapacity * sizeof(struct bkd_node));
    state.document = BKD_MALLOC(sizeof(struct bkd_document));

    /* Prime the input */
    bkd_getl(state.in);
    while (parse_main(&state))
        ;

    /* Resolve internal links and anchors */

    /* Set up document */
    state.document->items = BKD_REALLOC(state.nodes, state.nodesLength * sizeof(struct bkd_node));
    state.document->itemCount = state.nodesLength;
    bkd_buffree(state.buffer);
    return state.document;
}

void bkd_parse_cleanup(struct bkd_document * document) {
    for (uint32_t i = 0; i < document->itemCount; i++) {
        cleanup_node(document->items + i);
    }
}
