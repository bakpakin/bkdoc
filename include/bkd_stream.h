#ifndef BKD_STREAM_H_
#define BKD_STREAM_H_

#include <stdint.h>
#include <stddef.h>

/* Simple output streams */
struct bkd_ostream;

struct bkd_ostreamdef {
    int (*stream)(struct bkd_ostream * self, const uint8_t * data, uint32_t size);
    int (*flush)(struct bkd_ostream * self);
};

struct bkd_ostream {
    struct bkd_ostreamdef * type;
    void * user;
};

int bkd_putn(struct bkd_ostream * out, const uint8_t * data, uint32_t size);
int bkd_puts(struct bkd_ostream * out, const char * str);
int bkd_putc(struct bkd_ostream * out, char c);
void bkd_flush(struct bkd_ostream * out);

/* Simple input streams */
struct bkd_istream;

struct bkd_istreamdef {
    int (*line)(struct bkd_istream * self);
};

struct bkd_istream {
    struct bkd_istreamdef * type;
    void * user;
    size_t buflen; /* size of buffer */
    uint8_t * buffer;
    size_t readlen; /* size of characters read in buffer */
    size_t linelen; /* length of first line in buffer */
};

uint8_t * bkd_getl(struct bkd_istream * in, size_t * len);
void bkd_istream_freebuf(struct bkd_istream * in);

extern struct bkd_istreamdef * BKD_STRING_ISTREAMDEF;
extern struct bkd_ostreamdef * BKD_STRING_OSTREAMDEF;
#ifndef BKD_NO_STDIO
#include <stdio.h>
extern struct bkd_istreamdef * BKD_FILE_ISTREAMDEF;
extern struct bkd_istream * BKD_STDIN;
extern struct bkd_ostreamdef * BKD_FILE_OSTREAMDEF;
extern struct bkd_ostream * BKD_STDOUT;
#endif


#endif /* end of include guard: BKD_STREAM_H_ */
