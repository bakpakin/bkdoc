/*
 * The main BKD program. Read bkd files, and outputs other kinds
 * of files.
 */
#include "bkd.h"
#include "bkd_html.h"

#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

void handler(int sig) {
    fprintf(stderr, "FUCK. Error: %d\n", sig);
    exit(sig);
}

int main (int argc, const char ** argv) {

    signal(SIGSEGV, handler);
    FILE * f = fopen("test.bkd", "r");
    struct bkd_istream in = bkd_file_istream(f);
    struct bkd_document * doc = bkd_parse(&in);
    bkd_html(BKD_STDOUT, doc);

}
