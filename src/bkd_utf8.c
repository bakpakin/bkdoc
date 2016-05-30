#include "bkd_utf8.h"
#include <string.h>

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
        return 0;
    }
    memcpy(s, octets, count);
    return count;
}

size_t bkd_utf8_writelen(uint8_t * s, uint32_t value, uint32_t maxlen) {
    size_t size = bkd_utf8_sizep(value);
    if (size <= maxlen)
        return bkd_utf8_write(s, value);
    return size;
}

size_t bkd_utf8_read(uint8_t * s, uint32_t * ret) {
    uint8_t head = *s;
    if ((head & 0x80) == 0) {
        *ret = head;
        return 1;
    } else if ((head & 0xE0) == 0xC0) {
        *ret = (s[1] & 0x3F) + (head & 0x1F) * 0x40;
        return 2;
    } else if ((head & 0xF0) == 0xE0) {
        *ret = (s[2] & 0x3F) + (s[1] & 0x3F) * 0x20 + (head & 0x0F) * 0x800;
        return 3;
    } else if ((head & 0xF8) == 0xF0) {
        *ret = (s[3] & 0x3F) + (s[2] & 0x3F) * 0x10 + (s[1] & 0x3F) * 0x400 + (head & 0x07) * 0x10000;
        return 4;
    } else {
        return 0;
    }
}

size_t bkd_utf8_readlen(uint8_t * s, uint32_t * ret, uint32_t maxlen) {
    size_t size = bkd_utf8_sizeb(s[0]);
    if (size <= maxlen)
        return bkd_utf8_read(s, ret);
    return size;
}
