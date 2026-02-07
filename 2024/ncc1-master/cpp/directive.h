/* directive.h - preprocessor directives           ncc, the new c/c++ compiler

                    Copyright (c) 2021 Charles E. Youse (charles@gnuless.org).
                    All rights reserved. See LICENSE file for more details. */

#ifndef DIRECTIVE_H
#define DIRECTIVE_H

#include "token.h"

extern void directive(struct list *);
extern void directive_check(void);

#endif /* DIRECTIVE_H */

/* vi: set ts=4 expandtab: */
