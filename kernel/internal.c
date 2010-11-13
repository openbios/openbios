/* tag: internal words, inner interpreter and such
 *
 * Copyright (C) 2003 Patrick Mauritz, Stefan Reinauer
 *
 * See the file "COPYING" for further information about
 * the copyright and warranty status of this work.
 */

/*
 * execution works as follows:
 *  - PC is pushed on return stack
 *  - PC is set to new CFA
 *  - address pointed by CFA is executed by CPU
 */

#ifndef FCOMPILER
#include "libc/vsprintf.h"
#else
#include <stdarg.h>
#endif

typedef void forth_word(void);

static forth_word * const words[];
ucell PC;
volatile int interruptforth = 0;

#define DEBUG_MODE_NONE 0
#define DEBUG_MODE_STEP 1
#define DEBUG_MODE_TRACE 2
#define DEBUG_MODE_STEPUP 3

#define DEBUG_BANNER "\nStepper keys: <space>/<enter> Up Down Trace Rstack Forth\n"

/* Empty linked list of debug xts */
struct debug_xt {
	ucell xt_docol;
	ucell xt_semis;
	int mode;
	struct debug_xt *next;
};

static struct debug_xt debug_xt_eol = { (ucell)0, (ucell)0, 0, NULL};
static struct debug_xt *debug_xt_list = &debug_xt_eol;

/* Static buffer for xt name */
char xtname[MAXNFALEN];

#ifndef FCOMPILER
/* instead of pointing to an explicit 0 variable we
 * point behind the pointer.
 */
static ucell t[] = { 0, 0, 0, 0 };
static ucell *trampoline = t;

void forth_init(void)
{
    init_trampoline(trampoline);
}
#endif

#ifndef CONFIG_DEBUG_INTERPRETER
#define dbg_interp_printk( a... )	do { } while(0)
#else
#define dbg_interp_printk( a... )	printk( a )
#endif

#ifndef CONFIG_DEBUG_INTERNAL
#define dbg_internal_printk( a... )	do { } while(0)
#else
#define dbg_internal_printk( a... )	printk( a )
#endif


void init_trampoline(ucell *tramp)
{
    tramp[0] = DOCOL;
    tramp[1] = 0;
    tramp[2] = target_ucell(pointer2cell(tramp) + 3 * sizeof(ucell));
    tramp[3] = 0;
}


static inline void processxt(ucell xt)
{
	void (*tokenp) (void);

	dbg_interp_printk("processxt: pc=%x, xt=%x\n", PC, xt);
	tokenp = words[xt];
	tokenp();
}

static void docol(void)
{				/* DOCOL */
	PUSHR(PC);
	PC = read_ucell(cell2pointer(PC));

	dbg_interp_printk("docol: %s\n", cell2pointer( lfa2nfa(PC - sizeof(cell)) ));
}

static void semis(void)
{
	PC = POPR();
}

static inline void next(void)
{
	PC += sizeof(ucell);

	dbg_interp_printk("next: PC is now %x\n", PC);
	processxt(read_ucell(cell2pointer(read_ucell(cell2pointer(PC)))));
}

static inline void next_dbg(void);

int enterforth(xt_t xt)
{
	ucell *_cfa = (ucell*)cell2pointer(xt);
	cell tmp;

	if (read_ucell(_cfa) != DOCOL ) {
		trampoline[1] = target_ucell(xt);
		_cfa = trampoline;
	}

	if (rstackcnt < 0)
		rstackcnt = 0;

	tmp = rstackcnt;
	interruptforth = FORTH_INTSTAT_CLR;

	PUSHR(PC);
	PC = pointer2cell(_cfa);

	while (rstackcnt > tmp && !(interruptforth & FORTH_INTSTAT_STOP)) {
		if (debug_xt_list->next == NULL) {
			while (rstackcnt > tmp && !interruptforth) {
				dbg_interp_printk("enterforth: NEXT\n");
				next();
			}
		} else {
			while (rstackcnt > tmp && !interruptforth) {
				dbg_interp_printk("enterforth: NEXT_DBG\n");
				next_dbg();
			}
		}

		/* Always clear the debug mode change flag */
		interruptforth = interruptforth & (~FORTH_INTSTAT_DBG);
	}

#if 0
	/* return true if we took an exception. The caller should normally
	 * handle exceptions by returning immediately since the throw
	 * is supposed to abort the execution of this C-code too.
	 */

	if( rstackcnt != tmp )
		printk("EXCEPTION DETECTED!\n");
#endif
	return rstackcnt != tmp;
}

