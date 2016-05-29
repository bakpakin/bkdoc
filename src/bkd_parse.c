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
    0,
    MUCBIT_DELIMITER | MUCBIT_SYMETRIC | MUCBIT_RECURSIVE | MUCBIT_OPENER | MUCBIT_CLOSER,
    MUCBIT_DELIMITER | MUCBIT_SYMETRIC | MUCBIT_RECURSIVE | MUCBIT_OPENER | MUCBIT_CLOSER,
    MUCBIT_DELIMITER | MUCBIT_SYMETRIC | MUCBIT_RECURSIVE | MUCBIT_OPENER | MUCBIT_CLOSER,
    MUCBIT_DELIMITER | MUCBIT_SYMETRIC | MUCBIT_RECURSIVE | MUCBIT_OPENER | MUCBIT_CLOSER,
    MUCBIT_DELIMITER | MUCBIT_SYMETRIC | MUCBIT_OPENER | MUCBIT_CLOSER,
    MUCBIT_DELIMITER | MUCBIT_SYMETRIC | MUCBIT_OPENER | MUCBIT_CLOSER,
    MUCBIT_DELIMITER | MUCBIT_OPENER,
    MUCBIT_DELIMITER | MUCBIT_OPENER,
    MUCBIT_DELIMITER | MUCBIT_CLOSER,
    MUCBIT_DELIMITER | MUCBIT_OPENER,
    MUCBIT_DELIMITER | MUCBIT_CLOSER
};

/* Tokenize the input from a line into only the tokens we need. */
static enum markupchar charCheckMarkup(uint8_t * c, uint32_t maxlen, uint32_t * tokenLength) {
    *tokenLength = 1;
    switch (*c) {
        case '*':
            return MUC_STAR;
        case '_':
            return MUC_UNDERSCORE;
        case '~':
            return MUC_TILDA;
        case '$':
            return MUC_DOLLAR;
        case '+':
            return MUC_PLUS;
        case '!':
            if (maxlen > 1 && c[1] == '[') {
                *tokenLength = 2;
                return MUC_BANGBRACKET;
            } else {
                return MUC_NONE;
            }
        case '(':
            return MUC_OPENPAREN;
        case ')':
            return MUC_CLOSEPAREN;
        case '[':
            return MUC_OPENBRACKET;
        case ']':
            return MUC_CLOSEBRACKET;
        case '`':
            return MUC_BACKTICK;
        case '\\':
            if (maxlen < 2) {
                *tokenLength = 1;
            } else {
                *tokenLength =  bkd_utf8_sizeb(c[1]) + 1;
                if (*tokenLength > maxlen)
                    *tokenLength = maxlen;
            }
            return MUC_NONE;
        default:
            *tokenLength = bkd_utf8_sizeb(*c);
            if (*tokenLength > maxlen)
                *tokenLength = maxlen;
            return MUC_NONE;
    }
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
static enum markupchar find_next(uint8_t * utf8, uint32_t len, uint32_t * nextPos) {
    uint32_t pos = 0;
    uint32_t tokenLength = 0;
    enum markupchar current;
    while(pos < len) {
        current = charCheckMarkup(utf8 + pos, len - pos, &tokenLength);
        if (tokendata[current] & MUCBIT_OPENER) {
            *nextPos = pos;
            return current;
        }
        pos += tokenLength;
    }
    return MUC_NONE;
}

/* Tries to find the balancing delimiter for string utf8. Returns -1 if not found, else returns the
 * location following the closing delimiter. */
static int32_t find_balanced(uint8_t * utf8, uint32_t len) {
    uint32_t pos = 0;
    uint32_t tokenLength;
    enum markupchar open = charCheckMarkup(utf8, len, &pos);
    if (!(tokendata[open] & MUCBIT_OPENER)) return -1;
    enum markupchar close = open;
    enum markupchar current = MUC_NONE;
    if (close == MUC_OPENPAREN)
        close = MUC_CLOSEPAREN;
    else if (close == MUC_OPENBRACKET)
        close = MUC_CLOSEBRACKET;
    else if (close == MUC_BANGBRACKET)
        close = MUC_CLOSEBRACKET;
    while (pos < len) {
        current = charCheckMarkup(utf8 + pos, len - pos, &tokenLength);
        if (current == close)
            return pos + tokenLength;
        if ((tokendata[open] & MUCBIT_RECURSIVE) && (tokendata[current] & MUCBIT_OPENER)) {
            int32_t result = find_balanced(utf8 + pos, len - pos);
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
    };
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
    return *nodes + ((*count)++);
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
static struct bkd_linenode * parse_line(struct bkd_linenode * l,
        uint8_t * utf8,
        uint32_t len) {

    uint32_t nodeCount = 0;
    uint32_t nodeCapacity = 3;
    struct bkd_linenode * nodes = BKD_MALLOC(nodeCapacity * sizeof(struct bkd_linenode));
    struct bkd_linenode * child;

    enum markupchar m = MUC_NONE;
    uint32_t tokenLength = 0;
    uint32_t next = 0;
    uint32_t pos = 0;

    /* Read all of string */
    while (pos < len) {

        next = len - pos;
        m = find_next(utf8 + pos, len - pos, &next);

        /* Add plain text node up to next special token */
        if (next > 0) {
            child = add_node(&nodes, &nodeCapacity, &nodeCount);
            child->data = NULL;
            child->dataSize = 0;
            child->nodeCount = 0;
            child->markupType = BKD_NONE;
            child->tree.leaf.text = escape(utf8 + pos, next, &child->tree.leaf.textLength);
        }

        /* Break if there is no next special token */
        if (m == BKD_NONE)
            break;

        uint8_t * current = utf8 + pos + next;
        int32_t closerPosAfter = find_balanced(current, len - next - pos);
        if (closerPosAfter == -1) {
            pos += tokenLength;
        } else {
            uint32_t currentLength = closerPosAfter - 1;
            child = add_node(&nodes, &nodeCapacity, &nodeCount);
            charCheckMarkup(utf8 + next + pos, len - next - pos, &tokenLength);
            if (tokendata[m] & MUCBIT_RECURSIVE) {
                parse_line(child, utf8 + next + pos + tokenLength, closerPosAfter - tokenLength - 1);
            } else {
                child->data = NULL;
                child->dataSize = 0;
                child->nodeCount = 0;
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
                    /* Error? */
                    break;
            }

            pos += next + closerPosAfter;
        }
    }

    nodes = BKD_REALLOC(nodes, sizeof(struct bkd_linenode) * nodeCount);
    if (nodeCount > 1) {
        l->data = NULL;
        l->dataSize = 0;
        l->markupType = BKD_NONE;
        l->tree.node = nodes;
        l->nodeCount = nodeCount;
    } else {
        *l = *nodes;
        BKD_FREE(nodes);
    }
    return l;
}


struct bkd_linenode * bkd_parse_line(uint8_t * utf8, uint32_t len) {
    struct bkd_linenode * ret = BKD_MALLOC(sizeof(struct bkd_linenode));
    parse_line(ret, utf8, len);
    return ret;
}

struct bkd_document * bkd_parse(struct bkd_istream * in) {
    return NULL;
}
