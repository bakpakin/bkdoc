#include "bkd.h"
#include "bkd_utf8.h"
#include <string.h>

/* Error strings */
const char * bkd_errors[] = {
    NULL,
    "Out of memory.",
    "Invalid markup type.",
    "Unknown node type."
};

/* Output streams */

int bkd_putn(struct bkd_ostream * out, const uint8_t * data, uint32_t size) {
    return out->type->stream(out, data, size);
}

int bkd_puts(struct bkd_ostream * out, const char * str) {
    uint32_t size = strlen(str);
    return bkd_putn(out, (const uint8_t *) str, size);
}

int bkd_putc(struct bkd_ostream * out, char c) {
    return bkd_putn(out, (const uint8_t *) &c, 1);
}

void bkd_flush(struct bkd_ostream * out) {
    if (out->type->flush)
        out->type->flush(out);
}

/* Input streams */

struct bkd_istream * bkd_istream_init(struct bkd_istreamdef * type, struct bkd_istream * stream, void * user) {
    stream->type = type;
    stream->user = user;
    stream->buflen = 80;
    stream->linelen = 0;
    stream->done = 0;
    stream->buffer = BKD_MALLOC(80);
    return stream;
}

void bkd_istream_freebuf(struct bkd_istream * in) {
    BKD_FREE(in->buffer);
    in->buffer = NULL;
}

uint8_t * bkd_getl(struct bkd_istream * in, uint32_t  * len) {
    if (in->done)
        return NULL;
    in->type->line(in);
    return bkd_lastl(in, len);
}

uint8_t * bkd_lastl(struct bkd_istream * in, uint32_t * len) {
    if (in->buffer != NULL && !in->done) {
        if (len != NULL)
            *len = in->linelen;
        return in->buffer;
    } else {
        return NULL;
    }
}

#ifndef BKD_NO_STDIO

/* stdio output stream */

static int file_put_impl(FILE * file, const uint8_t * data, uint32_t size) {
    for (uint32_t i = 0; i < size; i++)
        fputc(data[i], file);
    return 0;
}

static int file_put(struct bkd_ostream * out, const uint8_t * data, uint32_t size) {
    return file_put_impl((FILE *) out->user, data, size);
}

static int file_flush(struct bkd_ostream * out) {
    fflush((FILE *) out->user);
    return 0;
}

static int stdout_put(struct bkd_ostream * out, const uint8_t * data, uint32_t size) {
    out->user = stdout;
    return file_put(out, data, size);
}

static struct bkd_ostreamdef _bkd_file_ostreamdef = {
    file_put,
    file_flush
};

static struct bkd_ostreamdef _bkd_stdout_def = {
    stdout_put,
    file_flush
};

static struct bkd_ostream _bkd_stdout = {
    &_bkd_stdout_def,
    NULL
};

struct bkd_ostreamdef * BKD_FILE_OSTREAMDEF = &_bkd_file_ostreamdef;
struct bkd_ostream * BKD_STDOUT = &_bkd_stdout;

/* stdio input stream */

static int file_getl(struct bkd_istream * in) {
    FILE * file = (FILE *) in->user;
    uint32_t len = 0;
    int c;
    for(;;) {
        c = fgetc(file);
        if(c == EOF) {
            if (len == 0) {
                in->done = 1;
                return 0;
            } else
                break;
        }
        if (c == '\n')
            break;
        if(len + 1 >= in->buflen) {
            uint8_t * newbuf = BKD_REALLOC(in->buffer, len * 2);
            if(newbuf == NULL) {
                return 0;
            } else {
                in->buffer = newbuf;
                in->buflen = len * 2;
            }
        }
        if (c != '\r')
            in->buffer[len++] = c;
    }
    in->linelen = len;
    return 1;
}

static int stdin_getl(struct bkd_istream * in) {
    in->user = stdin;
    if (!in->buffer)
        bkd_istream_init(in->type, in, stdin);
    return file_getl(in);
}

static struct bkd_istreamdef _bkd_file_istreamdef = {
    file_getl
};

static struct bkd_istreamdef _bkd_stdin_istreamdef = {
    stdin_getl
};

static struct bkd_istream _bkd_stdin = {
    &_bkd_stdin_istreamdef,
    NULL,
    0,
    NULL,
    0,
    0
};

struct bkd_istreamdef * BKD_FILE_ISTREAMDEF = &_bkd_file_istreamdef;
struct bkd_istream * BKD_STDIN = &_bkd_stdin;

struct bkd_istream bkd_file_istream(FILE * file) {
    struct bkd_istream stream;
    bkd_istream_init(BKD_FILE_ISTREAMDEF, &stream, file);
    return stream;
}

struct bkd_ostream bkd_file_ostream(FILE * file) {
    struct bkd_ostream stream;
    stream.type = BKD_FILE_OSTREAMDEF;
    stream.user = file;
    return stream;
}

#endif