/* called inline thus a slightly different behaviour */
static void lit(void)
{				/* LIT */
	PC += sizeof(cell);
	PUSH(read_ucell(cell2pointer(PC)));
	dbg_interp_printk("lit: %x\n", read_ucell(cell2pointer(PC)));
}

static void docon(void)
{				/* DOCON */
	ucell tmp = read_ucell(cell2pointer(read_ucell(cell2pointer(PC)) + sizeof(ucell)));
	PUSH(tmp);
	dbg_interp_printk("docon: PC=%x, value=%x\n", PC, tmp);
}

static void dovar(void)
{				/* DOVAR */
	ucell tmp = read_ucell(cell2pointer(PC)) + sizeof(ucell);
	PUSH(tmp);		/* returns address to variable */
	dbg_interp_printk("dovar: PC: %x, %x\n", PC, tmp);
}

static void dobranch(void)
{				/* unconditional branch */
	PC += sizeof(cell);
	PC += read_cell(cell2pointer(PC));
}

static void docbranch(void)
{				/* conditional branch */

	PC += sizeof(cell);
	if (POP()) {
		dbg_internal_printk("  ?branch: end loop\n");
	} else {
		dbg_internal_printk("  ?branch: follow branch\n");
		PC += read_cell(cell2pointer(PC));
	}
}


static void execute(void)
{				/* EXECUTE */
	ucell address = POP();
	dbg_interp_printk("execute: %x\n", address);

	PUSHR(PC);
	trampoline[1] = target_ucell(address);
	PC = pointer2cell(trampoline);
}

/*
 * call ( ... function-ptr -- ??? )
 */
static void call(void)
{
#ifdef FCOMPILER
	printk("Sorry. Usage of Forth2C binding is forbidden during bootstrap.\n");
	exit(1);
#else
	void (*funcptr) (void);
	funcptr=(void *)cell2pointer(POP());
	dbg_interp_printk("call: %x", funcptr);
	funcptr();
#endif
}

/*
 * sys-debug ( errno -- )
 */

static void sysdebug(void)
{
#ifdef FCOMPILER
	cell errorno=POP();
	exception(errorno);
#else
        (void) POP();
#endif
}

static void dodoes(void)
{				/* DODOES */
	ucell data = read_ucell(cell2pointer(PC)) + (2 * sizeof(ucell));
	ucell word = read_ucell(cell2pointer(read_ucell(cell2pointer(PC)) + sizeof(ucell)));

	dbg_interp_printk("DODOES data=%x word=%x\n", data, word);

	PUSH(data);
	PUSH(word);

	execute();
}

static void dodefer(void)
{
	docol();
}

static void dodo(void)
{
	cell startval, endval;
	startval = POP();
	endval = POP();

	PUSHR(endval);
	PUSHR(startval);
}

static void doisdo(void)
{
	cell startval, endval, offset;

	startval = POP();
	endval = POP();

	PC += sizeof(cell);

	if (startval == endval) {
		offset = read_cell(cell2pointer(PC));
		PC += offset;
	} else {
		PUSHR(endval);
		PUSHR(startval);
	}
}

static void doloop(void)
{
	cell offset, startval, endval;

	startval = POPR() + 1;
	endval = POPR();

	PC += sizeof(cell);

	if (startval < endval) {
		offset = read_cell(cell2pointer(PC));
		PC += offset;
		PUSHR(endval);
		PUSHR(startval);
	}

}

static void doplusloop(void)
{
	ucell high, low;
	cell increment, startval, endval, offset;

	increment = POP();

	startval = POPR();
	endval = POPR();

	low = (ucell) startval;
	startval += increment;

	PC += sizeof(cell);

	if (increment >= 0) {
		high = (ucell) startval;
	} else {
		high = low;
		low = (ucell) startval;
	}

	if (endval - (low + 1) >= high - low) {
		offset = read_cell(cell2pointer(PC));
		PC += offset;

		PUSHR(endval);
		PUSHR(startval);
	}
}

/*
 *  instance handling CFAs
 */
#ifndef FCOMPILER
static ucell get_myself(void)
{
	static ucell *myselfptr = NULL;
	if (myselfptr == NULL)
		myselfptr = (ucell*)cell2pointer(findword("my-self")) + 1;
	ucell *myself = (ucell*)cell2pointer(*myselfptr);
	return (myself != NULL) ? *myself : 0;
}

