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

#ifndef BKD_HTML_
#define BKD_HTML_

#include "bkd.h"

#define BKD_HTML_INSERTSTYLE 1
#define BKD_HTML_INSERTSCRIPT 2
#define BKD_HTML_INSERT_ISLINK 4
#define BKD_HTML_INSERT_ISSTREAM 8

struct bkd_htmlinsert {
    uint32_t type;
    union {
        struct bkd_string string;
        struct bkd_istream * stream;
    } data;
};

int bkd_html(
        struct bkd_ostream * out,
        struct bkd_list * document,
        uint32_t options,
        uint32_t insertCount,
        struct bkd_htmlinsert * inserts);

int bkd_html_fragment(
        struct bkd_ostream * out,
        struct bkd_node * node);

#endif /* end of include guard: BKD_HTML_ */
