/* token.cc - token classes                        ncc, the new c/c++ compiler

                    Copyright (c) 2021 Charles E. Youse (charles@gnuless.org).
                    All rights reserved. See LICENSE file for more details. */

#include <ctype.h>
#include "cc1.h"
#include "token.h"

struct keyword keywords[] =
{
    { "asm",        K_ASM           },  { "auto",       K_AUTO          },
    { "bool",       K_BOOL          },  { "break",      K_BREAK         },
    { "case",       K_CASE          },  { "catch",      K_CATCH         },
    { "char",       K_CHAR          },  { "class",      K_CLASS         },
    { "const",      K_CONST         },  { "continue",   K_CONTINUE      },
    { "default",    K_DEFAULT       },  { "delete",     K_DELETE        },
    { "do",         K_DO            },  { "double",     K_DOUBLE        },
    { "else",       K_ELSE          },  { "enum",       K_ENUM          },
    { "extern",     K_EXTERN        },  { "false",      K_FALSE         },
    { "float",      K_FLOAT         },  { "for",        K_FOR           },
    { "friend",     K_FRIEND        },  { "goto",       K_GOTO          },
    { "if",         K_IF            },  { "inline",     K_INLINE        },
    { "int",        K_INT           },  { "long",       K_LONG          },
    { "new",        K_NEW           },  { "operator",   K_OPERATOR      },
    { "private",    K_PRIVATE       },  { "protected",  K_PROTECTED     },
    { "public",     K_PUBLIC        },  { "register",   K_REGISTER      },
    { "return",     K_RETURN        },  { "short",      K_SHORT         },
    { "signed",     K_SIGNED        },  { "sizeof",     K_SIZEOF        },
    { "static",     K_STATIC        },  { "struct",     K_STRUCT        },
    { "switch",     K_SWITCH        },  { "template",   K_TEMPLATE      },
    { "this",       K_THIS          },  { "throw",      K_THROW         },
    { "true",       K_TRUE          },  { "try",        K_TRY           },
    { "typedef",    K_TYPEDEF       },  { "union",      K_UNION         },
    { "unsigned",   K_UNSIGNED      },  { "virtual",    K_VOID          },
    { "void",       K_VOID          },  { "volatile",   K_VOLATILE      },
    { "while",      K_WHILE         }
};

int nr_keywords = ARRAY_SIZE(keywords);


void print(FILE *fp, token k)
{
    static const char *text[] =         /* keyed to K_INDEX_MASK */
    {
        "end-of-file",              "identifier",
        "string literal",           "char constant",
        "int constant",             "unsigned constant",
        "long constant",            "unsigned long constant",
        "float constant",           "double constant",
        "long double constant",

        "#",    "newline",

        "(",    ")",    "[",    "]",    "{",    "}",    ".",    ".*",
        "...",  "^",    ",",    ":",    "::",   ";",    "?",    "~",
        "->",   "->*",  "++",   "--",   "!",    "/",    "*",    "+",
        "-",    ">",    ">>",   ">=",   ">>=",  "<" ,   "<<",   "<=",
        "<<=",  "&",    "&&",   "&=",   "|",    "||",   "|=",   "-=",
        "+=",   "*=",   "/=",   "==",   "!=",   "%",    "%=",   "^=",
        "="
    };

    int i = (k & K_INDEX_MASK) >> 1;
    const char *t;
    bool quote;

    if (i < ARRAY_SIZE(text)) {
        t = text[i];
        quote = !isalpha(*t);
    } else {
        /* obviously slow, but remember, this
           is only for error reporting */

        for (i = 0; i < nr_keywords; ++i)
            if (k == keywords[i].k) {
                t = keywords[i].text;
                break;
            }

        quote = true;
        ASSERT(i != nr_keywords);
    }

    if (quote) fputc('\'', fp);
    fputs(t, fp);
    if (quote) fputc('\'', fp);
}

/* vi: set ts=4 expandtab: */
