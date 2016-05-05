#ifndef BKD_PARSE_H_
#define BKD_PARSE_H_

#include "bkd_ast.h"
#include <stdint.h>
#include <stdio.h>

struct bkd_parsestate {
    void * memory;
    enum bkd_type lastType;

};

bkd_document * bkd_parsef(FILE * f);

bkd_document * bkd_parses(const uint8_t * markup);

#endif /* end of include guard: BKD_PARSE_H_ */
