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

#include "bkd.h"
#include "bkd_utf8.h"
#include "bkd_string.h"

#include <string.h>

static inline uint32_t getSubIndex(int32_t i, uint32_t len) {
    if (i < 0) {
        return len + i > 0 ? len + i : 0;
    } else {
        return ((uint32_t) i) > len ? len : (uint32_t) i;
    }
}

/**
 * Returns a substring in constant time (no allocation). Uses negative indexing for end of string. Includes index1 and index2.
 */
struct bkd_string bkd_strsub(struct bkd_string string, int32_t index1, int32_t index2) {
    uint32_t realIndex1 = getSubIndex(index1, string.length);
    uint32_t realIndex2 = getSubIndex(index2, string.length);
    struct bkd_string ret;
    if (realIndex2 >= realIndex1) {
        ret.length = realIndex2 - realIndex1 + 1;
        ret.data = string.data + realIndex1;
    } else {
        ret = BKD_NULLSTR;
    }
    return ret;
}

struct bkd_string bkd_cstr(const char * cstr) {
    uint32_t len = (uint32_t) strlen(cstr);
    struct bkd_string ret;
    ret.data = (uint8_t *) cstr;
    ret.length = len;
    return ret;
}

struct bkd_string bkd_cstr_new(const char * cstr) {
    uint32_t len = (uint32_t) strlen(cstr);
    struct bkd_string ret;
    if (len > 0) {
        ret.data = BKD_MALLOC(len);
    } else {
        return BKD_NULLSTR;
    }
    memcpy(ret.data, cstr, len);
    ret.length = len;
    return ret;
}

struct bkd_string bkd_str_new(struct bkd_string string) {
    struct bkd_string ret;
    ret.data = BKD_MALLOC(string.length);
    ret.length = string.length;
    memcpy(ret.data, string.data, string.length);
    return ret;
}

struct bkd_string bkd_strsub_new(struct bkd_string string, int32_t index1, int32_t index2) {
    struct bkd_string sub = bkd_strsub(string, index1, index2);
    return bkd_str_new(sub);
}

struct bkd_string bkd_strconcat_new(struct bkd_string str1, struct bkd_string str2) {
    uint32_t totalLength = str1.length + str2.length;
    struct bkd_string ret;
    ret.length = totalLength;
    ret.data = BKD_MALLOC(totalLength);
    memcpy(ret.data, str1.data, str1.length);
    memcpy(ret.data + str1.length, str2.data, str2.length);
    return ret;
}

int bkd_strfind(struct bkd_string string, uint32_t codepoint, uint32_t * index) {
    uint32_t pos = 0;
    uint32_t testpoint = 0;
    uint32_t charsize;
    while (pos < string.length) {
        charsize = bkd_utf8_readlen(string.data + pos, &testpoint, string.length - pos);
        if (testpoint == codepoint) {
            *index = pos;
            return 1;
        }
        pos += charsize;
    }
    return 0;
}

uint32_t bkd_strindent(struct bkd_string string) {
    uint32_t indent = 0;
    uint32_t pos = 0;
    uint32_t codepoint = 0;
    while (pos < string.length) {
        if (string.data[pos] == ' ') {
            indent += 1;
            pos++;
        } else if (string.data[pos] == '\t') {
            indent += 4;
            pos++;
        } else {
            pos += bkd_utf8_readlen(string.data + pos, &codepoint, string.length - pos);
            if (!bkd_utf8_whitespace(codepoint)) {
                break;
            }
        }
    }
    return indent;
}

int bkd_strequal(struct bkd_string str1, struct bkd_string str2) {
    uint8_t * c1, * c2, * c1end;;
    if (str1.length != str2.length) return 0;
    if (str1.data == str2.data) return 1;
    for (c1 = str1.data, c2 = str2.data, c1end = c1 + str1.length; c1 < c1end; c1++, c2++) {
        if (*c1 != *c2)
            return 0;
    }
    return 1;
}

int bkd_strempty(struct bkd_string string) {
    uint32_t pos = 0;
    uint32_t codepoint = 0;
    while (pos < string.length) {
        pos += bkd_utf8_readlen(string.data + pos, &codepoint, string.length - pos);
        if (!bkd_utf8_whitespace(codepoint)) {
            return 0;
        }
    }
    return 1;
}

uint32_t bkd_strhash(struct bkd_string string) {
    uint8_t * data = string.data;
    uint32_t hash = 5381;
    uint32_t c;
    while ((c = *data++))
        hash = 33 * hash + c;
    return hash;
}

