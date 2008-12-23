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

typedef void forth_word(void);

static forth_word * const words[];
ucell PC;
volatile int runforth = 0;

#ifndef FCOMPILER
/* instead of pointing to an explicit 0 variable we
 * point behind the pointer.
 */
static ucell t[] = { DOCOL, 0, (ucell)(t+3), 0 };
static ucell *trampoline = t;
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
	runforth = 1;

	PUSHR(PC);
	PC = pointer2cell(_cfa);
	while (rstackcnt > tmp && runforth) {
		dbg_interp_printk("enterforth: NEXT\n");
		next();
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
	funcptr=(void *)POP();
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
        static ucell **myself = NULL;
	if( !myself )
		myself = (ucell**)findword("my-self") + 1;
	return (*myself && **myself) ? (ucell)**myself : 0;
}

static void doivar(void)
{
	ucell r, *p = (ucell *)(*(ucell *) PC + sizeof(ucell));
	ucell ibase = get_myself();

	dbg_interp_printk("ivar, offset: %d size: %d (ibase %d)\n", p[0], p[1], ibase );

	r = ibase ? ibase + p[0] : (ucell)&p[2];
	PUSH( r );
}

static void doival(void)
{
	ucell r, *p = (ucell *)(*(ucell *) PC + sizeof(ucell));
	ucell ibase = get_myself();

	dbg_interp_printk("ivar, offset: %d size: %d\n", p[0], p[1] );

	r = ibase ? ibase + p[0] : (ucell)&p[2];
	PUSH( *(ucell *)r );
}

static void doidefer(void)
{
	ucell *p = (ucell *)(*(ucell *) PC + sizeof(ucell));
	ucell ibase = get_myself();

	dbg_interp_printk("doidefer, offset: %d size: %d\n", p[0], p[1] );

	PUSHR(PC);
	PC = ibase ? ibase + p[0] : (ucell)&p[2];
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
