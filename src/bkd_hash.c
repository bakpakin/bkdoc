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

#include "bkd_hash.h"
#include "bkd_string.h"

int bkd_hashinit(struct bkd_hashtable * htable, uint8_t capacityPower) {
    if (capacityPower > 30) {
        return 1; /* Too much space! */
    }
    htable->capacity = 1 << capacityPower;
    htable->hashMask = htable->capacity - 1;
    htable->count = 0;
    htable->buckets = BKD_CALLOC(htable->capacity * sizeof(struct bkd_hashbucket), 0);
    return !htable->buckets;
}

void bkd_hashfree(struct bkd_hashtable * htable) {
    /* Free all of the children first */
    struct bkd_hashbucket * current = htable->buckets;
    struct bkd_hashbucket * lastBucket = htable->buckets + htable->capacity;
    while (current < lastBucket) {
        if (current->activated) {
            /* Free the chain */
            struct bkd_hashbucket * hb = current, * prev = current;
            while (hb) {
                if (hb->ownsKey)
                    bkd_strfree(hb->key);
                if (hb->ownsValue)
                    bkd_strfree(hb->value);
                prev = hb;
                hb = hb->next;
                if (prev != current) {
                    BKD_FREE(prev);
                }
            }
        }
    }
    BKD_FREE(htable->buckets);
}

struct bkd_hashbucket * bkd_hashgetb(struct bkd_hashtable * htable, struct bkd_string key) {
    uint32_t index = bkd_strhash(key) & htable->hashMask;
    struct bkd_hashbucket * bucket = htable->buckets + index;
    if (!bucket->activated) /* Only the first bucket needs to be explicitely activated. */
        return NULL;
    while (bucket) {
        if (bkd_strequal(bucket->key, key)) {
            return bucket;
        }
        bucket = bucket->next;
    }
    return NULL;
}

int bkd_hashget(struct bkd_hashtable * htable, struct bkd_string key, struct bkd_string * out) {
    struct bkd_hashbucket * bucket = bkd_hashgetb(htable, key);
    if (bucket) {
        *out = bucket->key;
        return 1;
    } else {
        return 0;
    }
}

/* Readds a bucket to the table. Returns if the bucket itself was reused. If not, the
 * bucket can be freed. Does not need to check if key is in table. */
int bkd_hashput_reuse(struct bkd_hashtable * htable, struct bkd_hashbucket * bucket) {
    uint32_t index = bkd_strhash(bucket->key) & htable->hashMask;
    struct bkd_hashbucket * startbucket = htable->buckets + index;
    if (startbucket->activated) {
        /* Resolve collision */
        struct bkd_hashbucket * next = startbucket->next;
        startbucket->next = bucket;
        bucket->next = next;
        return 1;
    } else {
        /* We have no collision. */
        *startbucket = *bucket;
        startbucket->next = NULL;
        startbucket->activated = 1;
        return 0;
    }
}

/* Readds a non-malloced bucket to the table. */
void bkd_hashput_head(struct bkd_hashtable * htable, struct bkd_hashbucket * bucket) {
    uint32_t index = bkd_strhash(bucket->key) & htable->hashMask;
    struct bkd_hashbucket * startbucket = htable->buckets + index;
    if (startbucket->activated) {
        /* Resolve collision */
        struct bkd_hashbucket * next = startbucket->next;
        struct bkd_hashbucket * newbucket = BKD_MALLOC(sizeof(struct bkd_hashbucket));
        *newbucket = *bucket; /* copy the bucket */
        startbucket->next = newbucket;
        newbucket->next = next;
    } else {
        /* We have no collision. */
        *startbucket = *bucket;
        startbucket->next = NULL;
        startbucket->activated = 1;
    }
}

int resize(struct bkd_hashtable * htable) {
    uint32_t oldCapacity = htable->capacity;
    uint32_t newCapacity = oldCapacity * 2;
    struct bkd_hashbucket * oldBuckets = htable->buckets;
    struct bkd_hashbucket * newBuckets = BKD_CALLOC(sizeof(struct bkd_hashbucket) * htable->capacity, 0);
    if (!newBuckets) {
        return 1;
    }
    htable->capacity = newCapacity;
    htable->buckets = newBuckets;
    struct bkd_hashbucket * bucket = oldBuckets, * lastBucket = bucket + oldCapacity;
    while (bucket < lastBucket) {
        if (bucket->activated) {
            bkd_hashput_head(htable, bucket);
            struct bkd_hashbucket * head = bucket->next, * tmp;
            while(head) {
                int dontFree = bkd_hashput_reuse(htable, head);
                tmp = head;
                head = head->next;
                if (!dontFree)
                    BKD_FREE(tmp);
            }
        }
        bucket++;
    }
    BKD_FREE(oldBuckets);
    return 0;
}

int bkd_hashput(struct bkd_hashtable * htable, struct bkd_string key, struct bkd_string value) {
    if (htable->count >= htable->capacity)
        if (resize(htable)) {
            return 1;
        }
    uint32_t index = bkd_strhash(key) & htable->hashMask;
    struct bkd_hashbucket * bucket = htable->buckets + index;
    if (bucket->activated) {
        /* Resolve collision */
        while (bucket) {
            if (bkd_strequal(key, bucket->key)) { /* We found our key */
                break;
            } else {
                bucket = bucket->next;
            }
        }
        if (bucket) {
            /* We reuse an in-use bucket */
            if (bucket->ownsValue)
                bkd_strfree(bucket->value);
            bucket->ownsValue = 1;
            bucket->value = bkd_str_new(value);
        } else {
            /* We could not find the key in the collision */
            htable->count++;
            bucket->next = BKD_MALLOC(sizeof(struct bkd_hashbucket));
            bucket = bucket->next;
            bucket->ownsKey = 1;
            bucket->ownsValue = 1;
            bucket->key = bkd_str_new(key);
            bucket->value = bkd_str_new(value);
            bucket->next = NULL;
        }
    } else {
        /* We have no collision. */
        htable->count++;
        bucket->ownsKey = 1;
        bucket->ownsValue = 1;
        bucket->key = bkd_str_new(key);
        bucket->value = bkd_str_new(value);
        bucket->next = NULL;
    }
    return 0;
}
