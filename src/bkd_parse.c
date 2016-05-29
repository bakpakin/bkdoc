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

/* Adds a child node to a linenode and returns it. */
static struct bkd_linenode * add_child(struct bkd_linenode * parent, uint32_t * childCapacity) {
    if (*childCapacity <= parent->nodeCount) {
        *childCapacity = 2 * *childCapacity + 1;
        if (parent->tree.node == NULL) {
            parent->tree.node = BKD_MALLOC(*childCapacity);
        } else {
            parent->tree.node = BKD_REALLOC(parent->tree.node, *childCapacity);
        }
    }
    if (parent->tree.node == NULL) {
        BKD_ERROR(BKD_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    struct bkd_linenode * child = parent->tree.node + (parent->nodeCount++);
    child->nodeCount = 0;
    child->markupType = BKD_NONE;
    child->dataSize = 0;
    child->data = NULL;
    child->tree.leaf.text = NULL;
    child->tree.leaf.textLength = 0;
    return child;
}

/* recursivley free line nodes */
static struct bkd_linenode * cleanup(struct bkd_linenode * l) {
    if (l->nodeCount > 0) {
        for (unsigned i = 0; i < l->nodeCount; i++) {
            cleanup(l->tree.node + i);
        }
    } else {
        BKD_FREE(l->tree.leaf.text);
    }
    return NULL;
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

/* Gets the indentation level of a line. */
static uint32_t get_indent(uint8_t * utf8, uint32_t len) {
    uint32_t indent = 0;
    uint32_t pos = 0;
    while (pos < len) {
        if (utf8[pos] == ' ' || utf8[pos] == '>') {
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

struct bkd_document * bkd_parse(struct bkd_istream * in) {
    return NULL;
}
