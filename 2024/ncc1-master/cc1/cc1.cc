/* cc1.cc - compiler pass 1                        ncc, the new c/c++ compiler

                    Copyright (c) 2021 Charles E. Youse (charles@gnuless.org).
                    All rights reserved. See LICENSE file for more details. */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "cc1.h"
#include "token.h"
#include "string.h"
#include "lex.h"
#include "symbol.h"
#include "arm.h"
#include "c89.h"


bool cxx_mode;

/* the output file handle and its path name. the latter
   is only needed to remove partial output in error(). */

static FILE *out_fp;
static const char *out_path;


void output(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);

    while (*fmt) {
        if (*fmt != '%')
            putc(*fmt, out_fp);
        else switch (*++fmt) {
        case 'd':   fprintf(out_fp, "%d", va_arg(args, int)); break;
        case 'L':   {
                        asm_label label = va_arg(args, asm_label);
                        label.print(out_fp);
                        break;
                    }
        default:    ASSERT(0);
        }
        ++fmt;
    }

    va_end(args);
}


void error(error_level level, const char *fmt, ...)
{
    static const char *levels[] =
    {
        "WARNING",              /* keyed to enum error_level */
        "ERROR",
        "SYSTEM ERROR",
        "INTERNAL ERROR",
        "ASSERTION FAILURE"
    };

    va_list args;

    if (lex::active()) {
        lex::path().print(stderr);

        if (lex::line_no())
            fprintf(stderr, " (%d)", lex::line_no());

        fputs(": ", stderr);
    }

    fprintf(stderr, "%s: ", levels[level]);
    va_start(args, fmt);

    while (*fmt) {
        if (*fmt != '%')
            putc(*fmt, stderr);
        else switch (*++fmt) {
        case 'd':   fprintf(stderr, "%d", va_arg(args, int)); break;
        case 'k':   print(stderr, (token) va_arg(args, int)); break;
        case 's':   fprintf(stderr, "%s", va_arg(args, char *)); break;
        case 'E':   fprintf(stderr, "(%s)", strerror(errno)); break;
        case 'K':   lex::print(stderr); break;
        case 'S':   {
                        string s = va_arg(args, string);
                        s.print(stderr);
                        break;
                    }
        }
        ++fmt;
    }

    va_end(args);
    fputc('\n', stderr);

    if (level > WARNING) {
        if (out_fp) {
            fclose(out_fp);
            remove(out_path);
        }

        exit(1);
    }
}

/* called by slab::alloc() to allocate a
   new slab of objects from the system. */

void slab::refill()
{
    char *p;

    p = (char *) malloc(_obj_size * _per_slab);

    if (p == 0)
        error(SYSTEM, "out of memory %E");

    for (int i = 0; i < _per_slab; ++i, p += _obj_size)
        free(p);    /* n.b.: this->free() */
}


/* asm_label is trivial and can have no constructors.
   luckily, static initialization will give _num a value
   of 0 in asm_label::none, which is exactly what we want. */

asm_label asm_label::none;

void asm_label::assign()
{
    static unsigned num;

    _num = ++num;

    if (_num == 0)
        error(INTERNAL, "too many asm_labels");
}


int main(int argc, char *argv[])
{
    string::init();

    --argc;
    ++argv;

    while (*argv && (**argv == '-')) {
        ++(*argv);

        switch (**argv)
        {
        case 'x':
            cxx_mode = true;

            if ((*argv)[1])
                goto usage;

            break;

        default:
            goto usage;
        }

        ++argv;
        --argc;
    }

    if (argc != 2)
        goto usage;

    /* open input and output files, output
       first to avoid confusing errors. */

    out_path = argv[1];
    out_fp = fopen(out_path, "w");

    if (out_fp == 0)
        error(SYSTEM, "can't open output '%s' %E", out_path);

    lex::init(argv[0]);

    if (cxx_mode)
        arm::unit();
    else
        c89::unit();

    string::literals();
    fclose(out_fp);
    return 0;

usage:
    fprintf(stderr,
                                                                        "\n"
        "usage: cc1 {<option>} <input> <output>"                        "\n"
                                                                        "\n"
        "options:"                                                      "\n"
        "   -x                      compile in C++ mode"                "\n"
                                                                        "\n"
        "FATAL: invalid command-line syntax"                            "\n"
                                                                        "\n"
    );

    return 1;
}

/* vi: set ts=4 expandtab: */
