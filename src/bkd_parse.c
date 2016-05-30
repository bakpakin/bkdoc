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

/* Tokenize the input from a line into only the tokens we need. */
static enum markupchar charCheckMarkup(uint8_t * c, uint32_t maxlen, uint32_t * tokenLength) {
    if (maxlen == 0) {
        *tokenLength = 0;
        return MUC_NONE;
    }
    uint32_t length = 1;
    enum markupchar ret = MUC_NONE;
    switch (*c) {
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
            if (maxlen > 1 && c[1] == '[') {
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
            if (maxlen == 1) {
                length = 1;
            } else {
                length =  bkd_utf8_sizeb(c[1]) + 1;
                if (length == 1)
                    length = 2;
                if (length > maxlen)
                    length = maxlen;
            }
            ret = MUC_NONE;
            break;
        default:
            length = bkd_utf8_sizeb(*c);
            if (length == 0)
                length = 1;
            if (length > maxlen)
                length = maxlen;
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
        BKD_FREE(l->tree.leaf.text);
    }
    if (l->dataSize > 0 && l->data != NULL)
        BKD_FREE(l->data);
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
            BKD_FREE(node->data.codeblock.text);
            if (node->data.codeblock.languageLength > 0)
                BKD_FREE(node->data.codeblock.language);
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
static enum markupchar find_next(uint8_t * utf8, uint32_t len, uint32_t * nextPos, uint32_t * nextLength) {
    uint32_t pos = 0;
    uint32_t tokenLength = 0;
    enum markupchar current;
    while(pos < len) {
        current = charCheckMarkup(utf8 + pos, len - pos, &tokenLength);
        if (tokendata[current] & MUCBIT_OPENER) {
            if (nextPos != NULL)
                *nextPos = pos;
            if (nextLength != NULL)
                *nextLength = tokenLength;
            return current;
        }
        pos += tokenLength;
    }
    return MUC_NONE;
}

/* Tries to find the balancing delimiter for string utf8. Returns -1 if not found, else returns the
 * location following the closing delimiter. */
static int32_t find_balanced(uint8_t * utf8, uint32_t len, uint32_t * firstTokenLength, uint32_t * finalTokenLength) {

    uint32_t pos = 0;
    uint32_t tokenLength = 0;
    enum markupchar open = charCheckMarkup(utf8, len, &pos);
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

    while (pos < len) {
        current = charCheckMarkup(utf8 + pos, len - pos, &tokenLength);
        if (current == close) {
            if (firstTokenLength != NULL)
                *firstTokenLength = firstLength;
            if (finalTokenLength != NULL)
                *finalTokenLength = tokenLength;
            return pos + tokenLength;
        }
        if ((tokendata[open] & MUCBIT_RECURSIVE) && (tokendata[current] & MUCBIT_OPENER)) {
            int32_t result = find_balanced(utf8 + pos, len - pos, NULL, NULL);
            if (result == -1) { /* Could not balance inner opener */
                pos += tokenLength;
            } else {
                pos += result;
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
        *capacity = 2 * (*count);
        *nodes = BKD_REALLOC(*nodes, *capacity * sizeof(struct bkd_linenode));
    }
	struct bkd_linenode * child = *nodes + ((*count)++);
	child->data = NULL;
	child->dataSize = 0;
	child->nodeCount = 0;
	child->tree.leaf.text = NULL;
	child->markupType = BKD_NONE;
	return child;
}

/* Escapes special bkdoc characters in fragment of text. Returns a new piece of memory, and stores the memory
 * length in outLen. */
static uint8_t * escape(uint8_t * utf8, uint32_t len, uint32_t * outLen) {
    /* For now, no escapes, add escapes later */
    uint8_t * ret = BKD_MALLOC(len);
    *outLen = len;
    memcpy(ret, utf8, len);
    return ret;
}

/* Puts a string of utf8 text into a linenode struct. */
struct bkd_linenode * bkd_parse_line(struct bkd_linenode * l,
        uint8_t * utf8,
        uint32_t len) {

    uint32_t nodeCount = 0;
    uint32_t nodeCapacity = 3;
    struct bkd_linenode * nodes = BKD_MALLOC(nodeCapacity * sizeof(struct bkd_linenode));
    struct bkd_linenode * child;

    enum markupchar m = MUC_NONE;
    uint32_t openerLength = 0;
    uint32_t closerLength = 0;
    uint32_t next = 0;
    uint32_t last = 0;
    uint32_t pos = 0;

    /* Read all of string */
    while (1) {

        m = find_next(utf8 + pos, len - pos, &next, &openerLength);

        /* We're done if no next special */
        if (m == MUC_NONE) {
            if (last < len) {
                child = add_node(&nodes, &nodeCapacity, &nodeCount);
                child->tree.leaf.text = escape(utf8 + last, len - last, &child->tree.leaf.textLength);
            }
            break;
        }

        pos += next;
        int32_t closerPosAfter = -1;
        if (m != MUC_OPENPAREN)
            closerPosAfter = find_balanced(utf8 + pos, len - pos, &openerLength, &closerLength);
        if (closerPosAfter > 0) {
            if (last < pos) {
                child = add_node(&nodes, &nodeCapacity, &nodeCount);
                child->tree.leaf.text = escape(utf8 + last, pos - last, &child->tree.leaf.textLength);
                last = pos;
            }
            child = add_node(&nodes, &nodeCapacity, &nodeCount);
            uint8_t * current = utf8 + pos + openerLength;
            uint32_t currentLength = closerPosAfter - openerLength - closerLength;
            if (tokendata[m] & MUCBIT_RECURSIVE) {
                bkd_parse_line(child, current, currentLength);
            } else {
                child->tree.leaf.text = escape(current, currentLength, &child->tree.leaf.textLength);
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
                utf8[pos] == '(' &&
                (closerPosAfter = find_balanced(utf8 + pos, len - pos, NULL, NULL)) != -1) {

                child->data = BKD_MALLOC(closerPosAfter - 2);
                child->dataSize = closerPosAfter - 2;
                memcpy(child->data, utf8 + pos + 1, closerPosAfter - 2);
                pos += closerPosAfter;
            }
            last = pos;
        } else {
            pos += openerLength;
        }
    }

    nodes = BKD_REALLOC(nodes, sizeof(struct bkd_linenode) * nodeCount);
    if (nodeCount < 2 && nodes[0].markupType == BKD_NONE) {
        *l = *nodes;
        BKD_FREE(nodes);
    } else {
        l->data = NULL;
        l->dataSize = 0;
        l->markupType = BKD_NONE;
        l->tree.node = nodes;
        l->nodeCount = nodeCount;
    }
    return l;
}

/* Gets the indentation level of a line. */
static uint32_t get_indent(const uint8_t * utf8, uint32_t len) {
    uint32_t indent = 0;
    uint32_t pos = 0;
    while (pos < len) {
        if (utf8[pos] == ' ') {
            indent++;
            pos++;
        } else if (utf8[pos] == '\t') {
            indent += 4;
            pos++;
        } else {
            break;
        }
    }
    if (pos == len)
        return 0;
    return indent;
}

/* Strip off n virtual spaces in front of text, as well as trailing whitespace.*/
static uint8_t * stripn(uint8_t * utf8, uint32_t len, uint32_t n, uint32_t * lenOut) {
    uint32_t trailing = 0;
    uint32_t end = len - 1;
    uint32_t leading = 0;
    uint32_t pos = 0;
    uint32_t padding = 0;
    while (utf8[end] == ' ' || utf8[end] == '\t') {
        end--;
        trailing++;
    }
    while (leading < n) {
        if (utf8[pos] == ' ') {
            leading++;
        } else if (utf8[pos] == '\t') {
            leading += 4;
        } else {
            break;
        }
        pos++;
    }
    padding = leading - n;
    uint32_t newlen = len - trailing - pos + padding;
    uint8_t * newstring = BKD_MALLOC(newlen);
    for (uint32_t i = 0; i < padding; i++)
        newstring[i] = ' ';
    memcpy(newstring + padding, utf8 + pos, newlen - padding);
    *lenOut = newlen;
    return newstring;
}

/* Checks if a line is empty */
static int is_empty(uint8_t * utf8, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        if (utf8[i] != ' ' && utf8[i] != '\t')
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
    uint32_t bufferCapacity;
    uint32_t bufferLength;
    uint8_t * buffer;

    /* Buffer for nodes */
    uint32_t nodesCapacity;
    uint32_t nodesLength;
    struct bkd_node * nodes;
};

/* Get the type of line to help dispacth. */
static uint32_t linetype(uint8_t * utf8, uint32_t len) {
    if (is_empty(utf8, len))
        return BKD_COUNT_TYPE;
    if (utf8[0] == '=')
        return BKD_HEADER;
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
    if (state->nodes == NULL) {
        printf("FUCKKCKJSBCKSJ!\n");
    }
    state->nodesLength = newlen;
    return state->nodes + oldlen;
}

/* Appends text to the buffer in state */
static int append_text(struct bkd_parsestate * state, uint8_t * utf8, uint32_t len) {
    if (state->bufferLength + len > state->bufferCapacity) {
        state->bufferCapacity = 2 * (state->bufferLength + len);
        state->buffer = BKD_REALLOC(state->buffer, state->bufferCapacity);
    }
    memcpy(state->buffer + state->bufferLength, utf8, len);
    state->bufferLength += len;
    return 1;
}

/* Parsing state function for paragraphs */
static void parse_paragraph(struct bkd_parsestate * state) {
    uint8_t * utf8;
    uint32_t len;
    uint32_t indent;
    uint32_t startIndent = state->indent;
    uint8_t space[1] = {' '};
    utf8 = bkd_lastl(state->in, &len);
    while (1) {
        utf8 = stripn(utf8, len, startIndent, &len);
        append_text(state, utf8, len);
        BKD_FREE(utf8);
        /* Prepare for next */
        utf8 = bkd_getl(state->in, &len);
        if (utf8 == NULL)
            break;
        if (is_empty(utf8, len))
            break;
        indent = get_indent(utf8, len);
        if (indent < startIndent)
            break;
        append_text(state, space, 1);
    }
    struct bkd_node * node = add_docnode(state);
    node->type = BKD_PARAGRAPH;
    bkd_parse_line(&node->data.paragraph.text, state->buffer, state->bufferLength);
}

/* parsing state function for headers */
static void parse_header(struct bkd_parsestate * state) {
    uint8_t * utf8;
    uint32_t len;
    utf8 = bkd_lastl(state->in, &len);
    utf8 = stripn(utf8, len, state->indent, &len);
    uint8_t * htext = utf8;
    uint32_t hlen = len;
    uint32_t hnum = 0;
    while (htext[0] == '=') {
        htext++;
        hlen--;
        hnum++;
    }
    while (htext[0] == ' ' || htext[0] == '\t') {
        htext++;
        hlen--;
    }
    struct bkd_node * node = add_docnode(state);
    node->type = BKD_HEADER;
    node->data.header.size = hnum;
    bkd_parse_line(&node->data.header.text, htext, hlen);
    BKD_FREE(utf8);
    bkd_getl(state->in, NULL);
}

/* Dispatch to a given parse state based on the last line. */
static int parse_main(struct bkd_parsestate * state) {
    state->bufferLength = 0;
    uint32_t startIndent = state->indent;
    uint32_t len = 0;
    uint8_t * utf8 = bkd_lastl(state->in, &len);
    if (utf8 == NULL)
        return 0;
    state->indent = get_indent(utf8, len);
    uint32_t type = linetype(utf8, len);
    switch (type) {
        case BKD_COUNT_TYPE:
            bkd_getl(state->in, NULL);
            break;
        case BKD_HEADER:
            parse_header(state);
            break;
        case BKD_PARAGRAPH:
            parse_paragraph(state);
            break;
        default:
            break;
    }
    state->bufferLength = 0;
    state->indent = startIndent;
    return 1;
}

/* Parse a BKDoc input stream and create an AST. */
struct bkd_document * bkd_parse(struct bkd_istream * in) {

    struct bkd_parsestate state;
    state.in = in;
    state.indent = 0;
    state.bufferLength = 0;
    state.bufferCapacity = 80;
    state.buffer = BKD_MALLOC(state.bufferCapacity);
    state.nodesCapacity = 2;
    state.nodesLength = 0;
    state.nodes = BKD_MALLOC(state.nodesCapacity * sizeof(struct bkd_node));
    state.document = BKD_MALLOC(sizeof(struct bkd_document));
    /* Prime the input */
    bkd_getl(state.in, NULL);
    while (parse_main(&state))
        ;

    /* Resolve internal links and anchors */

    /* Set up document */
    state.document->items = BKD_REALLOC(state.nodes, state.nodesLength * sizeof(struct bkd_node));
    state.document->itemCount = state.nodesLength;
    BKD_FREE(state.buffer);
    return state.document;
}

void bkd_parse_cleanup(struct bkd_document * document) {
    for (uint32_t i = 0; i < document->itemCount; i++) {
        cleanup_node(document->items + i);
    }
}
