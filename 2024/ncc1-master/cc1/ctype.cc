/* ctype.cc - character classes                    ncc, the new c/c++ compiler

                    Copyright (c) 2021 Charles E. Youse (charles@gnuless.org).
                    All rights reserved. See LICENSE file for more details. */

#include "cc1.h"
#include "ctype.h"

unsigned short ctype::map[256] =
{
    /* 00 */    NUL,            0,              0,              0,
    /* 04 */    0,              0,              0,              0,
    /* 08 */    0,              SPC,            NL,             SPC,
    /* 0C */    SPC,            SPC,            0,              0,

    /* 10 */    0,              0,              0,              0,
    /* 14 */    0,              0,              0,              0,
    /* 18 */    0,              0,              0,              0,
    /* 1C */    0,              0,              0,              0,

    /* 20 */    SPC,            G,              QUO|G,          G,
    /* 24 */    G,              G,              G,              TIC|G,
    /* 28 */    G,              G,              G,              SGN|G,
    /* 2C */    G,              SGN|G,          DOT|G,          G,

    /* 30 */    X|D|O|G,        X|D|O|G,        X|D|O|G,        X|D|O|G,
    /* 34 */    X|D|O|G,        X|D|O|G,        X|D|O|G,        X|D|O|G,
    /* 38 */    X|D|G,          X|D|G,          G,              G,
    /* 3C */    G,              G,              G,              G,

    /* 40 */    G,              A|X|G,          A|X|G,          A|X|G,
    /* 44 */    A|X|G,          A|X|G|E,        A|X|G|F,        A|G,
    /* 48 */    A|G,            A|G,            A|G,            A|G,
    /* 4C */    A|G|L,          A|G,            A|G,            A|G,

    /* 50 */    A|G,            A|G,            A|G,            A|G,
    /* 54 */    A|G,            A|G|U,          A|G,            A|G,
    /* 58 */    A|G,            A|G,            A|G,            G,
    /* 5C */    G,              G,              G,              A|G,

    /* 60 */    G,              A|X|G,          A|X|G,          A|X|G,
    /* 64 */    A|X|G,          A|X|G|E,        A|X|G|F,        A|G,
    /* 68 */    A|G,            A|G,            A|G,            A|G,
    /* 6C */    A|G|L,          A|G,            A|G,            A|G,

    /* 70 */    A|G,            A|G,            A|G,            A|G,
    /* 74 */    A|G,            A|G|U,          A|G,            A|G,
    /* 78 */    A|G,            A|G,            A|G,            G,
    /* 7C */    G,              G,              G,              0

                        /* 80 .. FF are unclassed */
};

/* vi: set ts=4 expandtab: */
