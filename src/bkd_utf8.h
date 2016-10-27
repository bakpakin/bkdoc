/*
Copyright (c) 2016 Calvin Rose

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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
