%{

int yylex(void); /* Implemented by flex. */

#include <fccc.h>
#include <parserfunctions.h>
	
int fccc_error=0;
	
int yyerror(char *);
#define YYERROR_VERBOSE

%}

%expect 1 /* We know about the if-then-else shift-/reduceconflict */

%token IDENTIFIER CONSTANT STRING_LITERAL SIZEOF
%token PTR_OP INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
%token AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
%token XOR_ASSIGN OR_ASSIGN TYPE_NAME

%token TYPEDEF EXTERN STATIC AUTO REGISTER
%token CHAR SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE CONST VOLATILE VOID
%token STRUCT UNION ENUM ELLIPSIS

%token CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN

%start translation_unit
%%

primary_expression
        : IDENTIFIER
	{ P_DEBUG(printf("P, line %d: found identifier \"%s\"\n", getlinenum(),$$.text)); }
        | CONSTANT
	{ P_DEBUG(printf("P, line %d: found constant %d\n", getlinenum(),$$.num)); }
        | STRING_LITERAL
	{ P_DEBUG(printf("P, line %d: found string literal \"%s\"\n", getlinenum(),$$.text)); }
        | '(' expression ')'
	{ P_DEBUG(printf("P, line %d: found expression.\n", getlinenum())); }
        ;

postfix_expression
        : primary_expression
	{ P_DEBUG(printf("P, line %d: found primary expression\n", getlinenum())); }
        | postfix_expression '[' expression ']'
	{ P_DEBUG(printf("P, line %d: found array\n", getlinenum())); }
        | postfix_expression '(' ')'
	{ P_DEBUG(printf("P, line %d: found void function\n", getlinenum())); }
        | postfix_expression '(' argument_expression_list ')'
	{ P_DEBUG(printf("P, line %d: found function\n", getlinenum())); }
        | postfix_expression '.' IDENTIFIER
	{ P_DEBUG(printf("P, line %d: found struct member access\n", getlinenum())); }
        | postfix_expression PTR_OP IDENTIFIER
	{ P_DEBUG(printf("P, line %d: found pointer operator\n", getlinenum())); }
        | postfix_expression INC_OP
	{ P_DEBUG(printf("P, line %d: found increase\n", getlinenum())); }
        | postfix_expression DEC_OP
	{ P_DEBUG(printf("P, line %d: found decrease\n", getlinenum())); }
        ;

argument_expression_list
        : assignment_expression
	{ P_DEBUG(printf("P, line %d: found an assignment\n", getlinenum())); }
        | argument_expression_list ',' assignment_expression
	{ P_DEBUG(printf("P, line %d: found an additional assignment\n", getlinenum())); }
        ;

unary_expression
        : postfix_expression
	{ P_DEBUG(printf("P, line %d: found a postfix expression\n", getlinenum())); }
        | INC_OP unary_expression
	{ P_DEBUG(printf("P, line %d: found preinc on unary exp\n", getlinenum())); }
        | DEC_OP unary_expression
	{ P_DEBUG(printf("P, line %d: found predec on unary exp\n", getlinenum())); }
        | unary_operator cast_expression
	{ P_DEBUG(printf("P, line %d: found unary operation on cast exp\n", getlinenum())); }
        | SIZEOF unary_expression
	{ P_DEBUG(printf("P, line %d: found sizeof(unary expr)\n", getlinenum())); }
        | SIZEOF '(' type_name ')'
	{ P_DEBUG(printf("P, line %d: found sizeof(type)\n", getlinenum())); }
        ;

unary_operator
        : '&'
	{ P_DEBUG(printf("P, line %d: found address operator\n", getlinenum())); }
        | '*'
	{ P_DEBUG(printf("P, line %d: found pointer operator\n", getlinenum())); }
        | '+'
	{ P_DEBUG(printf("P, line %d: found positive operator\n", getlinenum())); }
        | '-'
	{ P_DEBUG(printf("P, line %d: found negative operator\n", getlinenum())); }
        | '~'
	{ P_DEBUG(printf("P, line %d: found binary not\n", getlinenum())); }
        | '!'
	{ P_DEBUG(printf("P, line %d: found boolean not\n", getlinenum())); }
        ;

cast_expression
        : unary_expression
        | '(' type_name ')' cast_expression
        ;

multiplicative_expression
        : cast_expression
        | multiplicative_expression '*' cast_expression
        | multiplicative_expression '/' cast_expression
        | multiplicative_expression '%' cast_expression
        ;

