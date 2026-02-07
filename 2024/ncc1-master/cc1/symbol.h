/* symbol.h - symbol table                         ncc, the new c/c++ compiler

                    Copyright (c) 2021 Charles E. Youse (charles@gnuless.org).
                    All rights reserved. See LICENSE file for more details. */

#ifndef SYMBOL_H
#define SYMBOL_H

#include "list.h"
#include "token.h"
#include "string.h"
#include "type.h"

struct block;

/* symbols can be named via identifier or operator, or they can be anonymous.
   we handle all three cases in one machine word, leveraging inside knowledge
   about token and string representation and some type punning:

   in an anonymous name, _u == 0. this is neither a valid string (a null drop)
   nor a token of interest (== K_NONE). if the name is an identifier, _u is
   non-zero, but its lsb is always unset (since _id._drop is aligned). if the
   name is an operator, then _u is non-zero with a set lsb, since valid tokens
   are deliberately assigned odd values. (in this last case, we must take care
   to ensure that the upper bits of _u are kept clear, since _k is smaller.)

   thus all possibilities have distinct mappings onto _u. */

class name
{
    union
    {
        long _u;
        token _k;
        string _id;
    };

public:
    const static name anon;     /* handy anonymous name */

    name() : _u(0) {}
    name(token k) { _u = 0; _k = k; }
    name(string id) : _id(id) {}

    /* remember, class string is specifically designed to allow
       scalar comparison for equality, so this works for all _u. */

    bool operator ==(const name& rhs) const { _u == rhs._u; }
    bool operator !=(const name& rhs) const { _u != rhs._u; }
};


enum scope
{
    /* maximum scope nesting depth. this is a somewhat arbitrary number, but
       it can't be arbitrarily large as each scope has fixed overhead in the
       symbol table. ANSI C89 5.2.4.1 requires that we support a minimum of
       15 nesting levels of compound statements, so this must be large enough
       to accommodate that, keeping in mind that our idea of a scope doesn't
       quite map directly onto theirs. */

    NR_SCOPES           =   64,

    /* SCOPE_NONE usually means "not assigned yet", but there are actually
       symbols in the table at this level. tag symbols that go out of scope
       are housed here- there are certain loopholes in the scope rules that
       make it difficult to tell whether such tags are still referenced, so
       we never free them. instead we dump them in SCOPE_NONE and move on. */

    SCOPE_NONE          =   0,

    /* symbols with external linkage can be declared in inner scopes. we must
       track them after they go out of scope to ensure that any redeclarations
       are compatible, but they can't be made visible at file scope. they are
       stashed in SCOPE_LURKER until a file-scope declaration occurs (if ever)
       at which point they are promoted to SCOPE_GLOBAL. */

    SCOPE_LURKER        =   1,

    /* SCOPE_GLOBAL represents the outermost (file) scope.

       SCOPE_LOCAL..SCOPE_MAX are local scopes, used during function
       definitions, prototype parsing, tagged type definitions, etc. */

    SCOPE_GLOBAL        =   2,
    SCOPE_LOCAL         =   3,
                            /* ... */
    SCOPE_MAX           =   (NR_SCOPES - 4),

    /* SCOPE_ZOMBIE holds local symbols that have gone out of scope in the
       current function. we discard them once the code generator is done. */

    SCOPE_ZOMBIE        =   (NR_SCOPES - 3),

    /* labels have function scope so we set aside a scope level just for them.
       arguably they should be moved out of the symbol table altogether. */

    SCOPE_LABEL         =   (NR_SCOPES - 2),

    /* when we exit a function prototype scope, non-argument symbols (tags,
       enumeration constants, etc.) are temporarily moved to SCOPE_CONDEMNED.
       if a definition immediately follows, we resurrect them into the local
       scope. this only applies to c, as in c++ it is not possible to declare
       non-argument symbols in a function declaration/definition. */

    SCOPE_CONDEMNED     =   (NR_SCOPES - 1)
};

/* storage classes. obviously these are more-encompassing than storage
   classes proper, also incorporating the idea of "symbol type". note
   these are organized as a bit set, for matching groups of sclasses;
   among other things, this facilitates dividing symbols into namespaces. */

