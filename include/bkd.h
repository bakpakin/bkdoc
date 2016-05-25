#ifndef BKD_HEADER_
#define BKD_HEADER_

#include "bkd_ast.h"
#include <stddef.h>

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

/* Define NULL in case stdlib not included */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Simple output streams */
struct bkd_ostream;

struct bkd_ostreamdef {
    int (*init)(struct bkd_ostream * self);
    int (*stream)(struct bkd_ostream * self, const uint8_t * data, uint32_t size);
    int (*flush)(struct bkd_ostream * self);
    int (*deinit)(struct bkd_ostream * self);
};

struct bkd_ostream {
    struct bkd_ostreamdef * type;
    void * user;
};

struct bkd_ostream * bkd_ostream_init(struct bkd_ostreamdef * type, struct bkd_ostream * stream, void * user);
void bkd_ostream_deinit(struct bkd_ostream * stream);
int bkd_putn(struct bkd_ostream * out, const uint8_t * data, uint32_t size);
int bkd_puts(struct bkd_ostream * out, const char * str);
int bkd_putc(struct bkd_ostream * out, char c);
void bkd_flush(struct bkd_ostream * out);

/* Simple input streams */
struct bkd_istream;

struct bkd_istreamdef {
    int (*init)(struct bkd_istream * self);
    int (*line)(struct bkd_istream * self);
    int (*deinit)(struct bkd_istream * self);
};

struct bkd_istream {
    struct bkd_istreamdef * type;
    void * user;
    size_t buflen; /* size of buffer */
    uint8_t * buffer;
    size_t readlen; /* size of characters read in buffer */
    size_t linelen; /* length of first line in buffer */

    /* linelen <= readlen <= buflen */

};

struct bkd_istream * bkd_istream_init(struct bkd_istreamdef * type, struct bkd_istream * stream, void * user);
void bkd_istream_deinit(struct bkd_istream * stream);
uint8_t * bkd_getl(struct bkd_istream * in, size_t * len);

#ifndef BKD_NO_STDIO
#include <stdio.h>
extern struct bkd_istreamdef * BKD_FILE_ISTREAMDEF;
extern struct bkd_istream * BKD_STDIN;
extern struct bkd_ostreamdef * BKD_FILE_OSTREAMDEF;
extern struct bkd_ostream * BKD_STDOUT;
#endif

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

#endif /* end of include guard: BKD_HEADER_ */
