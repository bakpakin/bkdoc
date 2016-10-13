#include "bkd.h"
#include "bkd_utf8.h"
#include "bkd_string.h"
#include <string.h>

/* Error strings */
const char * bkd_errors[] = {
    NULL,
    "Out of memory.",
    "Invalid markup type.",
    "Unknown node type.",
    "Unknown error."
};

/* Output streams */

int bkd_putn(struct bkd_ostream * out, const struct bkd_string string) {
    return out->type->stream(out, string);
}

int bkd_puts(struct bkd_ostream * out, const char * str) {
    uint32_t size = strlen(str);
    struct bkd_string string = {
        size,
        (uint8_t *) str
    };
    return bkd_putn(out, string);
}

int bkd_putc(struct bkd_ostream * out, char c) {
    struct bkd_string string = {
        1,
        (uint8_t *) &c
    };
    return bkd_putn(out, string);
}

void bkd_flush(struct bkd_ostream * out) {
    if (out->type->flush)
        out->type->flush(out);
}

/* Input streams */

struct bkd_istream * bkd_istream_init(struct bkd_istreamdef * type, struct bkd_istream * stream, void * user) {
    stream->type = type;
    stream->user = user;
    stream->done = 0;
    stream->buffer = bkd_bufnew(80);
    return stream;
}

void bkd_istream_freebuf(struct bkd_istream * in) {
    bkd_buffree(in->buffer);
}

struct bkd_string bkd_getl(struct bkd_istream * in) {
    if (in->done)
        return BKD_NULLSTR;
    in->type->line(in);
    return bkd_lastl(in);
}

struct bkd_string bkd_lastl(struct bkd_istream * in) {
    if (!in->done) {
        return in->buffer.string;
    } else {
        return BKD_NULLSTR;
    }
}

#ifndef BKD_NO_STDIO

/* stdio output stream */

static int file_put_impl(FILE * file, struct bkd_string string) {
    for (uint32_t i = 0; i < string.length; i++)
        fputc(string.data[i], file);
    return 0;
}

static int file_put(struct bkd_ostream * out, struct bkd_string string) {
    return file_put_impl((FILE *) out->user, string);
}

static int file_flush(struct bkd_ostream * out) {
    fflush((FILE *) out->user);
    return 0;
}

static int stdout_put(struct bkd_ostream * out, struct bkd_string string) {
    out->user = stdout;
    return file_put(out, string);
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

/* Switch to buffered reads with fread */
static int file_getl(struct bkd_istream * in) {
    FILE * file = (FILE *) in->user;
    int c;
    in->buffer.string.length = 0; /* Clear th buffer */
    for(;;) {
        c = fgetc(file);
        if(c == EOF) {
            if (in->buffer.string.length == 0) {
                in->done = 1;
                return 0;
            } else
                break;
        }
        if (c == '\n')
            break;
        if (c != '\r')
            in->buffer = bkd_bufpushb(in->buffer, c);
    }
    return 1;
}

static int stdin_getl(struct bkd_istream * in) {
    if (in->user != stdin) {
        bkd_istream_init(in->type, in, stdin);
    }
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
    {
        0, {0, 0}
    },
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
