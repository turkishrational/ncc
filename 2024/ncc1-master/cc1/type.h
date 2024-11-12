/* type.h - type representation                    ncc, the new c/c++ compiler

                    Copyright (c) 2021 Charles E. Youse (charles@gnuless.org).
                    All rights reserved. See LICENSE file for more details. */

#ifndef TYPE_H
#define TYPE_H

#include "token.h"
#include "string.h"
#include "list.h"
#include "stack.h"

class symbol;

class type
{
    static slab node_slab;

public:
    /* fundamental type representation. variables of type enum bits are
       usually called ts. in an actual type, the base bits are mutually
       exclusive, but they can be combined to represent classes of types.

       the base bits are positioned specifically to make the logic handling
       the "usual arithmetic conversions" simpler. the order also impacts
       the indexing of tables in the type debugging code. see base_index(). */

    enum bits
    {
        T_NONE,

        T_VOID      = 0x00000001,
        T_BOOL      = 0x00000002,
        T_CHAR      = 0x00000004,
        T_SCHAR     = 0x00000008,
        T_UCHAR     = 0x00000010,
        T_SHORT     = 0x00000020,
        T_USHORT    = 0x00000040,
        T_INT       = 0x00000080,
        T_UINT      = 0x00000100,
        T_LONG      = 0x00000200,
        T_ULONG     = 0x00000400,
        T_FLOAT     = 0x00000800,
        T_DOUBLE    = 0x00001000,
        T_LDOUBLE   = 0x00002000,
        T_TAG       = 0x00004000,
        T_ARRAY     = 0x00008000,
        T_FUNC      = 0x00010000,
        T_PTR       = 0x00020000,

        T_BASE_MASK = 0x0003FFFF,

        /* general qualifiers */

        T_VOLATILE  = 0x00040000,
        T_CONST     = 0x00080000,

        T_QUAL_MASK = 0x000C0000,

        /* additional bits for T_FUNC types. the argument configuration
           of a function is part of its type- how many arguments it takes,
           whether it is variadic, etc. T_OLD_STYLE functions nominally
           take no arguments arguments and can not be T_VARIADIC. */

        T_N_MASK    = 0x01F00000,       /* max 31 formals, MAX_ARGS (cc1.h) */
        T_N_SHIFT   = 20,               /* see set_n()/get_n() functions */

        T_VARIADIC  = 0x02000000,       /* variadic arguments */
        T_OLD_STYLE = 0x04000000,       /* nothing known about arguments */

        T_FUNC_MASK = 0x06000000,

        /* prefix nodes are unique nodes that form type prefixes */

        T_PREFIX    = 0x80000000        /* not in forest */
    };

    /* determine the positional index
       of the base bit of ts. */

    static inline int base_index(bits ts) { return __builtin_ctz(ts); }

    /* formal argument to a T_FUNC node.
       defined separately below. */

    class formal;

    /* a type is represented by a sequence of nodes. nodes that comprise
       complete types live in a forest, such that traversing a type from
       its beginning to its end is a traversal towards the root of a tree
       in this forest. nodes in the forest are hashed for speedy lookups,
       and reference-counted so they can be garbage-collected periodically.

       note that nodes that are part of prefixes, which are partial types
       used during the type-building process, are NOT in the forest. */

    class node
    {
        friend class type;

        bits _ts;
        unsigned _refs;
        node *_next;

        union
        {
            formal *_formals;   /* T_FUNC: formal arguments */
            symbol *_tag;       /* T_TAG: struct/union/class/enum symbol */
            int _nelem;     /* T_ARRAY: number of elements, 0 == unbounded */

            struct
            {
                bool _field : 1;    /* true if bit field */
                int _width : 7;     /* width (in bits) 1 .. 64 */
                int _shift : 6;     /* position (in bits from lsb), 0 .. 63 */
            };

            /* u overlaps all the above, and exists to make general
               manipulation simple- e.g., initialization and hashing.
               there is much type punning here; purists, look away. */

            long _u;
        };

        list<node>::entry _links;   /* in forest hash */

        node(bits ts, long u, node *next);
        ~node();

        void *operator new(size_t) { return node_slab.alloc(); }
        void operator delete(void *p) { node_slab.free(p); }

        void inc() { ++_refs; ASSERT(_refs); }
        void dec() { ASSERT(_refs); --_refs; }

        formal& access(int n) const;

    public:
        /* formals of a T_FUNC node are accessed by index like an array. */

        formal& operator [](int n) { return access(n); }
        const formal& operator [](int n) const { return access(n); }
    };

private:
    /* stack used by internal algorithms, e.g. graft(). we use
       a static object to avoid dynamic allocation flapping. */

    static stack<node *> node_stack;

    /* we set a bit in the dirty bucket to indicate a node has
       been inserted into the forest in that bucket. collect()
       uses this to optimize its bucket searches. */

    static long dirty;

    /* can't use more than 64 buckets, since dirty is one machine word. */

    enum {
        LOG2_NR_NODE_BUCKETS    = 6,
        NR_NODE_BUCKETS         = (1 << LOG2_NR_NODE_BUCKETS)
    };

    static list<node> buckets[NR_NODE_BUCKETS];

    static void insert(node *n);
    static void remove(node *n);
    static int bucket(bits ts, long u, node *next);
    static node *find(bits ts, long u, node *next);
    static node *get(bits ts, long u, node *next);

    static bits set_n(bits ts, int n);
    static int get_n(bits ts);

    void inc();
    void dec();

    node *build(bits ts, long u);

    /* the first node of this type (or prefix). note that this
       is the only actual data member of a type instance. */

    node *_first;

public:
    static const type none;     /* an empty type */

    type() : _first(0) {}
    ~type() { dec(); }

    type(const type& src) { _first = src._first; inc(); }
    type& operator =(const type &rhs) { dec(); _first = rhs._first; inc(); }

    /* given a set of K_SPEC_* token bits, return the associated type.
       fails with an error() if the set doesn't describe a valid type. */

    static const type& map(token specs);

    /* the build functions append nodes to prefix types, which are
       denormalized types whose nodes are not in the forest. once
       the prefix is complete, it is converted into a proper type
       with graft, which attaches it to a point in the forest. */

    void build_array(int nelem);
    void build_ptr(bits quals);
    node& build_func(bits func, int n);

    void graft(const type& base);

    /* return the nth formal of this type, which
       quite obviously must be a function. */

    const formal& operator [](int n) const;

    /* true if two types refer to the same branch. note that this
       is not the same as type equivalence at the language level. */

    bool operator ==(const type& rhs) const { return _first == rhs._first; }

    /* call periodically from the parser to
       garbage-collect unreferenced type nodes */

    static void collect();
};

ENUM_BITSET(type::bits);

/* the encapsulation seems a bit bizarre here. clients need full access
   to a formal during type building, and read-only access everywhere else.
   we police by handing out const or non-const references accordingly. */

class type::formal
{
    friend class type;

public:
    string id;
    type t;

    bool is_register : 1;
    bool auto_downcast : 1;

    /* true if two formals have [do not have] identical specifications. */

    bool operator ==(const formal& rhs) const
    {
        return  (id == rhs.id)
            &&  (t == rhs.t)
            &&  (is_register == rhs.is_register)
            &&  (auto_downcast == rhs.auto_downcast);
    }

    bool operator !=(const formal& rhs) const { return !(*this == rhs); }

private:
    formal()
        : id(string::none),
          is_register(false),
          auto_downcast(false) {}
};

#endif /* TYPE_H */

/* vi: set ts=4 expandtab: */
