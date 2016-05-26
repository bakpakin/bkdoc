#include "bkd.h"
#include "bkd_utf8.h"
#include "bkd_parse.h"

#include <string.h>

enum markupchar {
    MUC_NONE,
    MUC_1STAR,
    MUC_2STAR,
    MUC_1UNDERSCORE,
    MUC_2UNDERSCORE,
    MUC_BANG,
    MUC_OPENBRACKET,
    MUC_CLOSEBRACKET,
    MUC_OPENPAREN,
    MUC_CLOSEPAREN,
    MUC_BACKTICK
};

static enum markupchar charCheckMarkup(uint8_t * c, uint32_t maxlen) {
    switch (*c) {
        case '*':
            if (maxlen > 1 && c[1] == '*') {
                return MUC_2STAR;
            } else {
                return MUC_1STAR;
            }
        case '_':
            if (maxlen > 1 && c[1] == '_') {
                return MUC_2UNDERSCORE;
            } else {
                return MUC_1UNDERSCORE;
            }
        case '!':
            return MUC_BANG;
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
        default:
            return MUC_NONE;
    }
}

static struct bkd_linenode * create_line(uint8_t * utf8, uint32_t len) {
    struct bkd_linenode * l = BKD_MALLOC(sizeof(struct bkd_linenode));
    return l;
}

static void add_markup(struct bkd_parsestate * B, uint8_t * text, uint32_t length) {

}

struct bkd_document * bkd_parsef(FILE * f) {
    return NULL;
}

struct bkd_document * bkd_parses(const uint8_t * markup) {
    return NULL;
}