additive_expression
        : multiplicative_expression
        | additive_expression '+' multiplicative_expression
        | additive_expression '-' multiplicative_expression
        ;

shift_expression
        : additive_expression
        | shift_expression LEFT_OP additive_expression
        | shift_expression RIGHT_OP additive_expression
        ;

relational_expression
        : shift_expression
        | relational_expression '<' shift_expression
        | relational_expression '>' shift_expression
        | relational_expression LE_OP shift_expression
        | relational_expression GE_OP shift_expression
        ;

equality_expression
        : relational_expression
        | equality_expression EQ_OP relational_expression
        | equality_expression NE_OP relational_expression
        ;

and_expression
        : equality_expression
        | and_expression '&' equality_expression
        ;

exclusive_or_expression
        : and_expression
        | exclusive_or_expression '^' and_expression
        ;

inclusive_or_expression
        : exclusive_or_expression
        | inclusive_or_expression '|' exclusive_or_expression
        ;

logical_and_expression
        : inclusive_or_expression
        | logical_and_expression AND_OP inclusive_or_expression
        ;

logical_or_expression
        : logical_and_expression
        | logical_or_expression OR_OP logical_and_expression
        ;

conditional_expression
        : logical_or_expression
        | logical_or_expression '?' expression ':' conditional_expression
        ;

assignment_expression
        : conditional_expression
	{ P_DEBUG(printf("P, line %d: assignment_expression ends with conditional_exp\n", getlinenum()));}
        | unary_expression assignment_operator assignment_expression
	{ P_DEBUG(printf("P, line %d: Found further assignment_expression\n", getlinenum())); }
        ;

assignment_operator
        : '='
	{ P_DEBUG(printf("P, line %d: found = assignment\n", getlinenum())); }
        | MUL_ASSIGN
	{ P_DEBUG(printf("P, line %d: found *= assignment\n", getlinenum())); }
        | DIV_ASSIGN
	{ P_DEBUG(printf("P, line %d: found /= assignment\n", getlinenum())); }
        | MOD_ASSIGN
	{ P_DEBUG(printf("P, line %d: found \\= assignment\n", getlinenum())); }
        | ADD_ASSIGN
	{ P_DEBUG(printf("P, line %d: found += assignment\n", getlinenum())); }
        | SUB_ASSIGN
	{ P_DEBUG(printf("P, line %d: found -= assignment\n", getlinenum())); }
        | LEFT_ASSIGN
	{ P_DEBUG(printf("P, line %d: found >>= assignment\n", getlinenum())); }
        | RIGHT_ASSIGN
	{ P_DEBUG(printf("P, line %d: found <<= assignment\n", getlinenum())); }
        | AND_ASSIGN
	{ P_DEBUG(printf("P, line %d: found &= assignment\n", getlinenum())); }
        | XOR_ASSIGN
	{ P_DEBUG(printf("P, line %d: found ^= assignment\n", getlinenum())); }
        | OR_ASSIGN
	{ P_DEBUG(printf("P, line %d: found |= assignment\n", getlinenum())); }
        ;

expression
        : assignment_expression
	{ P_DEBUG(printf("P, line %d: found assignment exp\n", getlinenum())); }
        | expression ',' assignment_expression
	{ P_DEBUG(printf("P, line %d: found additional assignment exp\n", getlinenum())); }
        ;

constant_expression
        : conditional_expression
	{ P_DEBUG(printf("P, line %d: found conditional exp\n", getlinenum())); }
        ;

declaration
        : declaration_specifiers ';'
	{ P_DEBUG(printf("P, line %d: found uninitialized declaration\n", getlinenum())); }
        | declaration_specifiers init_declarator_list ';'
	{ P_DEBUG(printf("P, line %d: found initialized declaration\n", getlinenum())); }
        ;

declaration_specifiers
        : storage_class_specifier
	{ P_DEBUG(printf("P, line %d: found storage class spec\n", getlinenum())); }
        | storage_class_specifier declaration_specifiers
        | type_specifier
	{ P_DEBUG(printf("P, line %d: found type spec\n", getlinenum())); }
        | type_specifier declaration_specifiers
        | type_qualifier
        | type_qualifier declaration_specifiers
        ;

init_declarator_list
        : init_declarator
	{ P_DEBUG(printf("P, line %d: found init declarator\n", getlinenum())); }
        | init_declarator_list ',' init_declarator
	{ P_DEBUG(printf("P, line %d: found additional init declarator\n", getlinenum())); }
        ;

init_declarator
        : declarator
        | declarator '=' initializer
        ;

