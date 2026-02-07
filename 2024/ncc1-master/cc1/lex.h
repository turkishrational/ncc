/* lex.h - lexical analysis                        ncc, the new c/c++ compiler

                    Copyright (c) 2021 Charles E. Youse (charles@gnuless.org).
                    All rights reserved. See LICENSE file for more details. */

#ifndef LEX_H
#define LEX_H

#include "token.h"
#include "string.h"

class lex
{
    static slab state_slab;

public:

    /* the lexical analyzer works on a stack of input states. usually, there
       is but one element on this stack, marking the current input location
       in the source file, but the parser may use save(), push(), and pop()
       to temporarily return to prior states. this is used for lookahead when
       parsing ambiguous constructs, or (in c++ mode) to make multiple passes
       over class definitions or expand templates. */

    struct state
    {
        friend class lex;

    private:
        /* our perceived location,
           for error reporting */

        int line_no;
        string path;

        /* true if this state is being
           used to expand a template */

        bool expansion;

        /* the most recently seen token,
           and any associated payload */

        token k;

        union
        {
            long i;         /* K_CCON .. K_ULCON */
            double f;       /* K_FCON .. K_LDCON */
            string s;       /* K_IDENT, K_STRLIT */
        };

        /* text marks the beginning of the most
           recent token in the lexical buffer, and
           pos marks the next character of interest. */

        const char *text;
        const char *pos;

        struct state *prev;        /* stack link */

        void *operator new(size_t) { return state_slab.alloc(); }
        void operator delete(void *p) { state_slab.free(p); }
    };

private:
    static state *top;  /* top of the stack */

    /* the input file is mmaped() at buf. eof marks the first byte past
       the end of valid data in buf; *eof is guaranteed to be NUL. */

    static const char *buf;
    static const char *eof;

    static inline int value(char);
    static inline int xvalue(char);

    static int escape();
    static token ccon();
    static token strlit();
    static token number();
    static token ident();
    static token scan0();

public:
    /* true if the lexer is initialized */

    static bool active() { return top != 0; }

    /* simple getters for the current
       state (i.e., the top of stack) */

    static token k() { return top->k; }
    static long i() { return top->i; }
    static double f() { return top->f; }
    static string s() { return top->s; }
    static int line_no() { return top->line_no; }
    static string path() { return top->path; }

    /* print the contents of the most recent token directly out of
       the lexical buffer; for %K in error(). in some cases the text
       is only a subset of the token, see e.g., ccon(). */

    static void print(FILE *fp);

    /* initialize the lexer, opening
       the specified path for input */

    static void init(const char *path);

    /* get a copy of the current state, for a later push() */

    static struct state save() { return *top; }

    /* return to the state specified, or just push a copy of the current
       state (for lookahead). expansion is used to fill in the expansion
       member; set to true if we're returning to a template definition.
       this is solely for sane error reporting- see backtrace(). */

    static void push(const struct state& s = *top, bool expansion = false);

    static void pop();      /* discard top of stack */

    /* read the next token from the current input
       and update state accordingly */

    static void scan();

    /* prints a backtrace of the input stack in human-readable form to
       the specified stdio file. used by error() to provide context when
       the error occurs in a template expansion. */

    static void backtrace(FILE *fp);
};

#endif /* LEX_H */

/* vi: set ts=4 expandtab: */
