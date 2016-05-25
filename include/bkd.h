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

#include "bkd_ast.h"
#include "bkd_stream.h"

/* Define NULL in case stdlib not included */
#ifndef NULL
#define NULL ((void *)0)
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
