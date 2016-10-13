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

int main () {
    signal(SIGSEGV, handler);

    FILE * fi = fopen("test.bkd", "r");
    FILE * fo = fopen("test.html", "w");
    struct bkd_istream in = bkd_file_istream(fi);
    struct bkd_ostream out = bkd_file_ostream(fo);

    struct bkd_list * doc = bkd_parse(&in);
    bkd_html(&out, doc);
    bkd_html(BKD_STDOUT, doc);
    bkd_docfree(doc);

    bkd_istream_freebuf(&in);
    fclose(fi);
    fclose(fo);

}
