#ifndef BKD_UTF8_
#define BKD_UTF8_

#include <stdint.h>
#include <stddef.h>

size_t bkd_utf8_sizep(uint32_t codepoint);
size_t bkd_utf8_sizeb(uint8_t head);
size_t bkd_utf8_write(uint8_t * s, uint32_t value);
size_t bkd_utf8_read(uint8_t * s, uint32_t * ret);

#endif /* end of include guard: BKD_UTF8_ */
