##############################################################################
#
#					          ncc, the new c/c++ compiler
#
#                   Copyright (c) 2021 Charles E. Youse (charles@gnuless.org).
#                      All rights reserved. See LICENSE file for more details.
#
##############################################################################

CC=gcc -std=c89
CFLAGS=-g

CXX=g++ -std=c++98 -Wno-return-type
CXXFLAGS=-g -DDEBUG

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

.cc.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<

##############################################################################

all:	cpp/cpp cc1/cc1

clean:
	rm -f cpp/*.o cpp/cpp
	rm -f cc1/*.o cc1/cc1

##############################################################################

CPP_HDRS=	cpp/cpp.h cpp/directive.h cpp/evaluate.h cpp/input.h \
		cpp/macro.h cpp/token.h cpp/vstring.h

CPP_OBJS=	cpp/cpp.o cpp/directive.o cpp/evaluate.o cpp/input.o \
		cpp/macro.o cpp/token.o cpp/vstring.o

cpp/cpp.o: 		cpp/cpp.c $(CPP_HDRS)
cpp/directive.o:	cpp/directive.c $(CPP_HDRS)
cpp/evaluate.o:		cpp/evaluate.c $(CPP_HDRS)
cpp/input.o:		cpp/input.c $(CPP_HDRS)
cpp/macro.o:		cpp/macro.c $(CPP_HDRS)
cpp/token.o:		cpp/token.c $(CPP_HDRS)
cpp/vstring.o:		cpp/vstring.c $(CPP_HDRS)

cpp/cpp: $(CPP_OBJS)
	$(CC) $(CFLAGS) -o cpp/cpp $(CPP_OBJS)

##############################################################################

CC1_HDRS=	cc1/arm.h cc1/c89.h cc1/cc1.h cc1/ctype.h cc1/lex.h \
		cc1/list.h cc1/stack.h cc1/string.h cc1/symbol.h \
		cc1/token.h cc1/type.h

CC1_OBJS=	cc1/cc1.o cc1/ctype.o cc1/lex.o cc1/string.o cc1/symbol.o \
		cc1/token.o cc1/type.o cc1/c89-decl.o

cc1/cc1.o:		cc1/cc1.cc $(CC1_HDRS)
cc1/ctype.o:		cc1/ctype.cc $(CC1_HDRS)
cc1/lex.o:		cc1/lex.cc $(CC1_HDRS)
cc1/string.o:		cc1/string.cc $(CC1_HDRS)
cc1/symbol.o:		cc1/symbol.cc $(CC1_HDRS)
cc1/token.o:		cc1/token.cc $(CC1_HDRS)
cc1/type.o:		cc1/type.cc $(CC1_HDRS)

cc1/c89-decl.o:		cc1/c89-decl.cc $(CC1_HDRS)

cc1/cc1: $(CC1_OBJS)
	$(CXX) $(CXXFLAGS) -o cc1/cc1 $(CC1_OBJS)

##############################################################################
