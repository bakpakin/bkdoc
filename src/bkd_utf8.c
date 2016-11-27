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

#include "bkd_utf8.h"
#include <string.h>

/**
 * Returns the number of bytes that are needed to represent a codepoint in UTF8.
 */
size_t bkd_utf8_sizep(uint32_t codepoint) {
    if (codepoint < 0x80) {
        return 1;
    } else if (codepoint < 0x800) {
        return 2;
    } else if (codepoint < 0x10000) {
        return 3;
    } else if (codepoint < 0x200000) {
        return 4;
    } else {
        return 0;
    }
}

/**
 * Return the number of bytes in a UTF8 character given the first byte.
 */
size_t bkd_utf8_sizeb(uint8_t head) {
    if ((head & 0x80) == 0) {
        return 1;
    } else if ((head & 0xE0) == 0xC0) {
        return 2;
    } else if ((head & 0xF0) == 0xE0) {
        return 3;
    } else if ((head & 0xF8) == 0xF0) {
        return 4;
    } else {
        return 0;
    }
}

/**
 * Write the codepoint to the string s in UTF8 encoding.
 * s should have space for at least four bytes.
 */
size_t bkd_utf8_write(uint8_t * s, uint32_t value) {
    uint8_t octets[4];
    size_t count;
    if (value < 0x80) {
        count = 1;
        octets[0] = value;
    } else if (value < 0x800) {
        count = 2;
        octets[0] = (value >> 6) | 0xC0;
        octets[1] = (value & 0x3F) | 0x80;
    } else if (value < 0x10000) {
        count = 3;
        octets[0] = ((value >> 12) & 0x0F) | 0xE0;
        octets[1] = ((value >> 6) & 0x3F) | 0x80;
        octets[2] = (value & 0x3F) | 0x80;
    } else if (value < 0x200000) {
        count = 4;
        octets[0] = ((value >> 18) & 0x07) | 0xF0;
        octets[1] = ((value >> 12) & 0x3F) | 0x80;
        octets[2] = ((value >> 6) & 0x3F) | 0x80;
        octets[3] = (value & 0x3F) | 0x80;
    } else {
        /* Although longer characters are possible, they are not valid unicode. */
        return 0;
    }
    memcpy(s, octets, count);
    return count;
}

/**
 * Write up to maxlen bytes to strings. Otherwise, is the same as
 * bkd_utf8_write. If maxlen is shorter than the required number of bytes
 * to write the character, then this function will return the required number
 * of bytes.
 */
size_t bkd_utf8_writelen(uint8_t * s, uint32_t value, uint32_t maxlen) {
    size_t size = bkd_utf8_sizep(value);
    if (size <= maxlen)
        return bkd_utf8_write(s, value);
    return size;
}

/**
 * Reads string s into ret. Returns the amount of memory read into. Can be up to 4 bytes.
 */
size_t bkd_utf8_read(uint8_t * s, uint32_t * ret) {
    uint8_t head = *s;
    if ((head & 0x80) == 0) {
        *ret = head;
        return 1;
    } else if ((head & 0xE0) == 0xC0) {
        *ret = (s[1] & 0x3F) + ((head & 0x0F) << 6);
        return 2;
    } else if ((head & 0xF0) == 0xE0) {
        *ret = (s[2] & 0x3F) + ((s[1] & 0x3F) << 6) + ((head & 0x1F) << 12);
        return 3;
    } else if ((head & 0xF8) == 0xF0) {
        *ret = (s[3] & 0x3F) + ((s[2] & 0x3F) << 6) + ((s[1] & 0x3F) << 12) + ((head & 0x07) << 18);
        return 4;
    } else {
        return 0;
    }
}

/**
 * Same as bkd_utf8_read, but will only read maxlen bytes at most. If the character
 * being read is too long, this function will return the length of the current character.
 */
size_t bkd_utf8_readlen(uint8_t * s, uint32_t * ret, uint32_t maxlen) {
    size_t size = bkd_utf8_sizeb(s[0]);
    if (size <= maxlen)
        return bkd_utf8_read(s, ret);
    return size;
}

size_t bkd_utf8_readlenback(uint8_t * s, uint32_t * ret, uint32_t maxback) {
    uint8_t * head = bkd_utf8_findhead(s - 1, s - maxback);
    uint32_t len = s - head;
    return bkd_utf8_readlen(head, ret, len);
}

size_t bkd_utf8_readback(uint8_t * s, uint32_t * ret) {
    return bkd_utf8_readlenback(s, ret, 4);
}

/***
 * Helpers
 */

uint8_t * bkd_utf8_findhead(uint8_t * s, uint8_t * start) {
    while ((*s & 0xC0) == 0x80) {
        if (s < start)
            return NULL;
        --s;
    }
    return s;
}

/**
 * Checks if a unicode codepoint is whitespace.
 */
int bkd_utf8_whitespace(uint32_t codepoint) {
    /* There might be some other unicode to consider later, in the higher regions. */
    return (codepoint > 8 && codepoint < 14) || codepoint == 32;
}
