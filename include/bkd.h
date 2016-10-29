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

#ifndef BKD_CALLOC
#include <stdlib.h>
#define BKD_CALLOC calloc
#endif

#ifndef BKD_FREE
#include <stdlib.h>
#define BKD_FREE free
#endif

#include <stdint.h>
#include <stddef.h>

#define TRC() printf("TRACE: line %d in %s\n", __LINE__, __FILE__)

/*
 * Denotes the type of a block level element.
 */
#define BKD_PARAGRAPH 0
#define BKD_TABLE 1
#define BKD_HEADER 2
#define BKD_HORIZONTALRULE 3
#define BKD_CODEBLOCK 4
#define BKD_COMMENTBLOCK 5
#define BKD_TEXT 6
#define BKD_DATASTRING 7
#define BKD_LIST 8
#define BKD_COUNT_TYPE 9

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
#define BKD_STRIKETHROUGH 4
#define BKD_UNDERLINE 8
#define BKD_LINK 16
#define BKD_MATH 32
#define BKD_IMAGE 64
#define BKD_SUBSCRIPT 128
#define BKD_SUPERSCRIPT 256
#define BKD_CODEINLINE 512

/* Different styles for sub documents, or lists. */
#define BKD_LISTSTYLE_NONE 0
#define BKD_LISTSTYLE_NUMBERED 1
#define BKD_LISTSTYLE_BULLETS 2
#define BKD_LISTSTYLE_ALPHA 3
#define BKD_LISTSTYLE_ROMAN 4

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

struct bkd_node;

/* the 'tree' union should be treated as a 'leaf' if
 * nodeCount is 0, otherwise as a 'node'. */
struct bkd_linenode {
    uint32_t markup;
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

struct bkd_list {
    uint32_t style;
    uint32_t itemCount;
    struct bkd_node * items;
};

struct bkd_node {
    uint8_t type;
    union {
        struct bkd_linenode text;
        struct bkd_paragraph paragraph;
        struct bkd_table table;
        struct bkd_header header;
        struct bkd_linebreak linebreak;
        struct bkd_codeblock codeblock;
        struct bkd_commentblock commentblock;
        struct bkd_string datastring;
        struct bkd_list list;
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

/* Buffers */
struct bkd_buffer {
    uint32_t capacity;
    struct bkd_string string;
};

struct bkd_istream {
    struct bkd_istreamdef * type;
    void * user;
    struct bkd_buffer buffer;
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
#define BKD_ERROR_UNKNOWN 5

/* Main Functions */
struct bkd_list * bkd_parse(struct bkd_istream * in);
void bkd_docfree(struct bkd_list * document);

struct bkd_linenode * bkd_parse_line(struct bkd_linenode * node, struct bkd_string string);

#endif /* end of include guard: BKD_HEADER_ */
