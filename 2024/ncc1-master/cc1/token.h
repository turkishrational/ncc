/* token.h - token classes                         ncc, the new c/c++ compiler

                    Copyright (c) 2021 Charles E. Youse (charles@gnuless.org).
                    All rights reserved. See LICENSE file for more details. */

#ifndef TOKEN_H
#define TOKEN_H

#include <stdio.h>

/* token classes are prefixed with K_, and variables
   are usually named k, for "historical" reasons. */

enum token
{
    /* bits[8:0] hold a value which is unique to the token. these must be
       odd because class name relies on [valid] tokens having bit[0] set.
       bits[8:1] are treated as a table index in print() - see token.cc. */

    K_INDEX_MASK        = ( 0x000001FE ),

    /* bit[9] is set for tokens that are keywords.
       bit[10] indicates the keyword is C++-only.

       the lexer uses these to determine when to
       promote an identifier to keyword token. */

    K_KEYWORD           = ( 0x00000200 ),
    K_CXX               = ( 0x00000400 ),

    /* bits[20:11] form a set of certain type specifier keywords.
       these are used to simplify mapping combinations to types. */

    K_SPEC_BOOL         = ( 0x00000800 ),
    K_SPEC_VOID         = ( 0x00001000 ),
    K_SPEC_CHAR         = ( 0x00002000 ),
    K_SPEC_SHORT        = ( 0x00004000 ),
    K_SPEC_INT          = ( 0x00008000 ),
    K_SPEC_LONG         = ( 0x00010000 ),
    K_SPEC_FLOAT        = ( 0x00020000 ),
    K_SPEC_DOUBLE       = ( 0x00040000 ),
    K_SPEC_UNSIGNED     = ( 0x00080000 ),
    K_SPEC_SIGNED       = ( 0x00100000 ),

    K_SPEC_MASK         = ( 0x001FF800 ),

    /************************************ token values proper ****/

    K_NONE,     /* must be first (0), so K_NONE == false; note lsb == 0 */

    K_IDENT     = (  3),            /* identifier */
    K_STRLIT    = (  5),            /* string literal */

    K_CCON      = (  7),            /* char constant */
    K_ICON      = (  9),            /* int constant */
    K_UCON      = ( 11),            /* unsigned constant */
    K_LCON      = ( 13),            /* long constant */
    K_ULCON     = ( 15),            /* unsigned long constant */
    K_FCON      = ( 17),            /* float constant */
    K_DCON      = ( 19),            /* double constant */
    K_LDCON     = ( 21),            /* long double constant */

    K_HASH      = ( 23),            /* # */
    K_NL        = ( 25),            /* \n */

    K_LPAREN    = ( 27),            /* ( */
    K_RPAREN    = ( 29),            /* ) */
    K_LBRACK    = ( 31),            /* [ */
    K_RBRACK    = ( 33),            /* ] */
    K_LBRACE    = ( 35),            /* { */
    K_RBRACE    = ( 37),            /* } */
    K_DOT       = ( 39),            /* . */
    K_DOTMUL    = ( 41),            /* .* */
    K_ELLIP     = ( 43),            /* ... */
    K_XOR       = ( 45),            /* ^ */
    K_COMMA     = ( 47),            /* , */
    K_COLON     = ( 49),            /* : */
    K_SCOPE     = ( 51),            /* :: */
    K_SEMI      = ( 53),            /* ; */
    K_QUEST     = ( 55),            /* ? */
    K_TILDE     = ( 57),            /* ~ */
    K_ARROW     = ( 59),            /* -> */
    K_ARROWMUL  = ( 61),            /* ->* */
    K_INC       = ( 63),            /* ++ */
    K_DEC       = ( 65),            /* -- */
    K_NOT       = ( 67),            /* ! */
    K_DIV       = ( 69),            /* / */
    K_MUL       = ( 71),            /* * */
    K_PLUS      = ( 73),            /* + */
    K_MINUS     = ( 75),            /* - */
    K_GT        = ( 77),            /* > */
    K_SHR       = ( 79),            /* >> */
    K_GTEQ      = ( 81),            /* >= */
    K_SHREQ     = ( 83),            /* >>= */
    K_LT        = ( 85),            /* < */
    K_SHL       = ( 87),            /* << */
    K_LTEQ      = ( 89),            /* <= */
    K_SHLEQ     = ( 91),            /* <<= */
    K_AND       = ( 93),            /* & */
    K_LAND      = ( 95),            /* && */
    K_ANDEQ     = ( 97),            /* &= */
    K_OR        = ( 99),            /* | */
    K_LOR       = (101),            /* || */
    K_OREQ      = (103),            /* |= */
    K_MINUSEQ   = (105),            /* -= */
    K_PLUSEQ    = (107),            /* += */
    K_MULEQ     = (109),            /* *= */
    K_DIVEQ     = (111),            /* /= */
    K_EQEQ      = (113),            /* == */
    K_NOTEQ     = (115),            /* != */
    K_MOD       = (117),            /* % */
    K_MODEQ     = (119),            /* %= */
    K_XOREQ     = (121),            /* ^= */
    K_EQ        = (123),            /* = */

