#ifndef BKD_HEADER_
#define BKD_HEADER_

#ifndef BKD_MALLOC
#include <stdlib.h>
#define BKD_MALLOC malloc
#endif

#ifndef BKD_REALLOC
#include <stdlib.h>
#define BKD_REALLOC realloc
#endif

#ifndef BKD_FREE
#include <stdlib.h>
#define BKD_FREE free
#endif

#include <stdint.h>
#include <stddef.h>

/*
 * Denotes the type of a block level element.
 */
#define BKD_PARAGRAPH 0
#define BKD_OLIST 1
#define BKD_ULIST 2
#define BKD_TABLE 3
#define BKD_HEADER 4
#define BKD_LINEBREAK 5
#define BKD_CODEBLOCK 6
#define BKD_COMMENTBLOCK 7
#define BKD_TEXT 8
#define BKD_DATASTRING 9
#define BKD_COUNT_TYPE 10

/*
 * Denotes the type of a "markup". A "markup" is a transformation
 * on text within a bkd_linenode. Bold, italics, and inline code blocks
 * are all example of "markups". Images and links are also represented
 * in this way, with the link title and the image alternate text as the
 * original text, and the link destination and image url as the custom data.
 */
#define BKD_NONE 0
#define BKD_BOLD 1
#define BKD_ITALICS 2
#define BKD_CODEINLINE 3
#define BKD_MATH 4
#define BKD_IMAGE 5
#define BKD_LINK 6
#define BKD_STRIKETHROUGH 7
#define BKD_UNDERLINE 8
#define BKD_COUNT_MARKUP 9

/*
 * Style
 */
#define BKD_SOLID 0
#define BKD_DOTTED 1
#define BKD_INVISIBLE 2
#define BKD_PAGEBREAK 3
#define BKD_COUNT_STYLE 4


/* Strings */
struct bkd_string {
    uint32_t length;
    uint8_t * data;
};

/* Returns a substring of string in the same memory location */
struct bkd_string bkd_strsub(struct bkd_string string, int32_t index1, int32_t index2);

/* Returns a clone of string */
struct bkd_string bkd_strclone(struct bkd_string string);

struct bkd_node;

/* the 'tree' union should be treated as a 'leaf' if
 * nodeCount is 0, otherwise as a 'node'. */
struct bkd_linenode {
    uint8_t markupType;
    uint32_t nodeCount;
    union {
        struct bkd_string leaf;
        struct bkd_linenode * node;
    } tree;
    struct bkd_string data;
};

struct bkd_paragraph {
    struct bkd_linenode text;
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
    struct bkd_linenode text;
};

struct bkd_linebreak {
    uint8_t style;
};

struct bkd_codeblock {
    struct bkd_string text;
    struct bkd_string language;
};

struct bkd_commentblock {
    struct bkd_linenode text;
};

struct bkd_document {
    uint32_t itemCount;
    struct bkd_node * items;
};

struct bkd_node {
    uint8_t type;
    union {
        struct bkd_linenode text;
        struct bkd_paragraph paragraph;
        struct bkd_olist olist;
        struct bkd_ulist ulist;
        struct bkd_table table;
        struct bkd_header header;
        struct bkd_linebreak linebreak;
        struct bkd_codeblock codeblock;
        struct bkd_commentblock commentblock;
        struct bkd_string datastring;
    } data;
};

/* Simple output streams */
struct bkd_ostream;

struct bkd_ostreamdef {
    int (*stream)(struct bkd_ostream * self, const struct bkd_string data);
    int (*flush)(struct bkd_ostream * self);
};

struct bkd_ostream {
    struct bkd_ostreamdef * type;
    void * user;
};

int bkd_putn(struct bkd_ostream * out, struct bkd_string str);
int bkd_puts(struct bkd_ostream * out, const char * str);
int bkd_putc(struct bkd_ostream * out, char c);
void bkd_flush(struct bkd_ostream * out);

/* Simple input streams */
struct bkd_istream;

struct bkd_istreamdef {
    int (*line)(struct bkd_istream * self);
};

struct bkd_istream {
    struct bkd_istreamdef * type;
    void * user;
    struct bkd_string buffer;
    uint32_t linelen;
    uint8_t done;
};

struct bkd_string bkd_getl(struct bkd_istream * in);
struct bkd_string bkd_lastl(struct bkd_istream * in);
void bkd_istream_freebuf(struct bkd_istream * in);

extern struct bkd_istreamdef * BKD_STRING_ISTREAMDEF;
extern struct bkd_ostreamdef * BKD_STRING_OSTREAMDEF;

/* Standard IO Streams */
#ifndef BKD_NO_STDIO
#include <stdio.h>
extern struct bkd_istreamdef * BKD_FILE_ISTREAMDEF;
extern struct bkd_istream * BKD_STDIN;
extern struct bkd_ostreamdef * BKD_FILE_OSTREAMDEF;
extern struct bkd_ostream * BKD_STDOUT;
struct bkd_istream bkd_file_istream(FILE * file);
struct bkd_ostream bkd_file_ostream(FILE * file);
#endif

/* Define NULL in case stdlib not included */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Define an empty string */
#define BKD_NULLSTR ((struct bkd_string){0, NULL})

/* Array of error messages. */
extern const char * bkd_errors[];

#ifndef BKD_ERROR
#include <stdio.h>
#define BKD_ERROR(CODE) fprintf(stderr, "BKD Error code %d: %s\n", CODE, bkd_errors[CODE])
#endif

/* Error codes */
#define BKD_ERROR_OUT_OF_MEMORY 1
#define BKD_ERROR_INVALID_MARKUP_TYPE 2
#define BKD_ERROR_INVALID_MARKUP_PATTERN 3
#define BKD_ERROR_UNKNOWN_NODE 4

/* Main Functions */
struct bkd_document * bkd_parse(struct bkd_istream * in);
void bkd_parse_cleanup(struct bkd_document * document);

struct bkd_linenode * bkd_parse_line(struct bkd_linenode * node, struct bkd_string string);

#endif /* end of include guard: BKD_HEADER_ */
