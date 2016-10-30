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

/*
 * The main BKD program. Read bkd files, and outputs other kinds
 * of files.
 */
#include "bkd.h"
#include "bkd_html.h"
#include "bkd_string.h"
#include "bkd_utf8.h"

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

void handler(int sig) {
    fprintf(stderr, "Signal Error: %d\n", sig);
    exit(sig);
}

#define OPT_HELP 1
#define OPT_VERSION 2

int get_option(char o) {
    switch (o) {
        case 'h': return OPT_HELP;
        case 'v': return OPT_VERSION;
        default: return 0;
    }
}

int main(int argc, char *argv[]) {
    int64_t currentArg = 1;
    uint64_t options = 0;
    signal(SIGSEGV, handler);

    /* Get options */
    for (currentArg = 1; currentArg < argc; currentArg++) {
        char * arg = argv[currentArg];
        size_t len = strlen(arg);
        /* text option */
        if (arg[0] == '-') {
            for (size_t i = 1; i < len; i++)
                options |= get_option(arg[i]);
        } else {

        }
    }

    /* Show version and exit */
    if (options & OPT_VERSION) {
        printf("BKDoc v0.0 Copyright 2016 Calvin Rose.\n");
        return 0;
    }

    /* Show help text and exit */
    if (options & OPT_HELP) {
        printf("BKDoc v0.0 Copyright 2016 Calvin Rose.\n");
        printf("%s [options] < filein.bkd > fileout.bkd\n", argv[0]);
        printf("\t-h : Shows this help text.\n");
        printf("\t-v : Shows the version and copyright information.\n");
        return 0;
    }

    struct bkd_list * doc = bkd_parse(BKD_STDIN);
    bkd_html(BKD_STDOUT, doc);
    bkd_docfree(doc);

    bkd_istream_freebuf(BKD_STDIN);
    return 0;
}