storage_class_specifier
        : TYPEDEF
        | EXTERN
        | STATIC
        | AUTO
        | REGISTER
        ;

type_specifier
        : VOID
        | CHAR
        | SHORT
        | INT
        | LONG
        | FLOAT
        | DOUBLE
        | SIGNED
        | UNSIGNED
        | struct_or_union_specifier
        | enum_specifier
        | TYPE_NAME
        ;

struct_or_union_specifier
        : struct_or_union IDENTIFIER '{' struct_declaration_list '}'
        | struct_or_union '{' struct_declaration_list '}'
        | struct_or_union IDENTIFIER
        ;

struct_or_union
        : STRUCT
        | UNION
        ;

struct_declaration_list
        : struct_declaration
        | struct_declaration_list struct_declaration
        ;

struct_declaration
        : specifier_qualifier_list struct_declarator_list ';'
        ;

specifier_qualifier_list
        : type_specifier specifier_qualifier_list
        | type_specifier
        | type_qualifier specifier_qualifier_list
        | type_qualifier
        ;

struct_declarator_list
        : struct_declarator
        | struct_declarator_list ',' struct_declarator
        ;

struct_declarator
        : declarator
        | ':' constant_expression
        | declarator ':' constant_expression
        ;

enum_specifier
        : ENUM '{' enumerator_list '}'
        | ENUM IDENTIFIER '{' enumerator_list '}'
        | ENUM IDENTIFIER
        ;

enumerator_list
        : enumerator
        | enumerator_list ',' enumerator
        ;

enumerator
        : IDENTIFIER
        | IDENTIFIER '=' constant_expression
        ;

type_qualifier
        : CONST
        | VOLATILE
        ;

declarator
        : pointer direct_declarator
        | direct_declarator
        ;

direct_declarator
        : IDENTIFIER
	{ P_DEBUG(printf("P, line %d: declaring identifier \"%s\".\n", getlinenum(),$$.text)); 
	  $$.symbol_ref=insert($$.text);
	}
        | '(' declarator ')'
	{ P_DEBUG(printf("P, line %d: found declarator decl.\n", getlinenum())); }
        | direct_declarator '[' constant_expression ']'
        | direct_declarator '[' ']'
        | direct_declarator '(' 
	{ printf ("P, line %d: los! fct parameter\n", getlinenum());} parameter_type_list ')'
	{ P_DEBUG(printf("P, line %d: found function param list for fct %s.\n", getlinenum(),$1.text)); 
	}
	
        | direct_declarator '(' identifier_list ')'
	{ P_DEBUG(printf("P, line %d: found identifier list decl.\n", getlinenum())); }
        | direct_declarator '(' ')'
        ;

pointer
        : '*'
        | '*' type_qualifier_list
        | '*' pointer
        | '*' type_qualifier_list pointer
        ;

type_qualifier_list
        : type_qualifier
        | type_qualifier_list type_qualifier
        ;


parameter_type_list
        : parameter_list
        | parameter_list ',' ELLIPSIS
        ;

parameter_list
        : parameter_declaration
        | parameter_list ',' parameter_declaration
        ;

parameter_declaration
        : declaration_specifiers declarator
	{ P_DEBUG(printf("P, line %d: found normal param decl.\n", getlinenum())); }
        | declaration_specifiers abstract_declarator
	{ P_DEBUG(printf("P, line %d: found abstr. param decl.\n", getlinenum())); }
        | declaration_specifiers
	{ P_DEBUG(printf("P, line %d: found simple decl.\n", getlinenum())); }
        ;

identifier_list
        : IDENTIFIER
        | identifier_list ',' IDENTIFIER
        ;

type_name
        : specifier_qualifier_list
        | specifier_qualifier_list abstract_declarator
        ;

abstract_declarator
        : pointer
        | direct_abstract_declarator
	{ P_DEBUG(printf("P, line %d: found direct abstr decl.\n", getlinenum())); }
        | pointer direct_abstract_declarator
        ;

direct_abstract_declarator
        : '(' abstract_declarator ')'
        | '[' ']'
        | '[' constant_expression ']'
        | direct_abstract_declarator '[' ']'
        | direct_abstract_declarator '[' constant_expression ']'
        | '(' ')'
        | '(' parameter_type_list ')'
        | direct_abstract_declarator '(' ')'
        | direct_abstract_declarator '(' parameter_type_list ')'
        ;

