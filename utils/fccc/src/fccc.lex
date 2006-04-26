%option noyywrap
/* The linker throws itself on the back if the yywrapper is enabled,
   so better disable than use a dummy return(1) function.
 */

%{
/*
 * IDENTIFIER CONSTANT STRING_LITERAL SIZEOF
 * PTR_OP INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
 * AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
 * SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
 * XOR_ASSIGN OR_ASSIGN TYPE_NAME
 *
 * TYPEDEF EXTERN STATIC AUTO REGISTER
 * CHAR SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE CONST VOLATILE VOID
 * STRUCT UNION ENUM ELLIPSIS
 *
 * CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN
 */

#include <stdio.h>
#include <stdlib.h> /* for malloc() */
#include <fccc.h>
#include <fccc_bison.h>

#define YYDEBUG

#ifdef L_DEBUG
#undef L_DEBUG
#define L_DEBUG(L_STR) (L_STR)
#endif
#ifndef L_DEBUG
#define L_DEBUG(L_STR)
#endif

int getlinenum(void);
int linenum = 1;

extern int fileno __P ((FILE *__stream)); /* To remove warnings */
%}

/* Regular definitions */
delim			[ \t]
ws			{delim}+
digit                   [0-9]
letter                  [a-zA-Z_]
hex                     [a-fA-F0-9]
id                      {letter}({letter}|{digit})*

FS                      (f|F|l|L)
IS                      (u|U|l|L)*

%s COMMENT
%%
"/*"[^"*/"\n]*"*/"      { /* no action */ }
"/*"[^"*/"\n]*\n        { ++linenum; BEGIN(COMMENT); }
<COMMENT>[^"*/"\n]*\n   { ++linenum; }
<COMMENT>[^"*/"\n]*"*/" { BEGIN(INITIAL); }
{ws}			{ /* no action */ }
\n                      { linenum++; } 
auto                    { L_DEBUG(printf("Lex, line %d: AUTO\n", linenum));
			return(AUTO); }
break                   { L_DEBUG(printf("Lex, line %d: BREAK\n", linenum));
			return(BREAK); }
case                    { L_DEBUG(printf("Lex, line %d: CASE\n", linenum));
			return(CASE); }
char                    { L_DEBUG(printf("Lex, line %d: CHAR\n", linenum));
			return(CHAR); }
const                   { L_DEBUG(printf("Lex, line %d: CONST\n", linenum));
			return(CONST); }
continue                { L_DEBUG(printf("Lex, line %d: CONTINUE\n", linenum));
			return(CONTINUE); }
default                 { L_DEBUG(printf("Lex, line %d: DEFAULT\n", linenum));
			return(DEFAULT); }
do                      { L_DEBUG(printf("Lex, line %d: DO\n", linenum));
			return(DO); }
double                  { L_DEBUG(printf("Lex, line %d: DOUBLE\n", linenum));
			return(DOUBLE); }
else                    { L_DEBUG(printf("Lex, line %d: ELSE\n", linenum));
			return(ELSE); }
enum                    { L_DEBUG(printf("Lex, line %d: ENUM\n", linenum));
			return(ENUM); }
extern                  { L_DEBUG(printf("Lex, line %d: EXTERN\n", linenum));
			return(EXTERN); }
float                   { L_DEBUG(printf("Lex, line %d: FLOAT\n", linenum));
			return(FLOAT); }
for                     { L_DEBUG(printf("Lex, line %d: FOR\n", linenum));
			return(FOR); }
goto                    { L_DEBUG(printf("Lex, line %d: GOTO\n", linenum));
			return(GOTO); }
if                      { L_DEBUG(printf("Lex, line %d: IF\n", linenum));
			return(IF); }
int                     { L_DEBUG(printf("Lex, line %d: INT\n", linenum));
			yylval.var_type=INT_T;
			return(INT); }
long                    { L_DEBUG(printf("Lex, line %d: LONG\n", linenum));
			return(LONG); }
register                { L_DEBUG(printf("Lex, line %d: REGISTER\n", linenum));
			return(REGISTER); }
return                  { L_DEBUG(printf("Lex, line %d: RETURN\n", linenum));
			return(RETURN); }
short                   { L_DEBUG(printf("Lex, line %d: SHORT\n", linenum));
			return(SHORT); }
signed                  { L_DEBUG(printf("Lex, line %d: SIGNED\n", linenum));
			return(SIGNED); }
sizeof                  { L_DEBUG(printf("Lex, line %d: SIZEOF\n", linenum));
			return(SIZEOF); }