static void doivar(void)
{
	ucell r, *p = (ucell *)(*(ucell *) cell2pointer(PC) + sizeof(ucell));
	ucell ibase = get_myself();

	dbg_interp_printk("ivar, offset: %d size: %d (ibase %d)\n", p[0], p[1], ibase );

	r = ibase ? ibase + p[0] : pointer2cell(&p[2]);
	PUSH( r );
}

static void doival(void)
{
	ucell r, *p = (ucell *)(*(ucell *) cell2pointer(PC) + sizeof(ucell));
	ucell ibase = get_myself();

	dbg_interp_printk("ivar, offset: %d size: %d\n", p[0], p[1] );

	r = ibase ? ibase + p[0] : pointer2cell(&p[2]);
	PUSH( *(ucell *)cell2pointer(r) );
}

static void doidefer(void)
{
	ucell *p = (ucell *)(*(ucell *) cell2pointer(PC) + sizeof(ucell));
	ucell ibase = get_myself();

	dbg_interp_printk("doidefer, offset: %d size: %d\n", p[0], p[1] );

	PUSHR(PC);
	PC = ibase ? ibase + p[0] : pointer2cell(&p[2]);
	PC -= sizeof(ucell);
}
#else
static void noinstances(void)
{
	printk("Opening devices is not supported during bootstrap. Sorry.\n");
	exit(1);
}
#define doivar   noinstances
#define doival   noinstances
#define doidefer noinstances
#endif
/*
 * $include / $encode-file
 */

#ifdef FCOMPILER
static void
string_relay( void (*func)(const char *) )
{
	int len = POP();
	char *name, *p = (char*)cell2pointer(POP());
	name = malloc( len + 1 );
	memcpy( name, p, len );
	name[len]=0;
	(*func)( name );
	free( name );
}
#else
#define	string_relay( dummy )	do { DROP(); DROP(); } while(0)
#endif

static void
do_include( void )
{
	string_relay( &include_file );
}

static void
do_encode_file( void )
{
	string_relay( &encode_file );
}


/*
 * Debug support functions
 */

static
int printf_console( const char *fmt, ... )
{
	cell tmp;

	char buf[512];
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	/* Push to the Forth interpreter for console output */
	tmp = rstackcnt;

	PUSH(pointer2cell(buf));
	PUSH((int)strlen(buf));
	trampoline[1] = findword("type");

	PUSHR(PC);
	PC = pointer2cell(trampoline);

	while (rstackcnt > tmp) {
		dbg_interp_printk("printf_console: NEXT\n");
		next();
	}

	return i;
}

static
int getchar_console( void )
{
	cell tmp;

	/* Push to the Forth interpreter for console output */
	tmp = rstackcnt;

	trampoline[1] = findword("key");

	PUSHR(PC);
	PC = pointer2cell(trampoline);

	while (rstackcnt > tmp) {
		dbg_interp_printk("getchar_console: NEXT\n");
		next();
	}

	return POP();
}

static void
display_dbg_dstack ( void )
{
	/* Display dstack contents between parentheses */
	int i;

	if (dstackcnt == 0) {
		printf_console(" ( Empty ) ");
		return;
	} else {
		printf_console(" ( ");
		for (i = 1; i <= dstackcnt; i++) {
			if (i != 1)
				printf_console(" ");
			printf_console("%" FMT_CELL_x, dstack[i]);
		}
		printf_console(" ) ");
	}
}

static void
display_dbg_rstack ( void )
{
	/* Display rstack contents between parentheses */
	int i;

	if (rstackcnt == 0) {
		printf_console(" ( Empty ) ");
		return;
	} else {
		printf_console("\nR: ( ");
		for (i = 1; i <= rstackcnt; i++) {
			if (i != 1)
				printf_console(" ");
			printf_console("%" FMT_CELL_x, rstack[i]);
		}
		printf_console(" ) \n");
	}
}

