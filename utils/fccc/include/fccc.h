#ifndef FCCC_H
#define FCCC_H

#define LEXEM_SIZE 256

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Debugging macro */
#ifdef P_DEBUG
#undef P_DEBUG
#define P_DEBUG(P_STR) (P_STR)
#else
#define P_DEBUG(P_STR)
#endif /* P_DEBUG */

extern FILE *yyin;

FILE *forthfile;

int getlinenum(void);
void error_found(void);

enum var_types {CHAR_T, SHORT_T, INT_T, LONG_T, VOID_T, ERROR_T};

typedef struct {
	char lexem[LEXEM_SIZE];
	char unique_id[LEXEM_SIZE];
	int scope_no;
	enum var_types type; /* Used for return type and variable type */
} symbol;

struct tag_yystype {
	char text[LEXEM_SIZE];
	int num;
	enum var_types var_type;
	symbol* symbol_ref;
};

#define YYSTYPE struct tag_yystype

#endif /* FCCC_H */