    /* keywords and special identifiers are last.
       print() relies on this ordering (of index) */

    K_ASM       = (125) | K_KEYWORD | K_CXX,
    K_AUTO      = (127) | K_KEYWORD,
    K_BOOL      = (129) | K_KEYWORD | K_CXX | K_SPEC_BOOL,
    K_BREAK     = (131) | K_KEYWORD,
    K_CASE      = (133) | K_KEYWORD,
    K_CATCH     = (135) | K_KEYWORD | K_CXX,
    K_CHAR      = (137) | K_KEYWORD | K_SPEC_CHAR,
    K_CLASS     = (139) | K_KEYWORD | K_CXX,
    K_CONST     = (141) | K_KEYWORD,
    K_CONTINUE  = (143) | K_KEYWORD,
    K_DEFAULT   = (145) | K_KEYWORD,
    K_DELETE    = (147) | K_KEYWORD | K_CXX,
    K_DO        = (149) | K_KEYWORD,
    K_DOUBLE    = (151) | K_KEYWORD | K_SPEC_DOUBLE,
    K_ELSE      = (153) | K_KEYWORD,
    K_ENUM      = (155) | K_KEYWORD,
    K_EXTERN    = (157) | K_KEYWORD,
    K_FALSE     = (159) | K_KEYWORD | K_CXX,
    K_FLOAT     = (161) | K_KEYWORD | K_SPEC_FLOAT,
    K_FOR       = (163) | K_KEYWORD,
    K_FRIEND    = (165) | K_KEYWORD | K_CXX,
    K_GOTO      = (167) | K_KEYWORD,
    K_IF        = (169) | K_KEYWORD,
    K_INLINE    = (171) | K_KEYWORD | K_CXX,
    K_INT       = (173) | K_KEYWORD | K_SPEC_INT,
    K_LONG      = (175) | K_KEYWORD | K_SPEC_LONG,
    K_NEW       = (177) | K_KEYWORD | K_CXX,
    K_OPERATOR  = (179) | K_KEYWORD | K_CXX,
    K_PRIVATE   = (181) | K_KEYWORD | K_CXX,
    K_PROTECTED = (183) | K_KEYWORD | K_CXX,
    K_PUBLIC    = (185) | K_KEYWORD | K_CXX,
    K_REGISTER  = (187) | K_KEYWORD,
    K_RETURN    = (189) | K_KEYWORD,
    K_SHORT     = (191) | K_KEYWORD | K_SPEC_SHORT,
    K_SIGNED    = (193) | K_KEYWORD | K_SPEC_SIGNED,
    K_SIZEOF    = (195) | K_KEYWORD,
    K_STATIC    = (197) | K_KEYWORD,
    K_STRUCT    = (199) | K_KEYWORD,
    K_SWITCH    = (201) | K_KEYWORD,
    K_TEMPLATE  = (203) | K_KEYWORD | K_CXX,
    K_THIS      = (205) | K_KEYWORD | K_CXX,
    K_THROW     = (207) | K_KEYWORD | K_CXX,
    K_TRUE      = (209) | K_KEYWORD | K_CXX,
    K_TRY       = (211) | K_KEYWORD | K_CXX,
    K_TYPEDEF   = (213) | K_KEYWORD,
    K_UNION     = (215) | K_KEYWORD,
    K_UNSIGNED  = (217) | K_KEYWORD | K_SPEC_UNSIGNED,
    K_VIRTUAL   = (219) | K_KEYWORD | K_CXX,
    K_VOID      = (221) | K_KEYWORD | K_SPEC_VOID,
    K_VOLATILE  = (223) | K_KEYWORD,
    K_WHILE     = (225) | K_KEYWORD
};

ENUM_BITSET(token);

/* print a token in human-readable
   format to fp. for error reporting. */

void print(FILE *fp, token k);

/* simple map of text -> token for keywords
   and other special identifiers */

struct keyword
{
    const char *text;
    token k;
};

extern struct keyword keywords[];
extern int nr_keywords;

#endif /* TOKEN_H */

/* vi: set ts=4 expandtab: */
