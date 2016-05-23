#include "bkd.h"
#include <string.h>

int bkd_putn(struct bkd_ostream * out, const uint8_t * data, uint32_t size) {
    return out->stream(out->user, data, size);
}

int bkd_puts(struct bkd_ostream * out, const char * str) {
    uint32_t size = strlen(str);
    return bkd_putn(out, (const uint8_t *) str, size);
}

int bkd_putc(struct bkd_ostream * out, char c) {
    return bkd_putn(out, (const uint8_t *) &c, 1);
}

#ifndef BKD_NO_STDIO

static int file_put_impl(void * user, const uint8_t * data, uint32_t size) {
    FILE * file = (FILE *) user;
    for (uint32_t i = 0; i < size; i++)
        fputc(data[i], file);
    return 0;
}

static int stdout_put_impl(void * user, const uint8_t * data, uint32_t size) {
    if (user != NULL) return 1;
    return file_put_impl(stdout, data, size);
}

struct bkd_ostream * bkd_fileostream(struct bkd_ostream * out, FILE * file) {
    out->user = file;
    out->stream = file_put_impl;
    return out;
}

static struct bkd_ostream _bkd_stdout = {
    NULL,
    stdout_put_impl
};

struct bkd_ostream * BKD_STDOUT = &_bkd_stdout;

#endif
