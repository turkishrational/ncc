/* string.cc - immutable string table              ncc, the new c/c++ compiler

                    Copyright (c) 2021 Charles E. Youse (charles@gnuless.org).
                    All rights reserved. See LICENSE file for more details. */

#include <string.h>
#include <limits.h>
#include <sys/mman.h>
#include "cc1.h"
#include "string.h"

slab string::drop_slab(sizeof(struct drop), 100);

string string::none;                    /* see init() */
string::drop string::empty_drop;        /* all zeroes -> null string */

/* the empty string is statically
   pre-inserted into the hash table */

string::drop *string::buckets[NR_BUCKETS] = { &empty_drop };
unsigned long string::filters[NR_BUCKETS][NR_FILTERS] = { 1 };

char *string::pool_text;
char *string::pool_pos;


void string::init()
{
    /* map in an anonymous region
       for the string pool */

    void *addr;

    addr = mmap(0, POOL_SIZE, PROT_READ | PROT_WRITE,
                MAP_ANON | MAP_PRIVATE, -1, 0);

    if (addr == MAP_FAILED)
        error(SYSTEM, "can't map string pool %E");

    pool_text = pool_pos = (char *) addr;

    /* because string must be trivial, we can't declare any
       constructors and so must manually initialize none. we
       need not do this for empty_drop as all zeros is correct. */

    none._drop = &empty_drop;

    /* seed keywords. the anal retentive might say these should be
       pre-seeded rather than seeding at runtime; it's a good point. */

    for (int i = 0; i < nr_keywords; ++i) {
        string s = lookup(keywords[i].text, strlen(keywords[i].text));
        s._drop->k = keywords[i].k;
    }
}


/* this is the de-facto constructor. returns the string associated with the
   specified text buffer. if in_pool is true, then we're trying to commit a
   pool string- lookup() will update the pool pointers accordingly. */

string string::lookup(const char *text, size_t len, bool in_pool)
{
    struct drop *d = 0;
    unsigned hash = 0;
    int b;  /* bucket # */
    int f;  /* filter word */
    int n;  /* bit position in filter word */

    /* a user could, conceivably, give us input with an identifier or string
       literal that exceeds 2GB in length... unlikely, but we should check. */

    if (len > INT_MAX)
        error(INTERNAL, "string overflow");

    /* the astute observer will notice that this hash is merely
       a multiplication (by 15) followed by an xor. this is not
       especially amazing, and could probably be replaced. */

    for (int i = 0; i < len; ++i)
        hash = ((hash << 3) + (hash << 2) + (hash << 1)) ^ (text[i] & 0xff);

    b = hash & (NR_BUCKETS - 1);
    f = (hash >> LOG2_NR_BUCKETS) & (NR_FILTERS - 1);
    n = (hash >> (LOG2_NR_BUCKETS + LOG2_NR_FILTERS)) & (BITS_PER_WORD - 1);

    if (filters[b][f] & (1L << n)) {
        for (struct drop **p = &buckets[b]; d = *p; p = &d->link) {
            if ((d->hash == hash) && (d->len == len)
              && (memcmp(d->text, text, len) == 0)) {
                /* match; remove temporarily from bucket.
                   it will be put back in front shortly. */

                *p = d->link;

                if (in_pool) /* chuck it */
                    pool_pos = pool_text;

                break;
            }
        }
    }

    if (d == 0) {
        d = new struct drop;

        d->hash = hash;
        d->len = len;
        d->text = text;
        d->k = K_NONE;
        d->label = asm_label::none;

        if (in_pool) /* keep it */
            pool_text = pool_pos;

        filters[b][f] |= (1L << n);
    }

    d->link = buckets[b];
    buckets[b] = d;

    string ret;
    ret._drop = d;
    return ret;
}


/* emit the contents of a drop to the assembly output. we emit exactly
   n bytes in groups of 8, and pad with trailing zeroes if needed. */

void string::drop::output(size_t n)
{
    unsigned char c;

    for (size_t i = 0; i < n; ++i) {
        if (i < len)
            c = text[i];
        else
            c = 0;

        if ((i % 8) == 0) {
            if (i)
                ::output("\n");

            ::output("\t.byte ");
        } else
            ::output(",");

        ::output("%d", c);
    }

    ::output("\n");
}


void string::literals()
{
    for (int b = 0; b < NR_BUCKETS; ++b)
        for (struct drop *d = buckets[b]; d; d = d->link)
            if (d->label != asm_label::none) {
                /* XXX segment(SEG_TEXT); */
                ::output("%L:\n", d->label);
                d->output(d->len + 1);
            }
}


/* vi: set ts=4 expandtab: */
