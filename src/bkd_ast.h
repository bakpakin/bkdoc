/*
 * This header describes all of the data that can be represented in the bakdown format.
 * All manipulation is done by transforming the bakdown into a bkd_node or bkd_document.
 */

#ifndef BKD_AST_H_
#define BKD_AST_H_

#include <stdint.h>

/*
 * Denotes the type of a block level element.
 */
enum bkd_type {
    BKD_PARAGRAPH = 0,
    BKD_OLIST = 1,
    BKD_ULIST = 2,
    BKD_TABLE = 3,
    BKD_HEADER = 4,
    BKD_LINEBREAK = 5,
    BKD_CODEBLOCK = 6,
    BKD_COMMENTBLOCK = 7
};

/*
 * Denotes the type of a "markup". A "markup" is a transformation
 * on text within a bkd_text. Bold, italics, and inline code blocks
 * are all example of "markups". Images and links are also represented
 * in this way, with the link title and the image alternate text as the
 * original text, and the link destination and image url as the custom data.
 */
enum bkd_markuptype {
    BKD_BOLD = 0,
    BKD_ITALICS = 1,
    BKD_CODEINLINE = 2,
    BKD_MATH = 4,
    BKD_IMAGE = 5,
    BKD_LINK = 6
};

/*
 * Style
 */
enum bkd_style {
    BKD_SOLID = 0,
    BKD_DOTTED = 1
};

struct bkd_node;

struct bkd_markup {
    enum bkd_markuptype type;
    uint32_t start;
    uint32_t count;
    uint8_t * data; // Optional; only for image and link types.
};

struct bkd_text {
    uint32_t texLength;
    uint8_t * text;
    uint32_t markupCount;
    struct bkd_markup * markups;
};

struct bkd_paragraph {
    struct bkd_text text;
};

struct bkd_olist {
    uint32_t itemCount;
    struct bkd_node * items;
};

struct bkd_ulist {
    uint32_t itemCount;
    struct bkd_node * items;
};

struct bkd_table {
    uint32_t rows;
    uint32_t cols;
    struct bkd_node * items;
};

struct bkd_header {
    uint32_t size;
    struct bkd_text text;
};

struct bkd_linebreak {
    enum bkd_style style;
};

struct bkd_codeblock {
    uint32_t texLength;
    uint8_t * text;
    uint8_t * language;
};

struct bkd_commentblock {
    struct bkd_text text;
};

struct bkd_document {
    uint32_t itemCount;
    struct bkd_node * items;
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
        struct bkd_commentblock commentblock;
    } data;
};

#endif /* end of include guard: BKD_AST_H_ */
