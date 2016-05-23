#ifndef BKD_HEADER_
#define BKD_HEADER_

#include "bkd_ast.h"

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

/* Simple output streams */
struct bkd_ostream {
    void * user;
    int (*stream)(void * user, const uint8_t * data, uint32_t size);
};

int bkd_putn(struct bkd_ostream * out, const uint8_t * data, uint32_t size);
int bkd_puts(struct bkd_ostream * out, const char * str);
int bkd_putc(struct bkd_ostream * out, char c);

#ifndef BKD_NO_STDIO
#include <stdio.h>
struct bkd_ostream * bkd_fileostream(struct bkd_ostream * out, FILE * file);
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