struct bkd_string bkd_strstripn_new(struct bkd_string string, uint32_t n) {
    struct bkd_string ret;
    uint32_t leading = 0;
    uint32_t pos = 0;
    uint32_t padding = 0;
    uint32_t codepoint = 0;
    if (string.length < 1) return string;
    while (leading < n && pos < string.length) {
        pos += bkd_utf8_readlen(string.data + pos, &codepoint, string.length - pos);
        if (codepoint == '\t') {
            leading += 4;
        } else {
            leading++;
        }
    }
    if (leading < n) {
        return bkd_cstr_new("");
    }
    padding = leading - n;
    uint32_t newlen = string.length - pos + padding;
    if (newlen == 0) {
        return bkd_cstr_new("");
    }
    ret.length = newlen;
    ret.data = BKD_MALLOC(newlen);
    for (uint32_t i = 0; i < padding; i++)
        ret.data[i] = ' ';
    memcpy(ret.data + padding, string.data + pos, newlen - padding);
    return ret;
}

struct bkd_string bkd_strtrim(struct bkd_string string, int front, int back) {
    uint8_t * head = string.data;
    uint8_t * tail = head + string.length;
    uint32_t codepoint;
    uint32_t charsize;
    if (front) {
        while (head < tail) {
            charsize = bkd_utf8_readlen(head, &codepoint, tail - head);
            if (!bkd_utf8_whitespace(codepoint))
                break;
            head += charsize;
        }
    }
    if (back) {
        while (head < tail) {
            charsize = bkd_utf8_readlenback(tail, &codepoint, tail - head);
            if (!bkd_utf8_whitespace(codepoint))
                break;
            tail += charsize;
        }
    }
    string.data = head;
    string.length = tail - head;
    return string;
}

struct bkd_string bkd_strtrimc(struct bkd_string string, uint32_t codepoint, int front, int back) {
    uint8_t * head = string.data;
    uint8_t * tail = head + string.length;
    uint32_t testpoint;
    uint32_t charsize;
    if (front) {
        while (head < tail) {
            charsize = bkd_utf8_readlen(head, &testpoint, tail - head);
            if (testpoint != codepoint)
                break;
            head += charsize;
        }
    }
    if (back) {
        while (head < tail) {
            charsize = bkd_utf8_readlenback(tail, &testpoint, tail - head);
            if (testpoint != codepoint)
                break;
            tail += charsize;
        }
    }
    string.data = head;
    string.length = tail - head;
    return string;
}

void bkd_strfree(struct bkd_string string) {
    if (string.data) {
        BKD_FREE(string.data);
    }
}

/*
 * BUFFERS
 */

struct bkd_buffer bkd_bufnew(uint32_t capacity) {
    struct bkd_buffer ret;
    ret.capacity = capacity;
    ret.string.data = BKD_MALLOC(capacity);
    ret.string.length = 0;
    return ret;
}

void bkd_buffree(struct bkd_buffer buffer) {
    bkd_strfree(buffer.string);
}

struct bkd_buffer bkd_bufpush(struct bkd_buffer buffer, struct bkd_string string) {
    uint32_t newLength = buffer.string.length + string.length;
    if (buffer.capacity < newLength) {
        buffer.capacity = 1.5 * newLength + 1;
        buffer.string.data = BKD_REALLOC(buffer.string.data, buffer.capacity);
    }
    memcpy(buffer.string.data + buffer.string.length, string.data, string.length);
    buffer.string.length = newLength;
    return buffer;
}

struct bkd_buffer bkd_bufpushc(struct bkd_buffer buffer, uint32_t codepoint) {
    uint32_t csize = bkd_utf8_sizep(codepoint);
    uint32_t newLength = buffer.string.length + csize;
    if (buffer.capacity < newLength) {
        buffer.capacity = 1.5 * newLength + 1;
        buffer.string.data = BKD_REALLOC(buffer.string.data, buffer.capacity);
    }
    bkd_utf8_write(buffer.string.data + buffer.string.length, codepoint);
    buffer.string.length = newLength;
    return buffer;
}

struct bkd_buffer bkd_bufpushb(struct bkd_buffer buffer, uint8_t byte) {
    buffer.string.length++;
    if (buffer.capacity < buffer.string.length) {
        buffer.capacity = 1.5 * buffer.string.length + 1;
        buffer.string.data = BKD_REALLOC(buffer.string.data, buffer.capacity);
    }
    buffer.string.data[buffer.string.length - 1] = byte;;
    return buffer;
}
