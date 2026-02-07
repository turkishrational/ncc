/* cc1.h - compiler pass 1                         ncc, the new c/c++ compiler

                    Copyright (c) 2021 Charles E. Youse (charles@gnuless.org).
                    All rights reserved. See LICENSE file for more details. */

#ifndef CC1_H
#define CC1_H

#include <stdio.h>

#define ARRAY_SIZE(a)       (sizeof(a) / sizeof(*(a)))

/* ... should not require explanation ... */

const int BITS_PER_BYTE = 8;

/* POSIX introduced sysconf() in 2001 to query this value at runtime,
   but any sane POSIX system on AMD64 will employ 4k pages. */

const int PAGE_SIZE = 4096;

/* maximum size of a single type or object. we impose a limit well below
   the theoretical maximum (2^64 bytes) to avoid overflows. specifically,
   we choose MAX_SIZE such that MAX_SIZE * BITS_PER_BYTE fits in an int.
   this implies, of course, that MAX_SIZE itself fits in an int, and also
   that MAX_SIZE * MAX_SIZE * BITS_PER_BYTE fits into a long. we make use
   of all these properties.

   ANSI C89 and C99 minimums are ~32k and ~64k respectively. we're fine. */

const int MAX_SIZE = ((256 * 1024 * 1024) - 1);     /* 256Mb - 1 */

/* maximum number of formal arguments permitted. ANSI C89 5.2.4.1 says 31.
   note that type::bits (see type.h) relies on this value not exceeding 31. */

const int MAX_FORMALS = 31;

/* compiling in C++ mode */

extern bool cxx_mode;

/* write to output assembly file
   using printf()-style format */

void output(const char *fmt, ...);

/* ERRORs indicate syntactic or semantic issues in bad user source code.

   SYSTEM errors originate from the operating system, e.g., out of memory
   or file not found, even if they might arise from erroneous input.

   INTERNALs occur when architectural limitations of the compiler are reached.

   ASSERTIONs come from the ASSERT() macro, and so only occur in debug builds.

   WARNINGs are issued only when requested; legal code should always compile
   silently, without warnings, even if it is questionable.

   all non-warnings errors are fatal. we attempt no error recovery.
   don't reorder the enumeration- error() relies on the values. */

enum error_level { WARNING, ERROR, SYSTEM, INTERNAL, ASSERTION };

void error(error_level level, const char *fmt, ...);

#ifdef DEBUG
    #define ASSERT(x)                                                       \
        do {                                                                \
            if (!(x))                                                       \
                error(ASSERTION, "'%s' (%s %d)", #x, __FILE__, __LINE__);   \
        } while (0)
#else
    #define ASSERT(x)
#endif

/* unsurprisingly, assembler labels are derived from an ascending sequence
   of integers. this class must be trivial to accommodate some clients. */

class asm_label
{
    unsigned _num;

public:
    static asm_label none;      /* the value of an unassigned label */

    bool operator ==(const asm_label& rhs) const { return _num == rhs._num; }
    bool operator !=(const asm_label& rhs) const { return _num != rhs._num; }

    /* assigns this label the next in the sequence */

    void assign();

    /* prints the label to fp in a format
       suitable for consumption by the assembler. */

    void print(FILE *fp) const { fprintf(fp, "L%u", _num); }
};

/* slab allocation is used for small objects or those with high churn rates
   (or both). unused objects are kept on a stack, so the most-recently-freed
   objects will be the next allocated, an attempt to keep the cache hot. the
   first 8 bytes of unused objects are repurposed as stack pointers, placing
   a lower bound on the size of a slab object. */

class slab
{
    int _obj_size;
    int _per_slab;
    struct slab_obj { struct slab_obj *next; } *_stack;

    void refill();

public:
    slab(size_t obj_size, int per_slab)
        : _obj_size(obj_size),
          _per_slab(per_slab),
          _stack(0) {}

    /* n.b.: default destructor. we don't do enough bookkeeping to release
       any storage back to the system, so slabs should always be static. */

    /* alloc() and free() work as one would expect. these are
       inlined as they are simple pointer manipulations. only
       in rare circumstances (<1% of the time) do we refill(). */

    void *alloc()
    {
        struct slab_obj *obj;

        if (_stack == 0)
            refill();

        obj = _stack;
        _stack = _stack->next;

        return obj;
    }

    void free(void *p)
    {
        struct slab_obj *obj = (struct slab_obj *) p;

        obj->next = _stack;
        _stack = obj;
    }
};

/* type-preserving bitwise operators for enums. i'll take "things that
   still lack a satisfying solution in modern C++" for $200, alex. */

#define ENUM_BITSET(ENUM)                               \
                                                        \
    inline ENUM operator &(ENUM lhs, ENUM rhs)          \
    { return (ENUM) (((int) lhs) & ((int) rhs)); }      \
                                                        \
    inline ENUM& operator &=(ENUM& lhs, ENUM rhs)       \
    { lhs = lhs & rhs; return lhs; }                    \
                                                        \
    inline ENUM operator |(ENUM lhs, ENUM rhs)          \
    { return (ENUM) (((int) lhs) | ((int) rhs)); }      \
                                                        \
    inline ENUM& operator |=(ENUM& lhs, ENUM rhs)       \
    { lhs = lhs | rhs; return lhs; }                    \
                                                        \
    inline ENUM operator ~(ENUM operand)                \
    { return (ENUM) ~((int) operand); }

#endif /* CC1_H */

/* vi: set ts=4 expandtab: */
