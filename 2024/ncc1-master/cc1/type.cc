/* type.cc - type representation                   ncc, the new c/c++ compiler

                    Copyright (c) 2021 Charles E. Youse (charles@gnuless.org).
                    All rights reserved. See LICENSE file for more details. */

#include "cc1.h"
#include "type.h"

const type type::none;
slab type::node_slab(sizeof(node), 100);
list<type::node> type::buckets[NR_NODE_BUCKETS];
long type::dirty;
stack<type::node *> type::node_stack;

/* returns a new node. the reference count is already "bumped" for the
   caller's convenience. if we're dealing with a T_FUNC node that takes
   arguments, then a formals vector of correct size is allocated. if u
   (_formals) != 0, then the supplied formals are copied into the new
   node's vector. copying formals is inefficient, but infrequent. */

type::node::node(bits ts, long u, node *next)
    : _ts(ts),
      _u(u),
      _next(next),
      _refs(1)
{
    int n;

    if ((ts & T_FUNC) && (n = get_n(ts))) {
        _formals = new formal[n];

        if (u) {
            formal *formals = (formal *) u;

            for (int i = 0; i < n; ++i)
                _formals[i] = formals[i];
        }
    }
}

type::node::~node()
{
    if (_ts & T_FUNC)
        delete[] _formals;
}

/* helper function for node [] operator(s), which
   provide access to a T_FUNC node's arguments. */

type::formal& type::node::access(int n) const
{
    ASSERT(_ts & T_FUNC);
    ASSERT(n <= get_n(_ts));

    return _formals[n];
}

/* T_FUNC embeds the number of arguments in the type::bits.
   set_n() and get_n() set and get this number, accordingly. */

type::bits type::set_n(bits ts, int n)
{
    ASSERT(ts & T_FUNC);

    ts &= ~T_N_MASK;
    ts |= (type::bits) ((n << T_N_SHIFT) & T_N_MASK);

    return ts;
}

int type::get_n(bits ts)
{
    ASSERT(ts & T_FUNC);

    return (ts >> T_N_SHIFT) & T_N_MASK;
}

/* the intent is that a node hash will change when any of its content changes,
   except for its reference count. in practice we must also ignore the formals
   of T_FUNCs because the vector pointers have no relation to the contents. */

int type::bucket(bits ts, long u, node *next)
{
    if (ts & T_FUNC)
        u = 0;

    /* ts tends to be quite sparse, and we would like to encompass the number
       of function arguments, too, hence all the overlapping shifts. next and
       u are often pointers, so we try to account for the 0 lsbs. this hash is
       tuned [somewhat] under the assumption that NR_NODE_BUCKETS == 64. */

    return  ((ts ^ (ts >> 6) ^ (ts >> 12) ^ (ts >> 18))
            ^ (((long) next) >> 3)
            ^ (u ^ (u >> 3))) & (NR_NODE_BUCKETS - 1);
}

/* insert node into the forest hash */

void type::insert(node *n)
{
    unsigned b = bucket(n->_ts, n->_u, n->_next);
    buckets[b].insert(n, &node::_links);
    dirty |= (1L << b);
}

/* find the node fitting a description in the forest
   hash and return it, with its reference count bumped.
   returns 0 if not present. */

type::node *type::find(bits ts, long u, node *next)
{
    unsigned b = bucket(ts, u, next);
    node *n;

    for (node *n = buckets[b].first(); n;
        n = buckets[b].next(n, &node::_links))
    {
        if ((n->_ts == ts) && (n->_next == next)) {
            if (n->_ts & T_FUNC) {
                formal *formals = (formal *) u;
                int i;

                for (i = 0; i < get_n(ts); ++i)
                    if (n->_formals[i] != formals[i])
                        break;

                if (i == get_n(ts))
                    goto found;
            } else {
                if (n->_u == u)
                    goto found;
            }
        }
    }

    return 0;

found:
    n->inc();
    return n;
}

/* return the node in the forest that meets the description,
   creating a new one if none exists. the reference count is
   automatically bumped for the caller's convenience. */

type::node *type::get(bits ts, long u, node *next)
{
    node *n;

    n = find(ts, u, next);

    if (n == 0) {
        n = new node(ts, u, next);
        insert(n);
    }

    return n;
}

/* bump the reference count of a type up or down. we don't permit
   these on prefixes, as their operations are tightly circumscribed. */

void type::inc()
{
    node *n;

    for (n = _first; n; n = n->_next) {
        ASSERT(!(n->_ts & T_PREFIX));
        n->inc();
    }
}

void type::dec()
{
    node *n;

    for (n = _first; n; n = n->_next) {
        ASSERT(!(n->_ts & T_PREFIX));
        n->dec();
    }
}

/* internal helper for build family of functions.
   traverse the prefix to find its end, then append
   a new node. return the new node. */

