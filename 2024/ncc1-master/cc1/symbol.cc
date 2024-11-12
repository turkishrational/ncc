/* symbol.cc - symbol table                        ncc, the new c/c++ compiler

                    Copyright (c) 2021 Charles E. Youse (charles@gnuless.org).
                    All rights reserved. See LICENSE file for more details. */

#include "cc1.h"
#include "symbol.h"


const name name::anon;  /* default init is fine */
scope symbol::current_scope = SCOPE_GLOBAL;


void symbol::enter_scope()
{
    if (current_scope == SCOPE_MAX)
        error(INTERNAL, "scopes too deeply nested");

    current_scope = (scope) (current_scope + 1);
}

/* vi: set ts=4 expandtab: */
