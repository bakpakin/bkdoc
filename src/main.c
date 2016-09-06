/*
 * The main BKD program. Read bkd files, and outputs other kinds
 * of files.
 */
#include "bkd.h"
#include "bkd_html.h"

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

void handler(int sig) {
    fprintf(stderr, "Signal Error: %d\n", sig);
    exit(sig);
}

int main () {
    signal(SIGSEGV, handler);
    FILE * f = fopen("test.bkd", "r");
    struct bkd_istream in = bkd_file_istream(f);

    /* while (!in.done) { */
    /*     bkd_putn(BKD_STDOUT, bkd_getl(&in)); */
    /*     if (in.done) break; */
    /*     bkd_putc(BKD_STDOUT, '\n'); */
    /* } */

    struct bkd_document * doc = bkd_parse(&in);
    bkd_html(BKD_STDOUT, doc);

}
