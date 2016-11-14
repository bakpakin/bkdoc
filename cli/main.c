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

#define CLI_FLAG_TAKESARGS 1
#define CLI_FLAG_BIT 2

static const char cli_title[] = "BKDoc v0.0 Copyright 2016 Calvin Rose.\n";

struct cli_option {
    const char * name;
    unsigned char shortName;
    int flags;
    const char * description;
};

struct cli_optionparam {
    struct bkd_string data;
    int valid: 1;
};

static struct cli_option options[] = {
    {"standalone", 's', 2, "Generates an HTML document that is standalone, rather than a fragment of HTML"},
    {"script-inline", 'I', 1, "Inserts a script literal inline into the output HTML"},
    {"style-inline", 'i', 1, "Inserts CSS literal inline into the output HTML"},
    {"script-file", 'F', 1, "Inserts a script inline from a file into the output HTML"},
    {"style-file", 'f', 1, "Inserts a CSS style inline into the output HTML"},
    {"script", 'T', 1, "Inserts a script via href into the output HTML"},
    {"style", 't', 1, "Inserts a css stylesheet via href into the output HTML"},
    {"version", 'v', 2, "Prints the version"},
    {"help", 'h', 2, "Prints the help description"}
};

static struct cli_optionparam opts[128];

static int getopt(const char * arg) {
    if (arg[0] != '-') return -1;
    int size = sizeof(options) / sizeof(options[0]);
    if (arg[1] == '-') {
        for (int i = 0; i < size; ++i) {
            const char * name = options[i].name;
            if (options[i].flags & CLI_FLAG_TAKESARGS) {
                size_t namelen = strlen(options[i].name);
                if (strncmp(arg + 2, name, namelen) == 0 && arg[namelen + 2] == '=') {
                    const char * data = arg + namelen + 3;
                    struct bkd_string param;
                    param.data = (uint8_t *) data;
                    param.length = strlen(data);
                    opts[options[i].shortName].valid = 1;
                    opts[options[i].shortName].data = param;
                    return options[i].shortName;
                }
            } else if (strcmp(arg + 2, name) == 0) {
                opts[options[i].shortName].valid = 1;
                opts[options[i].shortName].data = BKD_NULLSTR;
                return options[i].shortName;
            }
        }
    } else {
        for (const char *d = arg + 1; *d != '\0'; ++d) {
            int found = 0;
            for (int i = 0; i < size; ++i) {
                if (!(options[i].flags & CLI_FLAG_TAKESARGS) && options[i].shortName == *d) {
                    found = 1;
                    opts[options[i].shortName].valid = 1;
                    opts[options[i].shortName].data = BKD_NULLSTR;
                    break;
                }
            }
            if (!found) return -1;
        }
    }
    return 0;
}

struct bkd_istream loadfile(struct bkd_string filename) {
    FILE *f = fopen((char *)filename.data, "r");
    return bkd_file_istream(f);
}

int main(int argc, char *argv[]) {
    int64_t currentArg = 1;
    uint32_t print_options = 0;
    struct bkd_htmlinsert *inserts = NULL;

    /* Clear opts */
    memset(opts, 0, sizeof(opts));

    /* Get options */
    for (currentArg = 1; currentArg < argc; currentArg++) {
        char * arg = argv[currentArg];
        /* text option */
        struct bkd_htmlinsert insert;
        int option = getopt(arg);
        switch(option) {
            case -1: return 1;
            case 'I': /* script inline */
                     insert.type = BKD_HTML_INSERTSCRIPT;
                     insert.data.string = opts['I'].data;
            case 'i': /* style inline */
                     insert.type = BKD_HTML_INSERTSTYLE;
                     insert.data.string = opts['i'].data;
            case 'F': /* script file */
                     insert.type = BKD_HTML_INSERTSCRIPT | BKD_HTML_INSERT_ISSTREAM;
                     insert.data.stream = BKD_MALLOC(sizeof(struct bkd_istream));
                     *insert.data.stream = loadfile(opts['F'].data);
            case 'f': /* style file */
                     insert.type = BKD_HTML_INSERTSTYLE | BKD_HTML_INSERT_ISSTREAM;
                     insert.data.stream = BKD_MALLOC(sizeof(struct bkd_istream));
                     *insert.data.stream = loadfile(opts['f'].data);
            case 'T': /* script link */
                     insert.type = BKD_HTML_INSERTSCRIPT | BKD_HTML_INSERT_ISLINK;
                     insert.data.string = opts['T'].data;
            case 't': /* style link*/
                     insert.type = BKD_HTML_INSERTSTYLE | BKD_HTML_INSERT_ISLINK;
                     insert.data.string = opts['t'].data;
            default: break;
        }
        if (option != 0) {
            bkd_sbpush(inserts, insert);
        }
    }

    /* Show version and exit */
    if (opts['v'].valid) {
        printf(cli_title);
        return 0;
    }

    /* Show help text and exit */
    if (opts['h'].valid) {
        printf(cli_title);
        printf("\n%s [options] < filein.bkd > fileout.bkd\n\n", argv[0]);
        int size = sizeof(options) / sizeof(options[0]);
        for (int i = 0; i < size; ++i) {
            struct cli_option o = options[i];
            const char spaces[] = "                          ";
            printf("    --%s", o.name);
            if (o.flags & CLI_FLAG_TAKESARGS) {
                int nums = 22 - strlen(o.name);
                printf("=...%.*s%s\n", nums, spaces, o.description);
            } else {
                int nums = 18 - strlen(o.name);
                printf("%.*s  (-%c)  %s\n", nums, spaces, o.shortName, o.description);
            }
        }
        printf("\n");
        return 0;
    }

    if (opts['s'].valid) {
        print_options |= BKD_OPTION_STANDALONE;
    }

    struct bkd_list * doc = bkd_parse(BKD_STDIN);

    /* Add some buffering to stdout for more speed */
    do {
        char buffer[8192];
        setvbuf(stdout, buffer, _IOFBF, sizeof(buffer));
    } while (0);
    bkd_html(BKD_STDOUT, doc, print_options, bkd_sbcount(inserts), inserts);
    fflush(stdout);

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
