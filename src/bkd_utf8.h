#ifndef BKD_UTF8_
#define BKD_UTF8_

#include <stdint.h>
#include <stddef.h>

#define BKD_UTF8_MAXCODEPOINT 0x10FFFF

uint8_t * bkd_utf8_findhead(uint8_t * s, uint8_t * start);
size_t bkd_utf8_sizep(uint32_t codepoint);
size_t bkd_utf8_sizeb(uint8_t head);
size_t bkd_utf8_write(uint8_t * s, uint32_t value);
size_t bkd_utf8_writelen(uint8_t * s, uint32_t value, uint32_t maxlen);
size_t bkd_utf8_read(uint8_t * s, uint32_t * ret);
size_t bkd_utf8_readlen(uint8_t * s, uint32_t * ret, uint32_t maxlen);
size_t bkd_utf8_readlenback(uint8_t * s, uint32_t * ret, uint32_t maxback);
size_t bkd_utf8_readback(uint8_t * s, uint32_t * ret);

/* Helper functions */

int bkd_utf8_whitespace(uint32_t codepoint);

#endif /* end of include guard: BKD_UTF8_ */
