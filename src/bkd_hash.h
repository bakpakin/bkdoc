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

#ifndef BKD_HASH_H_6CU79XSU
#define BKD_HASH_H_6CU79XSU

#include "bkd.h"

struct bkd_hashbucket {
    uint8_t activated: 1;
    uint8_t ownsKey: 1;
    uint8_t ownsValue: 1;
    struct bkd_string key;
    struct bkd_string value;
    struct bkd_hashbucket *next;
};

struct bkd_hashtable {
    uint32_t count;
    uint32_t capacity;
    uint32_t hashMask;
    struct bkd_hashbucket * buckets;
};

int bkd_hashinit(struct bkd_hashtable *, uint8_t capacityPower);

void bkd_hashfree(struct bkd_hashtable *);

struct bkd_hashbucket * bkd_hashgetb(struct bkd_hashtable *, struct bkd_string key);

int bkd_hashget(struct bkd_hashtable * htable, struct bkd_string key, struct bkd_string * out);

int bkd_hashput(struct bkd_hashtable *, struct bkd_string key, struct bkd_string value);

#endif /* end of include guard: BKD_HASH_H_6CU79XSU */
