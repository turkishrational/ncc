/* cpp.h - preprocessor                            ncc, the new c/c++ compiler

                    Copyright (c) 2021 Charles E. Youse (charles@gnuless.org).
                    All rights reserved. See LICENSE file for more details. */

#ifndef CPP_H
#define CPP_H

#include <stdlib.h>

#define ARRAY_SIZE(a)   (sizeof(a)/sizeof(*(a)))

#define NR_MACRO_BUCKETS 64

#define SYNC_WINDOW 20

extern char need_sync;
extern char x_flag;

void *safe_malloc(size_t);
void error(char *, ...);

#endif /* CPP_H */

/* vi: set ts=4 expandtab: */