initializer
        : assignment_expression
	{ P_DEBUG(printf("P, line %d: found assignment init.\n", getlinenum())); }
        | '{' initializer_list '}'
	{ P_DEBUG(printf("P, line %d: found initializer list.\n", getlinenum())); }
        | '{' initializer_list ',' '}'
	{ P_DEBUG(printf("P, line %d: found open initializer list.\n", getlinenum())); }
        ;

initializer_list
        : initializer
        | initializer_list ',' initializer
        ;

statement
        : labeled_statement
        | compound_statement
	{ P_DEBUG(printf("P, line %d: found compound stat.\n", getlinenum())); }
        | expression_statement
        | selection_statement
        | iteration_statement
        | jump_statement
        ;

labeled_statement
        : IDENTIFIER ':' statement
        | CASE constant_expression ':' statement
        | DEFAULT ':' statement
        ;

compound_statement
        : '{' '}'
	{ P_DEBUG(printf("P, line %d: found empty compound.\n", getlinenum())); }
        | '{' statement_list '}'
	{ P_DEBUG(printf("P, line %d: found sub program.\n", getlinenum())); }
        | '{' declaration_list '}'
	{ P_DEBUG(printf("P, line %d: found decl. compound\n", getlinenum())); }
        | '{' declaration_list statement_list '}'
	{ P_DEBUG(printf("P, line %d: found sub program w/ decl.\n", getlinenum())); }
        ;

declaration_list
        : declaration
        | declaration_list declaration
        ;

statement_list
        : statement
        | statement_list statement
        ;

expression_statement
        : ';'
        | expression ';'
        ;

selection_statement
        : IF '(' expression ')' statement
	{ P_DEBUG(printf("P, line %d: Got an if-statement.\n", getlinenum())); }
        | IF '(' expression ')' statement ELSE statement
	{ P_DEBUG(printf("P, line %d: Got an if-else-statement.\n", getlinenum())); }
        | SWITCH '(' expression ')' statement
	{ P_DEBUG(printf("P, line %d: Got a switch-statement.\n", getlinenum())); }
        ;

iteration_statement
        : WHILE '(' expression ')' statement
        | DO statement WHILE '(' expression ')' ';'
        | FOR '(' expression_statement expression_statement ')' statement
        | FOR '(' expression_statement expression_statement expression ')' statement
        ;

jump_statement
        : GOTO IDENTIFIER ';'
        | CONTINUE ';'
        | BREAK ';'
        | RETURN ';'
        | RETURN expression ';'
        ;

translation_unit
        : external_declaration
        | translation_unit external_declaration
        ;

external_declaration
        : function_definition
        | declaration
        ;

function_definition
        : declaration_specifiers declarator {new_scope();} declaration_list compound_statement 
	  { exit_scope(); P_DEBUG(printf("P, line %d: found function definition w/ decl list\n", getlinenum())); }
        | declaration_specifiers declarator compound_statement
	  { P_DEBUG(printf("P, line %d: found function definition\n", getlinenum())); }
        | declarator declaration_list compound_statement
	{ P_DEBUG(printf("P, line %d: found function definition w/ decl list w/o spec\n", getlinenum())); }
        | declarator compound_statement
	{ P_DEBUG(printf("P, line %d: found function definition w/o spec\n", getlinenum())); }
        ;

%%
#include <fccc.h>
#include <fccc-tools.h>
#include <symboltable.h>

#if 0
extern char yytext[];
extern int column;
#endif

int yyerror (char *s) {
	printf("\033[37;31;1mParser: line %d: %s\033[0m\n", getlinenum(),s);
	error_found();
	return 1;
}

int main(int argc, char **argv) 
{
	FILE* infile;
	char outfilename[LEXEM_SIZE];
	
	printf("fccc 0.0.1b October 13th 2001\n");
	if( argc==1 || argc>2) {
		printf("Usage: fccc <filename.c>\n");
		exit(1);
	}
	yyin = init_input_file(argv[1]);
	dot_c2dot_f(strncpy(outfilename, argv[1], LEXEM_SIZE));
	forthfile = init_output_file(outfilename);

	fccc_error = 0; /* No errors when we start */
	
	init_symboltable(); /* Initialize symbol table */
	
	yyparse(); /* Start the whole thing */

	dump(); /* dump symbol table */
	
	if(close((int)infile))
		printf("%s closed.\n", argv[1]);
	else
		printf("Cannot close %s!\n", argv[1]);
	if(close((int)forthfile))
		printf("%s closed.\n", outfilename);
	else
		printf("Cannot close %s!\n", outfilename);
	
	if(fccc_error) {
		unlink(outfilename);
		printf("There where error's - %s deleted.\n", outfilename);
	}
		
	return(0);
}