static                  { L_DEBUG(printf("Lex, line %d: STATIC\n", linenum));
			return(STATIC); }
struct                  { L_DEBUG(printf("Lex, line %d: STRUCT\n", linenum));
			return(STRUCT); }
switch                  { L_DEBUG(printf("Lex, line %d: SWITCH\n", linenum));
			return(SWITCH); }
typedef                 { L_DEBUG(printf("Lex, line %d: TYPEDEF\n", linenum));
			return(TYPEDEF); }
union                   { L_DEBUG(printf("Lex, line %d: UNION\n", linenum));
			return(UNION); }
unsigned                { L_DEBUG(printf("Lex, line %d: UNSIGNED\n", linenum));
			return(UNSIGNED); }
void                    { L_DEBUG(printf("Lex, line %d: VOID\n", linenum));
			return(VOID); }
volatile                { L_DEBUG(printf("Lex, line %d: VOLATILE\n", linenum));
			return(VOLATILE); }
while                   { L_DEBUG(printf("Lex, line %d: WHILE\n", linenum));
			return(WHILE); }
0[xX]{hex}+{IS}?        { L_DEBUG(printf("Lex, line %d: CONSTANT\n", linenum));
			yylval.num=atoi(yytext);
			return(CONSTANT); }
0{digit}+{IS}?          { L_DEBUG(printf("Lex, line %d: CONSTANT\n", linenum));
			yylval.num=atoi(yytext);
			return(CONSTANT); }
{digit}+{IS}?           { L_DEBUG(printf("Lex, line %d: CONSTANT\n", linenum));
			yylval.num = atoi(yytext);	
			return(CONSTANT); }
L?'(\\.|[^\\'])+'       { L_DEBUG(printf("Lex, line %d: CONSTANT\n", linenum));
			return(CONSTANT); }
L?\"(\\.|[^\\"])*\"	{ L_DEBUG(printf("Lex, line %d: STR_LIT\n", linenum));
			strncpy(yylval.text, yytext, LEXEM_SIZE-1);
			yylval.text[LEXEM_SIZE-1]='\0';
			return(STRING_LITERAL); }
{id}                    { L_DEBUG(printf("Lex, line %d: ID\n", linenum));
			strncpy(yylval.text, yytext, LEXEM_SIZE-1);
			yylval.text[LEXEM_SIZE-1]='\0';
			return(IDENTIFIER); }

"..."                   { L_DEBUG(printf("Lex, line %d: ELLIPSIS\n", linenum));
			return(ELLIPSIS); }
">>="                   { return(RIGHT_ASSIGN); }
"<<="                   { return(LEFT_ASSIGN); }
"+="                    { return(ADD_ASSIGN); }
"-="                    { return(SUB_ASSIGN); }
"*="                    { return(MUL_ASSIGN); }
"/="                    { return(DIV_ASSIGN); }
"%="                    { return(MOD_ASSIGN); }
"&="                    { return(AND_ASSIGN); }
"^="                    { return(XOR_ASSIGN); }
"|="                    { return(OR_ASSIGN); }
">>"                    { return(RIGHT_OP); }
"<<"                    { return(LEFT_OP); }
"++"                    { return(INC_OP); }
"--"                    { return(DEC_OP); }
"->"                    { return(PTR_OP); }
"&&"                    { return(AND_OP); }
"||"                    { return(OR_OP); }
"<="                    { return(LE_OP); }
">="                    { return(GE_OP); }
"=="                    { return(EQ_OP); }
"!="                    { return(NE_OP); }
";"                     { return(';'); }
("{"|"<%")              { return('{'); }
("}"|"%>")              { return('}'); }
","                     { return(','); }
":"                     { return(':'); }
"="                     { return('='); }
"("                     { return('('); }
")"                     { return(')'); }
("["|"<:")              { return('['); }
("]"|":>")              { return(']'); }
"."                     { return('.'); }
"&"                     { return('&'); }
"!"                     { return('!'); }
"~"                     { return('~'); }
"-"                     { return('-'); }
"+"                     { return('+'); }
"*"                     { return('*'); }
"/"                     { return('/'); }
"%"                     { return('%'); }
"<"                     { return('<'); }
">"                     { return('>'); }
"^"                     { return('^'); }
"|"                     { return('|'); }
"?"                     { return('?'); }

%%

int getlinenum(void) {
	return linenum;
}

void suppress_flex_bug_warning(void) {
	if (0) unput('\n');
}

