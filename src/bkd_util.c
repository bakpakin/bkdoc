#include "bkd.h"
#include "bkd_utf8.h"
#include <string.h>

/* Output streams */

struct bkd_ostream * bkd_ostream_init(struct bkd_ostreamdef * type, struct bkd_ostream * stream, void * user) {
    stream->type = type;
    stream->user = user;
    if (type->init)
        type->init(stream);
    return stream;
}

void bkd_ostream_deinit(struct bkd_ostream * stream) {
    if (stream->type->deinit)
        stream->type->deinit(stream);
}

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
    stream->readlen = 0;
    stream->buffer = BKD_MALLOC(stream->buflen);
    if (type->init)
        type->init(stream);
    return stream;
}

void bkd_istream_deinit(struct bkd_istream * stream) {
    if (stream->type->deinit)
        stream->type->deinit(stream);
    BKD_FREE(stream->buffer);
}

uint8_t * bkd_getl(struct bkd_istream * in, size_t * len) {
    if (in->readlen > in->linelen) {
        in->readlen -= in->linelen;
        memmove(in->buffer, in->buffer + in->linelen, in->readlen);
        in->linelen = 0;
    }
    int hasline = in->type->line(in);
    if (hasline) {
        *len = in->linelen;
        return in->buffer;
    } else
        return NULL;
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
    NULL,
    file_put,
    file_flush,
    NULL,
};

static struct bkd_ostreamdef _bkd_stdout_def = {
    NULL,
    stdout_put,
    file_flush,
    NULL,
};

static struct bkd_ostream _bkd_stdout = {
    &_bkd_stdout_def,
    NULL
};

struct bkd_ostreamdef * BKD_FILE_OSTREAMDEF = &_bkd_file_ostreamdef;
struct bkd_ostream * BKD_STDOUT = &_bkd_stdout;

/* stdio input stream */

static int find_newline(uint8_t * s, size_t nmax) {
    int imax = (int) nmax;
    for (int i = 0; i < imax; i++) {
        if (s[i] == '\n')
            return i;
    }
    return -1;
}

/* Would rather use 'getline', but is not cross platform and uses
 * own memory managment. Also, I think the current implementation
 * could be shorter. */
static int file_getl(struct bkd_istream * in) {
    int newline = find_newline(in->buffer, in->readlen);
    if (newline >= 0) { // We have read all characters on the line.
        in->linelen = newline;
        return 1;
    } else { // We don't have a complete line in the buffer
        FILE * f = (FILE *) in->user;
        while (newline < 0) {
            if (in->buflen <= in->readlen) {
                in->buflen = in->readlen * 2;
                in->buffer = BKD_REALLOC(in->buffer, in->buflen);
            }
            size_t bytestoread = in->buflen - in->readlen;
            size_t read = fread(in->buffer + in->readlen, 1, bytestoread, f);
            in->readlen += read;
            if (read < bytestoread) {
                in->linelen = in->readlen;
                return (in->linelen > 0) || (read > 0);
            } else {
                newline = find_newline(in->buffer + in->readlen, in->buflen - in->readlen);
            }
        }
        in->linelen = newline;
        return 1;
    }
}

static int stdin_getl(struct bkd_istream * in) {
    in->user = stdin;
    if (!in->buffer)
        bkd_istream_init(in->type, in, stdin);
    return file_getl(in);
}

static struct bkd_istreamdef _bkd_file_istreamdef = {
    NULL,
    file_getl,
    NULL
};

static struct bkd_istreamdef _bkd_stdin_istreamdef = {
    NULL,
    stdin_getl,
    NULL
};

static struct bkd_istream _bkd_stdin = {
    &_bkd_stdin_istreamdef,
    NULL
};

struct bkd_istreamdef * BKD_FILE_ISTREAMDEF = &_bkd_file_istreamdef;
struct bkd_istream * BKD_STDIN = &_bkd_stdin;

#endif
