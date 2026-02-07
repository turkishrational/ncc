/* string.h - immutable string table               ncc, the new c/c++ compiler

                    Copyright (c) 2021 Charles E. Youse (charles@gnuless.org).
                    All rights reserved. See LICENSE file for more details. */

#ifndef STRING_H
#define STRING_H

#include "token.h"

/* every string encountered by the lexer- identifiers/keywords and string
   literals- go into the immutable string table, with exactly one entry in
   the same for each unique string. this eases memory management, and more
   importantly, speeds comparisons for equality, since two strings have the
   same contents iff they reference the same table entry.

   the table is organized as a hash in NR_BUCKETS buckets. each bucket is
   kept in most-recently-used order, and guarded by a sort of Bloom-style
   filter to mitigate the cost of the initial hash lookups.

   class string must be trivial, as it may be a union member. this is
   only a nuisance in that we can't automatically initialize a string. */

class string
{
    friend class name;

    static const int LOG2_NR_BUCKETS       = 8;    /* 256 buckets */
    static const int LOG2_NR_FILTERS       = 3;    /* 8 filter words */
    static const int LOG2_BITS_PER_WORD    = 6;    /* 64 bits per word */

    static const int NR_BUCKETS            = (1 << LOG2_NR_BUCKETS);
    static const int NR_FILTERS            = (1 << LOG2_NR_FILTERS);
    static const int BITS_PER_WORD         = (1 << LOG2_BITS_PER_WORD);

    /* each string is represented by a drop (drops in buckets, get it?). the
       text field points to an [external] static buffer, not NUL terminated.
       we use an int for the len, as it plays nice with stdio formats.

       if k is not K_NONE, then the text represents a special identifier.

       if the label is assigned, then the text was used as an anonymous string
       literal, and must be output after compilation- see literals(). */

    static slab drop_slab;

    struct drop
    {
        unsigned hash;
        int len;
        const char *text;
        token k;
        asm_label label;
        struct drop *link;

        void *operator new(size_t) { return drop_slab.alloc(); }
        void output(size_t n);
    };

    static struct drop *buckets[NR_BUCKETS];
    static unsigned long filters[NR_BUCKETS][NR_FILTERS];

    static string lookup(const char *text, size_t len, bool in_pool);

    static struct drop empty_drop;      /* for empty string */

    /* we allocate a giant pool (of POOL_SIZE bytes) at initialization to
       allow clients to build a string a piece at a time before committing
       it to the table (see putc() et al. below). the size of the pool is
       fixed, but tuneable. we rely on the size being "large enough" and do
       not bother verifying that we stay within bounds, instead relying on
       the system to trap an invalid access beyond the mmap'd region. */

    enum { POOL_SIZE = (1 << 30) /* 1GB */ };

    static char *pool_text;     /* beginning of current pool string */
    static char *pool_pos;      /* next free position in pool */

    /* this is the only data member of a string.
       the friend class name relies on this layout. */

    struct drop *_drop;

public:
    static string none;         /* empty string; points to empty_drop */

    bool operator ==(const string& rhs) const { return _drop == rhs._drop; }
    bool operator !=(const string& rhs) const { return _drop != rhs._drop; }
    token k() const { return _drop->k; }

    /* output string contents to specified stdio file. this
       will not work properly on strings with embedded NULs. */

    void print(FILE *fp) const
    {
        fprintf(stderr, "%.*s", _drop->len, _drop->text);
    }

    /* output assembly .byte directives for the first
       n bytes of the string. if n exceeds the string
       length, pad with zeroes. */

    void output(size_t n) { _drop->output(n); }

    /* return the label associated with this string,
       assigning one first, if necessary. */

    asm_label label()
    {
        if (_drop->label == asm_label::none)
            _drop->label.assign();

        return _drop->label;
    }

    /* initialization. call early from
       main(), before using strings. */

    static void init();

    /* return the string associated with the specified text */

    static string lookup(const char *text, size_t len)
    {
        return lookup(text, len, false);
    }

    /* pool functions. call putc() repeatedly to append to the current
       pool string. then call either preserve() to commit the string to
       the table, or discard() to forget about it. */

    static void putc(char c) { *pool_pos++ = c; }
    static void discard() { pool_pos = pool_text; }

    static string preserve()
    {
        return lookup(pool_text, pool_pos - pool_text, true);
    }

    /* call near end of compilation to dump string
       literals to the output assembly file */

    static void literals();
};

#endif /* STRING_H */

/* vi: set ts=4 expandtab: */