enum sclass
{
    S_NONE,         /* reserve 0 for "none" */

    /* tags live in their own namespace */

    S_STRUCT    =   ( 0x00000001 ),
    S_UNION     =   ( 0x00000002 ),
    S_ENUM      =   ( 0x00000004 ),

    S_TAG       =   ( S_STRUCT | S_UNION | S_ENUM ),

    /* most symbols live in the normal namespace. some explanation
       is required here: S_LOCAL is the storage class assigned to a
       block-level symbol without an explicit storage class. if its
       address is taken, it is converted to S_AUTO. if it goes out
       of scope still as an S_LOCAL, it is converted to S_REGISTER.
       this is a simple way to identify unaliased locals.

       S_REDIRECT symbols are symbols declared in inner scopes that
       refer to file-scope symbols at SCOPE_GLOBAL or SCOPE_LURKER. */

    S_TYPEDEF   =   ( 0x00000008 ),
    S_STATIC    =   ( 0x00000010 ),
    S_EXTERN    =   ( 0x00000020 ),
    S_AUTO      =   ( 0x00000040 ),
    S_REGISTER  =   ( 0x00000080 ),
    S_LOCAL     =   ( 0x00000100 ),
    S_CONST     =   ( 0x00000200 ),     /* enumeration constant */
    S_REDIRECT  =   ( 0x00000400 ),     /* symbol redirection */

    S_NORMAL    =   ( S_TYPEDEF | S_STATIC | S_EXTERN | S_AUTO
                    | S_REGISTER | S_LOCAL | S_CONST | S_REDIRECT ),

    /* members and labels live in their own separate worlds */

    S_MEMBER    =   ( 0x00000800 ),
    S_LABEL     =   ( 0x00001000 )
};

ENUM_BITSET(sclass);

/* the symbol table is a layered hash table- symbols are divided
   into buckets based on their name hashes, and the buckets are
   partially ordered by scope (outermost scope first). we try to
   be good citizens and minimize an instance's memory footprint. */

class symbol
{
    name _name;
    scope _scope;
    sclass _sclass;

    type _t;                    /* empty where N/A */

    bool _defined : 1;          /* defined (if declaration != definition) */
    bool _tentative : 1;        /* tentative definition seen (c only) */
    bool _arg : 1;              /* this is a function argument */
    bool _implicit : 1;         /* user has been warned about implicit def */
    bool _referenced : 1;       /* has been referenced in an expression */

    /* for error reporting- see where(). we record a symbol's
       first appearance here when it is created, and update it
       when the symbol is defined (if applicable). */

    string _path;
    int _line_no;

    union
    {
        int _value;                                     /* S_CONST */
        struct block *_target;                          /* S_LABEL */
        asm_label _label;                               /* S_STATIC */
        symbol *_redirect;                              /* S_REDIRECT */

        struct { int offset; symbol *of; } _member;     /* S_MEMBER */

        /* S_STRUCT/S_UNION/S_CLASS. size and align are running counters,
           with size in BITS, when _defined == false. once _defined, size
           is in BYTES. (align is always expressed in bytes.) */

        struct { int size; int align; } _strun;
    };

    /* _chain is a forward link between S_ARG/S_MEMBER siblings. in
       a S_STRUCT/S_UNION/S_CLASS, _chain indicates first S_MEMBER.
       (the head of the S_ARG chain is maintained externally.) */

    symbol *_chain;

    list<symbol>::entry *_links;        /* bucket links */

    /*************************************** end symbol instance members ***/

    static const int NR_BUCKETS = 64;   /* fixed, due to dirty[] word size */

    static list<symbol> buckets[NR_BUCKETS];

    /* when a symbol is inserted into a scope, we set a dirty bit for its
       bucket at that scope. this obviates a full scan of buckets each time we
       leave a scope, which is quite frequent (e.g., compound statements). */

    static unsigned long dirty[NR_SCOPES];

    static scope current_scope;

public:
    static void enter_scope();
};

#endif /* SYMBOL_H */

/* vi: set ts=4 expandtab: */
