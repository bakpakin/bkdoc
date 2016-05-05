/*
 * This header describes all of the data that can be represented in the bakdown format.
 * All manipulation is done by transforming the bakdown into a bkd_node or bkd_document.
 */

#ifndef BKD_AST_H_
#define BKD_AST_H_

#include <stdint.h>

enum bkd_type {
    BKD_PARAGRAPH = 0,
    BKD_OLIST = 1,
    BKD_ULIST = 2,
    BKD_TABLE = 3,
    BKD_HEADER = 4,
    BKD_LINEBREAK = 5,
    BKD_CODEBLOCK = 6,
};

enum bkd_markuptype {
    BKD_BOLD = 0,
    BKD_ITALICS = 1,
    BKD_CODEINLINE = 2,
    BKD_MATH = 3
};

enum bkd_style {
    BKD_SOLID = 0,
    BKD_DOTTED = 1
};

struct bkd_node;

struct bkd_markup {
    enum bkd_markuptype type;
    uint32_t start;
    uint32_t count;
};

struct bkd_paragraph {
    uint32_t texLength;
    uint8_t * text;
    uint32_t markupCount;
    bkd_markup * markups;
};

struct bkd_olist {
    uint32_t itemCount;
    bkd_node * items;
};

struct bkd_ulist {
    uint32_t itemCount;
    bkd_node * items;
};

struct bkd_table {
    uint32_t rows;
    uint32_t cols;
    bkd_node * items;
};

struct bkd_header {
    uint32_t size;
    uint32_t textLength;
    uint8_t * text;
    uint32_t markupCount;
    bkd_markup * markups;
};

struct bkd_linebreak {
    enum bkd_style style;
};

struct bkd_codeblock {
    uint32_t texLength;
    uint8_t * text;
    uint8_t * language;
};

struct bkd_document {
    uint32_t itemCount;
    bkd_node * items;
};

struct bkd_node {
    enum bkd_type type;
    union {
        struct bkd_paragraph paragraph;
        struct bkd_olist olist;
        struct bkd_ulist ulist;
        struct bkd_table table;
        struct bkd_header header;
        struct bkd_linebreak linebreak;
        struct bkd_codeblock codeblock;
    } data;
};

#endif /* end of include guard: BKD_AST_H_ */