static int
add_debug_xt( ucell xt )
{
	struct debug_xt *debug_xt_item;

	/* If the xt CFA isn't DOCOL then issue a warning and do nothing */
	if (read_ucell(cell2pointer(xt)) != DOCOL) {
		printf_console("\nprimitive words cannot be debugged\n");
		return 0;
	}

	/* If this xt is already in the list, do nothing but indicate success */
	for (debug_xt_item = debug_xt_list; debug_xt_item->next != NULL; debug_xt_item = debug_xt_item->next)
		if (debug_xt_item->xt_docol == xt)
			return 1;

	/* We already have the CFA (PC) indicating the starting cell of the word, however we also
	   need the ending cell too (we cannot rely on the rstack as it can be arbitrarily
	   changed by a forth word). Hence the use of findsemis() */

	/* Otherwise add to the head of the linked list */
	debug_xt_item = malloc(sizeof(struct debug_xt));
	debug_xt_item->xt_docol = xt;
	debug_xt_item->xt_semis = findsemis(xt);
	debug_xt_item->mode = DEBUG_MODE_NONE;
	debug_xt_item->next = debug_xt_list;
	debug_xt_list = debug_xt_item;

	/* Indicate debug mode change */
	interruptforth |= FORTH_INTSTAT_DBG;

	/* Success */
	return 1;
} 

static void
del_debug_xt( ucell xt )
{
	struct debug_xt *debug_xt_item, *tmp_xt_item;

	/* Handle the case where the xt is at the head of the list */
	if (debug_xt_list->xt_docol == xt) {
		tmp_xt_item = debug_xt_list;
		debug_xt_list = debug_xt_list->next;
		free(tmp_xt_item);

		return;
	}	

	/* Otherwise find this xt in the linked list and remove it */
	for (debug_xt_item = debug_xt_list; debug_xt_item->next != NULL; debug_xt_item = debug_xt_item->next) {
		if (debug_xt_item->next->xt_docol == xt) {
			tmp_xt_item = debug_xt_item->next;
			debug_xt_item->next = debug_xt_item->next->next;
			free(tmp_xt_item);
		}
	}

	/* If the list is now empty, indicate debug mode change */
	if (debug_xt_list->next == NULL)
		interruptforth |= FORTH_INTSTAT_DBG;
}

static void
do_source_dbg( struct debug_xt *debug_xt_item )
{
	/* Forth source debugger implementation */
	char k, done = 0;

	/* Display current dstack */
	display_dbg_dstack();
	printf_console("\n");

	fstrncpy(xtname, lfa2nfa(read_ucell(cell2pointer(PC)) - sizeof(cell)), MAXNFALEN);
	printf_console("%p: %s ", cell2pointer(PC), xtname);

	/* If in trace mode, we just carry on */
	if (debug_xt_item->mode == DEBUG_MODE_TRACE)
		return;

	/* Otherwise in step mode, prompt for a keypress */
	k = getchar_console();

	/* Only proceed if done is true */
	while (!done)
	{
		switch (k) {

			case ' ':
			case '\n':
				/* Perform a single step */
				done = 1;
				break;

			case 'u':
			case 'U':
				/* Up - unmark current word for debug, mark its caller for
				 * debugging and finish executing current word */ 

				/* Since this word could alter the rstack during its execution,
				 * we only know the caller when (semis) is called for this xt.
				 * Hence we mark the xt as a special DEBUG_MODE_STEPUP which
				 * means we run as normal, but schedule the xt for deletion
				 * at its corresponding (semis) word when we know the rstack
				 * will be set to its final parent value */
				debug_xt_item->mode = DEBUG_MODE_STEPUP;
				done = 1;
				break;

			case 'd':
			case 'D':
				/* Down - mark current word for debug and step into it */
				done = add_debug_xt(read_ucell(cell2pointer(PC)));
				if (!done) {
					k = getchar_console();
				}
				break;

			case 't':
			case 'T':
				/* Trace mode */
				debug_xt_item->mode = DEBUG_MODE_TRACE;
				done = 1;
				break;

			case 'r':
			case 'R':
				/* Display rstack */
				display_dbg_rstack();
				done = 0;
				k = getchar_console();
				break;

			case 'f':
			case 'F':
				/* Start subordinate Forth interpreter */
				PUSHR(PC - sizeof(cell));
				PC = findword("outer-interpreter") + sizeof(ucell);

				/* Save rstack position for when we return */
				dbgrstackcnt = rstackcnt;
				done = 1;
				break;

			default:
				/* Display debug banner */
				printf_console(DEBUG_BANNER);
				k = getchar_console();
		}
	}
}

