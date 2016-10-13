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

void handler(int sig) {
    fprintf(stderr, "Signal Error: %d\n", sig);
    exit(sig);
}

int main(int argc, char *argv[])
{
    signal(SIGSEGV, handler);

    struct bkd_list * doc = bkd_parse(BKD_STDIN);
    bkd_html(BKD_STDOUT, doc);
    bkd_docfree(doc);

    bkd_istream_freebuf(BKD_STDIN);
    return 0;
}
