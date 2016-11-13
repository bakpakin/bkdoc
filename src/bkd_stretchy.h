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

#ifndef BKD_STRETCHY_H_W39GUEUP
#define BKD_STRETCHY_H_W39GUEUP

/* Simple stretchy buffer modified from https://github.com/nothings/stb */
#include <string.h>
#include "bkd.h"

#define bkd_sbfree(a)         ((a) ? BKD_FREE(bkd__sbraw(a)),0 : 0)
#define bkd_sbpush(a,v)       (bkd__sbmaybegrow(a,1), (a)[bkd__sbn(a)++] = (v))
#define bkd_sbcount(a)        ((a) ? bkd__sbn(a) : 0)
#define bkd_sbadd(a,n)        (bkd__sbmaybegrow(a,n), bkd__sbn(a)+=(n), &(a)[bkd__sbn(a)-(n)])
#define bkd_sblast(a)         ((a)[bkd__sbn(a)-1])
#define bkd_sblastp(a)         ((a) + bkd__sbn(a)-1)
#define bkd_sbpop(a)          (--bkd__sbn(a))

#define bkd__sbraw(a) ((int *) (a) - 2)
#define bkd__sbm(a)   bkd__sbraw(a)[0]
#define bkd__sbn(a)   bkd__sbraw(a)[1]

#define bkd__sbneedgrow(a,n)  ((a)==0 || bkd__sbn(a)+(n) >= bkd__sbm(a))
#define bkd__sbmaybegrow(a,n) (bkd__sbneedgrow(a,(n)) ? bkd__sbgrow(a,n) : 0)
#define bkd__sbgrow(a,n)      ((a) = bkd__sbgrowf((a), (n), sizeof(*(a))))

static void * bkd__sbgrowf(void *arr, int increment, int itemsize) {
   int dbl_cur = arr ? 2 * bkd__sbm(arr) : 0;
   int min_needed = bkd_sbcount(arr) + increment;
   int m = dbl_cur > min_needed ? dbl_cur : min_needed;
   int *p = (int *) BKD_REALLOC(arr ? bkd__sbraw(arr) : 0, itemsize * m + sizeof(int)*2);
   if (p) {
      if (!arr)
         p[1] = 0;
      p[0] = m;
      return p+2;
   } else {
      #ifdef STRETCHY_BUFFER_OUT_OF_MEMORY
      STRETCHY_BUFFER_OUT_OF_MEMORY ;
      #endif
      return (void *) (2*sizeof(int)); // try to force a NULL pointer exception later
   }
}

#endif /* end of include guard: BKD_STRETCHY_H_W39GUEUP */