type::node *type::build(bits ts, long u)
{
    node **p;
    node *n;

    ts |= T_PREFIX;

    for (p = &_first; n = *p; p = &(n->_next))
        ASSERT(n->_ts & T_PREFIX);

    n = new node(ts, u, 0);
    *p = n;

    return n;
}


void type::build_array(int nelem)
{
    ASSERT((nelem >= 0) && (nelem <= MAX_SIZE));
    build(T_ARRAY, nelem);
}


void type::build_ptr(bits quals)
{
    ASSERT((quals & T_QUAL_MASK) == quals);
    build(T_PTR | quals, 0);
}

/* note that build_func() returns a writeable node reference, but the only
   public operation on a node is argument indexing. the parser uses this
   access to fill in the argument details during the type-building process. */

type::node& type::build_func(bits func, int n)
{
    ASSERT((func & T_FUNC_MASK) == func);
    ASSERT((n >= 0) && (n <= MAX_FORMALS));

    return *build(set_n(T_FUNC, n) | func, 0);
}

/* when we attach a prefix to the tree, the prefix
   nodes are either put into the forest, or discarded.
   the end result is a normalized type. */

void type::graft(const type& base)
{
    node *n;

    while (_first) {
        node_stack.push(_first);
        _first = _first->_next;
    }

    _first = base._first;
    inc();

    while (!node_stack.empty()) {
        n = node_stack.top();
        n->_ts &= ~T_PREFIX;

        if (n = find(n->_ts, n->_u, _first))
            delete node_stack.top();
        else {
            n = node_stack.top();
            n->_next = _first;
            insert(n);
        }

        _first = n;
        node_stack.pop();
    }
}


void type::collect()
{
    node *n;
    node *next;
    int b;

    while (dirty) {
        b = __builtin_ctzl(dirty);

        for (n = buckets[b].first(); n; n = next) {
            next = buckets[b].next(n, &node::_links);

            if (n->_refs == 0) {
                buckets[b].remove(n, &node::_links);
                delete n;
            }
        }

        dirty &= ~(1L << b);
    }
}

/* the map[] array is searched linearly. it should rearranged in
   order of most common combinations- determine this empirically.
   almost certainly, K_SPEC_INT should be first. */

const type& type::map(token specs)
{
    static struct { token specs; bits ts; type t; } map[] =
    {
        { K_SPEC_BOOL,                                      T_BOOL      },
        { K_SPEC_VOID,                                      T_VOID      },
        { K_SPEC_CHAR,                                      T_CHAR      },
        { K_SPEC_CHAR | K_SPEC_SIGNED,                      T_SCHAR     },
        { K_SPEC_CHAR | K_SPEC_UNSIGNED,                    T_UCHAR     },
        { K_SPEC_SHORT,                                     T_SHORT     },
        { K_SPEC_SHORT | K_SPEC_SIGNED,                     T_SHORT     },
        { K_SPEC_SHORT | K_SPEC_INT,                        T_SHORT     },
        { K_SPEC_SHORT | K_SPEC_SIGNED | K_SPEC_INT,        T_SHORT     },
        { K_SPEC_SHORT | K_SPEC_UNSIGNED,                   T_USHORT    },
        { K_SPEC_SHORT | K_SPEC_UNSIGNED | K_SPEC_INT,      T_USHORT    },
        { K_SPEC_INT,                                       T_INT       },
        { K_SPEC_SIGNED,                                    T_INT       },
        { K_SPEC_SIGNED | K_SPEC_INT,                       T_INT       },
        { K_SPEC_UNSIGNED,                                  T_UINT      },
        { K_SPEC_UNSIGNED | K_SPEC_INT,                     T_UINT      },
        { K_SPEC_LONG,                                      T_LONG      },
        { K_SPEC_LONG | K_SPEC_SIGNED,                      T_LONG      },
        { K_SPEC_LONG | K_SPEC_INT,                         T_LONG      },
        { K_SPEC_LONG | K_SPEC_SIGNED | K_SPEC_INT,         T_LONG      },
        { K_SPEC_UNSIGNED | K_SPEC_LONG,                    T_ULONG     },
        { K_SPEC_UNSIGNED | K_SPEC_LONG | K_SPEC_INT,       T_ULONG     },
        { K_SPEC_FLOAT,                                     T_FLOAT     },
        { K_SPEC_DOUBLE,                                    T_DOUBLE    },
        { K_SPEC_DOUBLE | K_SPEC_LONG,                      T_LDOUBLE   }
    };

    for (int i = 0; i < ARRAY_SIZE(map); ++i) {
        if (map[i].specs == specs) {
            if (map[i].t._first == 0) /* populate on demand */
                map[i].t._first = get(map[i].ts, 0, 0);

            return map[i].t;
        }
    }

    error(ERROR, "illegal type specification");
}


/* vi: set ts=4 expandtab: */
