/* ctype.h - character classes                     ncc, the new c/c++ compiler

                    Copyright (c) 2021 Charles E. Youse (charles@gnuless.org).
                    All rights reserved. See LICENSE file for more details. */

#ifndef CTYPE_H
#define CTYPE_H

/* the standard ctype.h character classes don't suit the lexical analyzer
   very well, so we define our own. the poor semantics of c++ enumerations
   force us to play fast and loose with conversions inside the class. */

class ctype
{
private:
    static unsigned short map[256];

public:
    enum flags
    {
        A   = 0x0001,           /* alphabetic: A-Za-z_ */
        O   = 0x0002,           /* octal digit: 0-7 */
        D   = 0x0004,           /* decimal digit: 0-9 */
        X   = 0x0008,           /* hex digit: 0-9A-Fa-f */
        G   = 0x0010,           /* graphical character */
        E   = 0x0020,           /* letter E: eE */
        F   = 0x0040,           /* letter f: fF */
        L   = 0x0080,           /* letter l: lL */
        U   = 0x0100,           /* letter u: uU */
        SGN = 0x0200,           /* sign: +- */
        DOT = 0x0400,           /* dot: . */
        QUO = 0x0800,           /* quote: " */
        TIC = 0x1000,           /* tic: ' */
        SPC = 0x2000,           /* non-newline space */
        NL  = 0x4000,           /* newline */
        NUL = 0x8000            /* ASCII NUL */
    };

    static bool is(char c, flags f) { return map[c & 0xff] & f; }
};

inline ctype::flags operator |(ctype::flags lhs, ctype::flags rhs)
{
    return (ctype::flags) (((int) lhs) | ((int) rhs));
}

#endif /* CTYPE_H */

/* vi: set ts=4 expandtab: */