static void docol_dbg(void)
{				/* DOCOL */
	struct debug_xt *debug_xt_item;

	PUSHR(PC);
	PC = read_ucell(cell2pointer(PC));

	/* If current xt is in our debug xt list, display word name */
	debug_xt_item = debug_xt_list;
	while (debug_xt_item->next) {
		if (debug_xt_item->xt_docol == PC) {
			fstrncpy(xtname, lfa2nfa(PC - sizeof(cell)), MAXNFALEN);
			printf_console("\n: %s ", xtname);

			/* Step mode is the default */
			debug_xt_item->mode = DEBUG_MODE_STEP;
		}

		debug_xt_item = debug_xt_item->next;
	}

	dbg_interp_printk("docol_dbg: %s\n", cell2pointer( lfa2nfa(PC - sizeof(cell)) ));
}

static void semis_dbg(void)
{
	struct debug_xt *debug_xt_item, *debug_xt_up = NULL;

	/* If current semis is in our debug xt list, disable debug mode */
	debug_xt_item = debug_xt_list;
	while (debug_xt_item->next) {
		if (debug_xt_item->xt_semis == PC) {
			if (debug_xt_item->mode != DEBUG_MODE_STEPUP) {
				/* Handle the normal case */
				fstrncpy(xtname, lfa2nfa(debug_xt_item->xt_docol - sizeof(cell)), MAXNFALEN);
				printf_console("\n[ Finished %s ] ", xtname);

				/* Reset to step mode in case we were in trace mode */
				debug_xt_item->mode = DEBUG_MODE_STEP;
			} else {
				/* This word requires execution of the debugger "Up"
				 * semantics. However we can't do this here since we
				 * are iterating through the debug list, and we need 
				 * to change it. So we do it afterwards. 
				 */ 
				debug_xt_up = debug_xt_item;	
			}
		}

		debug_xt_item = debug_xt_item->next;
	}

	/* Execute debugger "Up" semantics if required */
	if (debug_xt_up) {
		/* Only add the parent word if it is not within the trampoline */
		if (rstack[rstackcnt] != (cell)pointer2cell(&trampoline[1])) {
			del_debug_xt(debug_xt_up->xt_docol);
			add_debug_xt(findxtfromcell(rstack[rstackcnt]));

			fstrncpy(xtname, lfa2nfa(findxtfromcell(rstack[rstackcnt]) - sizeof(cell)), MAXNFALEN);
			printf_console("\n[ Up to %s ] ", xtname);
		} else {
			fstrncpy(xtname, lfa2nfa(findxtfromcell(debug_xt_up->xt_docol) - sizeof(cell)), MAXNFALEN);
			printf_console("\n[ Finished %s (Unable to go up, hit trampoline) ] ", xtname); 

			del_debug_xt(debug_xt_up->xt_docol);
		}

		debug_xt_up = NULL;
	}

	PC = POPR();
}

static inline void next_dbg(void)
{
	struct debug_xt *debug_xt_item;
	void (*tokenp) (void);

	PC += sizeof(ucell);

	/* If the PC lies within a debug range, run the source debugger */
	debug_xt_item = debug_xt_list;
	while (debug_xt_item->next) {
		if (PC >= debug_xt_item->xt_docol && PC <= debug_xt_item->xt_semis &&
			debug_xt_item->mode != DEBUG_MODE_STEPUP) {
			do_source_dbg(debug_xt_item);
		}

		debug_xt_item = debug_xt_item->next;
	}

	dbg_interp_printk("next_dbg: PC is now %x\n", PC);

	/* Intercept DOCOL and SEMIS and redirect to debug versions */
	if (read_ucell(cell2pointer(read_ucell(cell2pointer(PC)))) == DOCOL) {
		tokenp = docol_dbg;
		tokenp();
	} else if (read_ucell(cell2pointer(read_ucell(cell2pointer(PC)))) == DOSEMIS) {
		tokenp = semis_dbg;
		tokenp();
	} else {
		/* Otherwise process as normal */
		processxt(read_ucell(cell2pointer(read_ucell(cell2pointer(PC)))));
	}
}

static void
do_debug_xt( void )
{
	ucell xt = POP();

	/* Add to the debug list */
	if (add_debug_xt(xt)) {
		/* Display debug banner */
		printf_console(DEBUG_BANNER);

		/* Indicate change to debug mode */
		interruptforth |= FORTH_INTSTAT_DBG;	
	}
}

static void
do_debug_off( void )
{
	/* Empty the debug xt linked list */
	while (debug_xt_list->next != NULL)
		del_debug_xt(debug_xt_list->xt_docol);
}

