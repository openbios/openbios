CC        = gcc
CCFLAGS   = -Wall -O0 -ansi -pedantic -g -c -I../include -I./
LINK      = gcc
LEX       = flex
YACC      = bison
YACCFLAGS = -d -v
DEBUGFLAGS = -U L_DEBUG -D P_DEBUG

.phony: all
all: fccc

.phony: clean
clean:
	$(RM) *.o *~ fccc_lex.c fccc_bison.c fccc_bison.output fccc_bison.h \
          fccc core

OFILES = $(patsubst %.c,%.o,$(wildcard *.c))

%.o: %.c
	$(CC) $(CCFLAGS) $<;

fccc: $(OFILES) fccc_bison.o fccc_lex.o
	$(LINK) -o fccc $(shell \ls *.o)
	
fccc_lex.o: fccc_lex.c
	$(CC) $(CCFLAGS) $(DEBUGFLAGS) fccc_lex.c

fccc_bison.o: fccc_bison.c
	$(CC) $(CCFLAGS) $(DEBUGFLAGS) fccc_bison.c

fccc_lex.c: fccc.lex fccc.y
	$(LEX) -ofccc_lex.c fccc.lex

fccc_bison.c: fccc.y
	$(YACC) $(YACCFLAGS) -o fccc_bison.c fccc.y
