/* lex.cc - lexical analysis                       ncc, the new c/c++ compiler

                    Copyright (c) 2021 Charles E. Youse (charles@gnuless.org).
                    All rights reserved. See LICENSE file for more details. */

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>
#include "cc1.h"
#include "lex.h"
#include "ctype.h"

slab lex::state_slab(sizeof(state), 64);

struct lex::state *lex::top;
const char *lex::buf;
const char *lex::eof;

/* mmap() the specified path and push the initial state. the only trick
   here is that we need *eof to be NUL; if the file length is an exact
   multiple of PAGE_SIZE, we must map a guard page to make this guarantee. */

void lex::init(const char *path)
{
    int fd;
    void *addr;
    struct state *s;
    struct stat sb;

    fd = open(path, O_RDONLY);

    if (fd == -1)
        error(SYSTEM, "can't open '%s' for reading %E", path);

    if (fstat(fd, &sb) == -1)
        error(SYSTEM, "can't stat '%s' %E", path);

    /* we could accomodate empty input files with additional logic,
       but why? an empty input passed through the preprocessor will
       not result in empty output, so this won't happen in practice. */

    if (sb.st_size == 0)
        error(INTERNAL, "input file '%s' is empty", path);

    addr = mmap(0, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    if (addr == MAP_FAILED)
        error(SYSTEM, "can't mmap input '%s' %E", path);

    lex::buf = (const char *) addr;
    lex::eof = lex::buf + sb.st_size;

    if ((sb.st_size % PAGE_SIZE) == 0) {
        if (mmap((void *) lex::eof, PAGE_SIZE, PROT_READ,
                 MAP_ANON | MAP_PRIVATE, -1, 0) == MAP_FAILED)
            error(SYSTEM, "can't mmap guard page %E");
    }

    close(fd);

    /* we purposely leave pos unset
       here as a sentinel. see scan(). */

    s = new state;

    s->line_no = 0;
    s->path = string::lookup(path, strlen(path));
    s->expansion = false;
    s->pos = 0;
    s->prev = 0;
    top = s;

    scan();     /* prime with the first token */
}

/* at some point, we may want to add the name of
   the template being expanded to the backtrace. */

void lex::backtrace(FILE *fp)
{
    struct state *s;

    for (s = top; s; s = s->prev) {
        if (s->expansion) {
            fputs("\tfrom ", fp);
            s->prev->path.print(fp);
            fprintf(fp, " (%d)\n", s->prev->line_no);
        }
    }
}


void lex::push(const struct state& s, bool expansion)
{
    struct state *s2 = new struct state(s);

    s2->expansion = expansion;
    s2->prev = top;
    top = s2;
}


void lex::pop()
{
    struct state *s = top;

    top = top->prev;
    delete s;

    /* we never pop the initial input, so
       the stack should never be empty. */

    ASSERT(top);
}

/* naughty macro shortcuts for elements of the current state. */

#define TEXT        (top->text)
#define POS         (top->pos)

/* there exists the remote possibility that the token text exceeds the length
   of an int; cap at MAX_INT just in case, for whatever good that will do. */

void lex::print(FILE *fp)
{
    int len;

    if ((POS - TEXT) > INT_MAX)
        len = INT_MAX;
    else
        len = POS - TEXT;

    fprintf(fp, "%.*s", len, TEXT);
}

/* decimal or hexadecimal digit value */

inline int lex::value(char c)
{
    return c - '0';
}

inline int lex::xvalue(char c)
{
    if (ctype::is(c, ctype::D))
        return value(c);
    else
        return (c | 0x20) - 'a' + 10;
}

/* interpret an escape sequence and
   return its associated value */

int lex::escape()
{
    const char *text = POS;     /* saved for error reporting */
    bool overflow = false;
    int c;

    ++POS;  /* backslash */

    if (ctype::is(*POS, ctype::O)) {
        c = value(*POS++);
        if (ctype::is(*POS, ctype::O)) {
            c <<= 3;
            c += value(*POS++);
            if (ctype::is(*POS, ctype::O)) {
                c <<= 3;
                c += value(*POS++);

                if (c > UCHAR_MAX)
                    goto out_of_range;
            }
        }
    } else if (*POS == 'x') {
        c = 0;
        ++POS;

        if (!ctype::is(*POS, ctype::X))
            error(ERROR, "malformed hexadecimal escape sequence");

        while (ctype::is(*POS, ctype::X)) {
            if (c & 0xF0)
                overflow = true;

            c <<= 4;
            c += xvalue(*POS++);
        }

        if (overflow)
            goto out_of_range;
    } else {
        switch (*POS)
        {
        case 'a':       c = '\a'; break;
        case 'b':       c = '\b'; break;
        case 'f':       c = '\f'; break;
        case 'n':       c = '\n'; break;
        case 'r':       c = '\r'; break;
        case 't':       c = '\t'; break;
        case 'v':       c = '\v'; break;

        case '?':
        case '"':
        case '\\':
        case '\'':      c = *POS; break;

        default:    if (ctype::is(*POS, ctype::G))
                        error(ERROR, "unrecognized escape \\%c", *POS);
                    else
                        error(ERROR, "invalid escape sequence");
        }
        ++POS;
    }

    return c;

out_of_range:
    TEXT = text;
    error(ERROR, "escape sequence %K out of range");
}


/* single-character constants are sign-extended, since plain char is
   signed. multi-character constants are ints, specified in base 256,
   which are permitted to overflow (e.g., '\xff\xff\xff\xff' is -1). */

token lex::ccon()
{
    unsigned char c;
    int count = 0;

    ++POS;  /* open ' */
    top->i = 0;

    while (!ctype::is(*POS, ctype::NUL | ctype::NL | ctype::TIC)) {
        if (*POS == '\\')
            c = escape();
        else
            c = *POS++;

        top->i <<= 8;
        top->i += c;
        ++count;
    }

    if (*POS != '\'')
        error(ERROR, "unterminated character constant");

    ++POS;  /* close ' */

    switch (count)
    {
    case 1:     top->i = (char) top->i;
                return cxx_mode ? K_CCON : K_ICON;

    case 2:     /* we might want to add a warning */
    case 3:     /* for multi-character constants */
    case 4:     return K_ICON;

    default:    error(ERROR, "malformed character constant %K");
    }
}

/* if a literal contains escape sequences, we must build its contents in
   the string pool. otherwise we can use its text in the lexical buffer. */

token lex::strlit()
{
    bool pool = false;
    int c;

    ++POS;  /* open " */

    while (!ctype::is(*POS, ctype::QUO | ctype::NUL | ctype::NL)) {
        if (*POS == '\\') {
            pool = true;
            c = escape();
        } else
            c = *POS++;

        string::putc(c);
    }

    if (*POS != '"')
        error(ERROR, "unterminated string literal");

    if (pool)
        top->s = string::preserve();
    else {
        string::discard();
        top->s = string::lookup(TEXT + 1, POS - TEXT - 1);
    }

    ++POS;  /* close " */
    return K_STRLIT;
}

/* numeric constants. the committee really made a mess of these,
   for dubious reasons. luckily we can fob off much of the dirty
   work onto the standard library. */

token lex::number()
{
    bool is_unsigned = false;
    bool is_long = false;
    bool is_fp = false;
    unsigned long u;
    char *endptr;
    token k;

    errno = 0;

    while (ctype::is(*POS, ctype::A | ctype::D | ctype::DOT)) {
        if (*POS == '.')
            is_fp = true;

        if (ctype::is(*POS, ctype::E)
          && ctype::is(POS[1], ctype::SGN | ctype::D))
        {
            ++POS;
            is_fp = true;
        }

        ++POS;
    }

    if (is_fp) {
        k = K_DCON;
        top->f = strtod(TEXT, &endptr);

        if (ctype::is(*endptr, ctype::L)) {
            ++endptr;
            k = K_LDCON;
        } else if (ctype::is(*endptr, ctype::F)) {
            top->f = strtof(TEXT, &endptr);
            ++endptr;
            k = K_FCON;
        }
    } else {
        top->i = u = strtoul(TEXT, &endptr, 0);

        if (ctype::is(*endptr, ctype::U)) {
            is_unsigned = true;
            ++endptr;
        }

        if (ctype::is(*endptr, ctype::L)) {
            is_long = true;
            ++endptr;
        }

        if (!is_unsigned && ctype::is(*endptr, ctype::U)) {
            is_unsigned = true;
            ++endptr;
        }

        if (is_long && is_unsigned)         /* this is a trainwreck. */
            k = K_ULCON;                    /* surely there is a more */
        else if (is_long) {                 /* elegant way to determine */
            if (u > LONG_MAX)               /* the type of a constant */
                k = K_ULCON;
            else
                k = K_LCON;
        } else if (is_unsigned) {
            if (u > UINT_MAX)
                k = K_ULCON;
            else
                k = K_UCON;
        } else {
            if (u > LONG_MAX)
                k = K_ULCON;
            else if (u > UINT_MAX)
                k = K_LCON;
            else if (u > INT_MAX) {
                if (*TEXT == '0')
                    k = K_UCON;
                else
                    k = K_LCON;
            } else
                k = K_ICON;
        }
    }

    if (endptr != POS)
        error(ERROR, "malformed numeric constant '%K'");

    if (errno)
        error(ERROR, "numeric constant '%K' out of range");

    return k;
}

/* the string table knows which identifiers are keywords. our only job is
   to be sure c++ keywords are only recognized when we're in c++ mode. */

token lex::ident()
{
    token k = K_IDENT;

    while (ctype::is(*POS, ctype::A | ctype::D))
        ++POS;

    top->s = string::lookup(TEXT, POS - TEXT);

    if ((top->s.k() & K_KEYWORD) && (cxx_mode || !(top->s.k() & K_CXX)))
        k = top->s.k();

    return k;
}

/* the inner lexer does the heavy lifting, the actual token recognition. it
   updates all of the current state except (perhaps oddly) the token itself,
   relying on the outer lexer for that. */

#define OPERATOR(self, dup, eq, dupeq)                                  \
    do {                                                                \
        if (dupeq && (POS[1] == *POS) && (POS[2] == '=')) {             \
            POS += 3;                                                   \
            return dupeq;                                               \
        }                                                               \
                                                                        \
        if (dup && (POS[1] == *POS)) {                                  \
            POS += 2;                                                   \
            return dup;                                                 \
        }                                                               \
                                                                        \
        if (eq && (POS[1] == '=')) {                                    \
            POS += 2;                                                   \
            return eq;                                                  \
        }                                                               \
                                                                        \
        ++POS;                                                          \
        return self;                                                    \
    } while (0)

token lex::scan0()
{
    if (POS == 0) {         /* this fakes a newline at the beginning of */
        POS = buf;          /* input, to sync up the logic in scan() */
        return K_NL;
    }

    while (ctype::is(*POS, ctype::SPC))
        ++POS;

    TEXT = POS;

    switch (*POS)
    {
    case '\'':  return ccon();
    case '\"':  return strlit();

    case '\n':  ++POS; return K_NL;
    case '#':   ++POS; return K_HASH;
    case '?':   ++POS; return K_QUEST;
    case ';':   ++POS; return K_SEMI;
    case ',':   ++POS; return K_COMMA;
    case '(':   ++POS; return K_LPAREN;
    case ')':   ++POS; return K_RPAREN;
    case '{':   ++POS; return K_LBRACE;
    case '}':   ++POS; return K_RBRACE;
    case '[':   ++POS; return K_LBRACK;
    case ']':   ++POS; return K_RBRACK;
    case '~':   ++POS; return K_TILDE;

    case '=':   OPERATOR(K_EQ, K_EQEQ, K_NONE, K_NONE);
    case '!':   OPERATOR(K_NOT, K_NONE, K_NOTEQ, K_NONE);
    case '<':   OPERATOR(K_LT, K_SHL, K_LTEQ, K_SHLEQ);
    case '>':   OPERATOR(K_GT, K_SHR, K_GTEQ, K_SHREQ);
    case '^':   OPERATOR(K_XOR, K_NONE, K_XOREQ, K_NONE);
    case '|':   OPERATOR(K_OR, K_LOR, K_OREQ, K_NONE);
    case '&':   OPERATOR(K_AND, K_LAND, K_ANDEQ, K_NONE);
    case '*':   OPERATOR(K_MUL, K_NONE, K_MULEQ, K_NONE);
    case '/':   OPERATOR(K_DIV, K_NONE, K_DIVEQ, K_NONE);
    case '%':   OPERATOR(K_MOD, K_NONE, K_MODEQ, K_NONE);
    case '+':   OPERATOR(K_PLUS, K_INC, K_PLUSEQ, K_NONE);

    case ':':   if (cxx_mode && (POS[1] == ':')) {
                    POS += 2;
                    return K_SCOPE;
                }

                ++POS;
                return K_COLON;

    case '-':   if (POS[1] == '>') {
                    if (cxx_mode && (POS[2] == '*')) {
                        POS += 3;
                        return K_ARROWMUL;
                    }

                    POS += 2;
                    return K_ARROW;
                }

                OPERATOR(K_MINUS, K_DEC, K_MINUSEQ, K_NONE);

    case '.':   if ((POS[1] == '.') && (POS[2] == '.')) {
                    POS += 3;
                    return K_ELLIP;
                }

                if (cxx_mode && (POS[1] == '*')) {
                    POS += 2;
                    return K_DOTMUL;
                }

                if (!ctype::is(POS[1], ctype::D)) {
                    ++POS;
                    return K_DOT;
                }

                /* fall thru */

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':

                return number();

    case 'A': case 'B': case 'C': case 'D': case 'E':
    case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': case '_': case 'a': case 'b': case 'c':
    case 'd': case 'e': case 'f': case 'g': case 'h':
    case 'i': case 'j': case 'k': case 'l': case 'm':
    case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w':
    case 'x': case 'y': case 'z':

                return ident();

    case 0:
        if (POS == eof) return K_NONE;
    default:
        error(ERROR, "invalid character (ASCII %d)", *POS & 0xff);
    }
}

/* the outer lexer is responsible for error location tracking. it acts
   as a filter, stripping out # directives and newlines from the token
   stream seen by the parser. if we ever implement #pragma directives,
   they will be dispatched from here. */

void lex::scan()
{
    top->k = scan0();

    while (top->k == K_NL) {
        ++top->line_no;
        top->k = scan0();

        if (top->k == K_HASH) {
            top->k = scan0();

            if (top->k == K_ICON) {
                top->line_no = top->i;
                top->k = scan0();
                if (top->k == K_STRLIT) {
                    top->path = top->s;
                    top->k = scan0();
                }
            }

            /* if the botched directive was a line directive
               we'll be hopelessly lost here. oh well. */

            if (top->k != K_NL)
                error(ERROR, "malformed directive");

            top->k = scan0();
        }
    }
}

/* vi: set ts=4 expandtab: */
