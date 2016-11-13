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
#include "bkd_stretchy.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OPT_HELP 1
#define OPT_VERSION 2
#define OPT_STANDALONE 4

int get_option(char o) {
    switch (o) {
        case 'h': return OPT_HELP;
        case 'v': return OPT_VERSION;
        case 's': return OPT_STANDALONE;
        default: return 0;
    }
}

struct bkd_istream loadfile(const char * name) {
    FILE *f = fopen(name, "r");
    return bkd_file_istream(f);
}

int main(int argc, char *argv[]) {
    int64_t currentArg = 1;
    uint64_t options = 0;
    uint32_t print_options = 0;
    struct bkd_htmlinsert *inserts = NULL;

    /* Get options */
    for (currentArg = 1; currentArg < argc; currentArg++) {
        char * arg = argv[currentArg];
        size_t len = strlen(arg);
        /* text option */
        if (arg[0] == '-') {
            if (arg[1] == '-') {
                struct bkd_htmlinsert insert;
                if (strncmp(arg + 2, "style-inline=", 13) == 0) {
                    insert.type = BKD_HTML_INSERTSTYLE;
                    insert.data.string.data = (uint8_t *) arg + 2 + 13;
                    insert.data.string.length = strlen((char *) insert.data.string.data);
                } else if (strncmp(arg + 2, "script-inline=", 14) == 0) {
                    insert.type = BKD_HTML_INSERTSCRIPT;
                    insert.data.string.data = (uint8_t *) arg + 2 + 14;
                    insert.data.string.length = strlen((char *) insert.data.string.data);
                } else if (strncmp(arg + 2, "style-file=", 11) == 0) {
                    insert.type = BKD_HTML_INSERTSTYLE | BKD_HTML_INSERT_ISSTREAM;
                    insert.data.stream = BKD_MALLOC(sizeof(struct bkd_istream));
                    *insert.data.stream = loadfile(arg + 2 + 11);
                } else if (strncmp(arg + 2, "script-file=", 12) == 0) {
                    insert.type = BKD_HTML_INSERTSCRIPT | BKD_HTML_INSERT_ISSTREAM;
                    insert.data.stream = BKD_MALLOC(sizeof(struct bkd_istream));
                    *insert.data.stream = loadfile(arg + 2 + 12);
                } else if (strncmp(arg + 2, "style=", 6) == 0) {
                    insert.type = BKD_HTML_INSERTSTYLE | BKD_HTML_INSERT_ISLINK;
                    insert.data.string.data = (uint8_t *) arg + 2 + 6;
                    insert.data.string.length = strlen((char *) insert.data.string.data);
                } else if (strncmp(arg + 2, "script=", 7) == 0) {
                    insert.type = BKD_HTML_INSERTSCRIPT | BKD_HTML_INSERT_ISLINK;
                    insert.data.string.data = (uint8_t *) arg + 2 + 7;
                    insert.data.string.length = strlen((char *) insert.data.string.data);
                }
                bkd_sbpush(inserts, insert);
            } else {
                for (size_t i = 1; i < len; i++)
                    options |= get_option(arg[i]);
            }
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

    if (options & OPT_STANDALONE) {
        print_options |= BKD_OPTION_STANDALONE;
    }

    struct bkd_list * doc = bkd_parse(BKD_STDIN);
    bkd_html(BKD_STDOUT, doc, print_options, bkd_sbcount(inserts), inserts);
    bkd_docfree(doc);

    /* Close ingoing files */
    for (int32_t i = 0; i < bkd_sbcount(inserts); ++i) {
        if (inserts[i].type & BKD_HTML_INSERT_ISSTREAM) {
            bkd_istream_freebuf(inserts[i].data.stream);
            fclose((FILE *) inserts[i].data.stream->user);
            BKD_FREE(inserts[i].data.stream);
        }
    }

    bkd_sbfree(inserts);

    bkd_istream_freebuf(BKD_STDIN);
    return 0;
}
