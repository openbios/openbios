/* tag: C implementation of all forth primitives
 *
 * Copyright (C) 2003 Patrick Mauritz, Stefan Reinauer
 *
 * See the file "COPYING" for further information about
 * the copyright and warranty status of this work.
 */

/*
 *  dup         ( x -- x x )
 */

static void fdup(void)
{
	const cell tmp = GETTOS();
	PUSH(tmp);
}


/*
 *  2dup        ( x1 x2 -- x1 x2 x1 x2 )
 */

static void twodup(void)
{
	cell tmp = GETITEM(1);
	PUSH(tmp);
	tmp = GETITEM(1);
	PUSH(tmp);
}


/*
 *  ?dup        ( x -- 0 | x x )
 */

static void isdup(void)
{
	const cell tmp = GETTOS();
	if (tmp)
		PUSH(tmp);
}


/*
 *  over        ( x y -- x y x )
 */

static void over(void)
{
	const cell tmp = GETITEM(1);
	PUSH(tmp);
}


/*
 *  2over ( x1 x2 x3 x4 -- x1 x2 x3 x4 x1 x2 )
 */

static void twoover(void)
{
	const cell tmp = GETITEM(3);
	const cell tmp2 = GETITEM(2);
	PUSH(tmp);
	PUSH(tmp2);
}

/*
 *  pick        ( xu ... x1 x0 u -- xu ... x1 x0 xu )
 */

static void pick(void)
{
	const cell u = POP();
	if (dstackcnt >= u) {
		ucell tmp = dstack[dstackcnt - u];
		PUSH(tmp);
	} else {
		/* underrun */
	}
}


/*
 *  drop        ( x --  )
 */

static void drop(void)
{
	POP();
}

/*
 *  2drop       ( x1 x2 --  )
 */

static void twodrop(void)
{
	POP();
	POP();
}


/*
 *  nip         ( x1 x2 -- x2 )
 */

static void nip(void)
{
	const cell tmp = POP();
	POP();
	PUSH(tmp);
}


/*
 *  roll        ( xu ... x1 x0 u -- xu-1... x1 x0 xu )
 */

static void roll(void)
{
	const cell u = POP();
	if (dstackcnt >= u) {
		int i;
		const cell xu = dstack[dstackcnt - u];
		for (i = dstackcnt - u; i < dstackcnt; i++) {
			dstack[i] = dstack[i + 1];
		}
		dstack[dstackcnt] = xu;
	} else {
		/* Stack underrun */
	}
}


/*
 *  rot         ( x1 x2 x3 -- x2 x3 x1 )
 */

static void rot(void)
{
	const cell tmp = POP();
	const cell tmp2 = POP();
	const cell tmp3 = POP();
	PUSH(tmp2);
	PUSH(tmp);
	PUSH(tmp3);
}


/*
 *  -rot        ( x1 x2 x3 -- x3 x1 x2 )
 */

static void minusrot(void)
{
	const cell tmp = POP();
	const cell tmp2 = POP();
	const cell tmp3 = POP();
	PUSH(tmp);
	PUSH(tmp3);
	PUSH(tmp2);
}


/*
 *  swap        ( x1 x2 -- x2 x1 )
 */

static void swap(void)
{
	const cell tmp = POP();
	const cell tmp2 = POP();
	PUSH(tmp);
	PUSH(tmp2);
}


/*
 *  2swap       ( x1 x2 x3 x4 -- x3 x4 x1 x2 )
 */

static void twoswap(void)
{
	const cell tmp = POP();
	const cell tmp2 = POP();
	const cell tmp3 = POP();
	const cell tmp4 = POP();
	PUSH(tmp2);
	PUSH(tmp);
	PUSH(tmp4);
	PUSH(tmp3);
}


/*
 *  >r          ( x -- ) (R: -- x )
 */

static void tor(void)
{
	ucell tmp = POP();
#ifdef CONFIG_DEBUG_RSTACK
	printk("  >R: %x\n", tmp);
#endif
	PUSHR(tmp);
}


/*
 *  r>          ( -- x ) (R: x -- )
 */

static void rto(void)
{
	ucell tmp = POPR();
#ifdef CONFIG_DEBUG_RSTACK
	printk("  R>: %x\n", tmp);
#endif
	PUSH(tmp);
}


/*
 *  r@          ( -- x ) (R: x -- x )
 */

static void rfetch(void)
{
	PUSH(GETTORS());
}


/*
 *  depth       (  -- u )
 */

static void depth(void)
{
	const cell tmp = dstackcnt;
	PUSH(tmp);
}


/*
 *  depth!      ( ... u --  x1 x2 .. xu )
 */

static void depthwrite(void)
{
	ucell tmp = POP();
	dstackcnt = tmp;
}


/*
 *  rdepth      (  -- u )
 */

static void rdepth(void)
{
	const cell tmp = rstackcnt;
	PUSH(tmp);
}


/*
 *  rdepth!     ( u --  ) ( R: ... -- x1 x2 .. xu )
 */

static void rdepthwrite(void)
{
	ucell tmp = POP();
	rstackcnt = tmp;
}


/*
 *  +           ( nu1 nu2 -- sum )
 */

static void plus(void)
{
	cell tmp = POP() + POP();
	PUSH(tmp);
}


/*
 *  -           ( nu1 nu2 -- diff )
 */

static void minus(void)
{
	const cell nu2 = POP();
	const cell nu1 = POP();
	PUSH(nu1 - nu2);
}


/*
 *  *           ( nu1 nu2 -- prod )
 */

static void mult(void)
{
	const cell nu2 = POP();
	const cell nu1 = POP();
	PUSH(nu1 * nu2);
}


/*
 *  u*          ( u1 u2 -- prod )
 */

static void umult(void)
{
	const ucell tmp = (ucell) POP() * (ucell) POP();
	PUSH(tmp);
}


/*
 *  mu/mod      ( n1 n2 -- rem quot.l quot.h )
 */

static void mudivmod(void)
{
	const ucell b = POP();
	const ducell a = DPOP();
#ifdef NEED_FAKE_INT128_T
        if (a.hi != 0) {
            fprintf(stderr, "mudivmod called (0x%016llx %016llx / 0x%016llx)\n",
                    a.hi, a.lo, b);
            exit(-1);
        } else {
            ducell c;

            PUSH(a.lo % b);
            c.hi = 0;
            c.lo = a.lo / b;
            DPUSH(c);
        }
#else
	PUSH(a % b);
	DPUSH(a / b);
#endif
}


/*
 *  abs         ( n -- u )
 */

static void forthabs(void)
{
	const cell tmp = GETTOS();
	if (tmp < 0) {
		POP();
		PUSH(-tmp);
	}
}


/*
 *  negate      ( n1 -- n2 )
 */

static void negate(void)
{
	const cell tmp = POP();
	PUSH(-tmp);
}


/*
 *  max         ( n1 n2 -- n1|n2 )
 */

static void max(void)
{
	const cell tmp = POP();
	const cell tmp2 = POP();
	PUSH((tmp > tmp2) ? tmp : tmp2);
}


/*
 *  min         ( n1 n2 -- n1|n2 )
 */

static void min(void)
{
	const cell tmp = POP();
	const cell tmp2 = POP();
	PUSH((tmp < tmp2) ? tmp : tmp2);
}


/*
 *  lshift      ( x1 u -- x2 )
 */

static void lshift(void)
{
	const ucell u = POP();
	const ucell x1 = POP();
	PUSH(x1 << u);
}


/*
 *  rshift      ( x1 u -- x2 )
 */

static void rshift(void)
{
	const ucell u = POP();
	const ucell x1 = POP();
	PUSH(x1 >> u);
}


/*
 *  >>a         ( x1 u -- x2 ) ??
 */

static void rshifta(void)
{
	const cell u = POP();
	const cell x1 = POP();
	PUSH(x1 >> u);
}


/*
 *  and         ( x1 x2 -- x3 )
 */

static void and(void)
{
	const cell x1 = POP();
	const cell x2 = POP();
	PUSH(x1 & x2);
}


/*
 *  or          ( x1 x2 -- x3 )
 */

static void or(void)
{
	const cell x1 = POP();
	const cell x2 = POP();
	PUSH(x1 | x2);
}


/*
 *  xor         ( x1 x2 -- x3 )
 */

static void xor(void)
{
	const cell x1 = POP();
	const cell x2 = POP();
	PUSH(x1 ^ x2);
}


/*
 *  invert      ( x1 -- x2 )
 */

static void invert(void)
{
	const cell x1 = POP();
	PUSH(x1 ^ -1);
}


/*
 *  d+          ( d1 d2 -- d.sum )
 */

static void dplus(void)
{
	const dcell d2 = DPOP();
	const dcell d1 = DPOP();
#ifdef NEED_FAKE_INT128_T
        ducell c;

        if (d1.hi != 0 || d2.hi != 0) {
            fprintf(stderr, "dplus called (0x%016llx %016llx + 0x%016llx %016llx)\n",
                    d1.hi, d1.lo, d2.hi, d2.lo);
            exit(-1);
        }
        c.hi = 0;
        c.lo = d1.lo + d2.lo;
        DPUSH(c);
#else
	DPUSH(d1 + d2);
#endif
}


/*
 *  d-          ( d1 d2 -- d.diff )
 */

static void dminus(void)
{
	const dcell d2 = DPOP();
	const dcell d1 = DPOP();
#ifdef NEED_FAKE_INT128_T
        ducell c;

        if (d1.hi != 0 || d2.hi != 0) {
            fprintf(stderr, "dminus called (0x%016llx %016llx + 0x%016llx %016llx)\n",
                    d1.hi, d1.lo, d2.hi, d2.lo);
            exit(-1);
        }
        c.hi = 0;
        c.lo = d1.lo - d2.lo;
        DPUSH(c);
#else
	DPUSH(d1 - d2);
#endif
}


/*
 *  m*          ( ?? --  )
 */

static void mmult(void)
{
	const cell u2 = POP();
	const cell u1 = POP();
#ifdef NEED_FAKE_INT128_T
        ducell c;

        if (0) { // XXX How to detect overflow?
            fprintf(stderr, "mmult called (%016llx * 0x%016llx)\n", u1, u2);
            exit(-1);
        }
        c.hi = 0;
        c.lo = u1 * u2;
        DPUSH(c);
#else
	DPUSH((dcell) u1 * u2);
#endif
}


/*
 *  um*         ( u1 u2 -- d.prod )
 */

static void ummult(void)
{
	const ucell u2 = POP();
	const ucell u1 = POP();
#ifdef NEED_FAKE_INT128_T
        ducell c;

        if (0) { // XXX How to detect overflow?
            fprintf(stderr, "ummult called (%016llx * 0x%016llx)\n", u1, u2);
            exit(-1);
        }
        c.hi = 0;
        c.lo = u1 * u2;
        DPUSH(c);
#else
	DPUSH((ducell) u1 * u2);
#endif
}


/*
 *  @           ( a-addr -- x )
 */

static void fetch(void)
{
	const ucell *aaddr = (ucell *)cell2pointer(POP());
	PUSH(read_ucell(aaddr));
}


/*
 *  c@          ( addr -- byte )
 */

static void cfetch(void)
{
	const u8 *aaddr = (u8 *)cell2pointer(POP());
	PUSH(read_byte(aaddr));
}


/*
 *  w@          ( waddr -- w )
 */

static void wfetch(void)
{
	const u16 *aaddr = (u16 *)cell2pointer(POP());
	PUSH(read_word(aaddr));
}


/*
 *  l@          ( qaddr -- quad )
 */

static void lfetch(void)
{
	const u32 *aaddr = (u32 *)cell2pointer(POP());
	PUSH(read_long(aaddr));
}


/*
 *  !           ( x a-addr -- )
 */

static void store(void)
{
	const ucell *aaddr = (ucell *)cell2pointer(POP());
	const ucell x = POP();
#ifdef CONFIG_DEBUG_INTERNAL
	printk("!: %lx : %lx -> %lx\n", aaddr, read_ucell(aaddr), x);
#endif
	write_ucell(aaddr,x);
}


/*
 *  +!          ( nu a-addr -- )
 */

static void plusstore(void)
{
	const ucell *aaddr = (ucell *)cell2pointer(POP());
	const cell nu = POP();
	write_cell(aaddr,read_cell(aaddr)+nu);
}


/*
 *  c!          ( byte addr -- )
 */

static void cstore(void)
{
	const u8 *aaddr = (u8 *)cell2pointer(POP());
	const ucell byte = POP();
#ifdef CONFIG_DEBUG_INTERNAL
	printk("c!: %x = %x\n", aaddr, byte);
#endif
	write_byte(aaddr, byte);
}


/*
 *  w!          ( w waddr -- )
 */

static void wstore(void)
{
	const u16 *aaddr = (u16 *)cell2pointer(POP());
	const u16 word = POP();
	write_word(aaddr, word);
}


/*
 *  l!          ( quad qaddr -- )
 */

static void lstore(void)
{
	const u32 *aaddr = (u32 *)cell2pointer(POP());
	const u32 longval = POP();
	write_long(aaddr, longval);
}


/*
 *  =           ( x1 x2 -- equal? )
 */

static void equals(void)
{
	cell tmp = (POP() == POP());
	PUSH(-tmp);
}


/*
 *  >           ( n1 n2 -- greater? )
 */

static void greater(void)
{
	cell tmp = ((cell) POP() < (cell) POP());
	PUSH(-tmp);
}


/*
 *  <           ( n1 n2 -- less? )
 */

static void less(void)
{
	cell tmp = ((cell) POP() > (cell) POP());
	PUSH(-tmp);
}


/*
 *  u>          ( u1 u2 -- unsigned-greater? )
 */

static void ugreater(void)
{
	cell tmp = ((ucell) POP() < (ucell) POP());
	PUSH(-tmp);
}


/*
 *  u<          ( u1 u2 -- unsigned-less? )
 */

static void uless(void)
{
	cell tmp = ((ucell) POP() > (ucell) POP());
	PUSH(-tmp);
}


/*
 *  sp@         (  -- stack-pointer )
 */

static void spfetch(void)
{
	// FIXME this can only work if the stack pointer
	// is within range.
	ucell tmp = pointer2cell(&(dstack[dstackcnt]));
	PUSH(tmp);
}


/*
 *  move        ( src-addr dest-addr len -- )
 */

static void fmove(void)
{
	ucell count = POP();
	void *dest = (void *)cell2pointer(POP());
	const void *src = (const void *)cell2pointer(POP());
	memmove(dest, src, count);
}


/*
 *  fill        ( addr len byte -- )
 */

static void ffill(void)
{
	ucell value = POP();
	ucell count = POP();
	void *src = (void *)cell2pointer(POP());
	memset(src, value, count);
}


/*
 *  unaligned-w@  ( addr -- w )
 */

static void unalignedwordread(void)
{
	const unsigned char *addr = (const unsigned char *) cell2pointer(POP());
	PUSH(unaligned_read_word(addr));
}


/*
 *  unaligned-w!  ( w addr -- )
 */

static void unalignedwordwrite(void)
{
	const unsigned char *addr = (const unsigned char *) cell2pointer(POP());
	u16 w = POP();
	unaligned_write_word(addr, w);
}


/*
 *  unaligned-l@  ( addr -- quad )
 */

static void unalignedlongread(void)
{
	const unsigned char *addr = (const unsigned char *) cell2pointer(POP());
	PUSH(unaligned_read_long(addr));
}


/*
 *  unaligned-l!  ( quad addr -- )
 */

static void unalignedlongwrite(void)
{
	unsigned char *addr = (unsigned char *) cell2pointer(POP());
	u32 l = POP();
	unaligned_write_long(addr, l);
}

/*
 *  here        (  -- dictionary-pointer )
 */

static void here(void)
{
	PUSH(pointer2cell(dict) + dicthead);
#ifdef CONFIG_DEBUG_INTERNAL
	printk("here: %x\n", pointer2cell(dict) + dicthead);
#endif
}

/*
 *  here!       ( new-dict-pointer -- )
 */

static void herewrite(void)
{
	ucell tmp = POP(); /* converted pointer */
	dicthead = tmp - pointer2cell(dict);
#ifdef CONFIG_DEBUG_INTERNAL
	printk("here!: new value: %x\n", tmp);
#endif
}


/*
 *   emit       ( char --  )
 */

static void emit(void)
{
	cell tmp = POP();
#ifndef FCOMPILER
	putchar(tmp);
#else
       	put_outputbyte(tmp);
#endif
}


/*
 *   key?       (  -- pressed? )
 */

static void iskey(void)
{
	PUSH((cell) availchar());
}


/*
 *   key        (  -- char )
 */

static void key(void)
{
	while (!availchar());
#ifdef FCOMPILER
	PUSH(get_inputbyte());
#else
	PUSH(getchar());
#endif
}


/*
 *   ioc@       ( reg -- val )
 */

static void iocfetch(void)
{
#ifndef FCOMPILER
	cell reg = POP();
	PUSH(inb(reg));
#else
        (void)POP();
        PUSH(0);
#endif
}


/*
 *   iow@       ( reg -- val )
 */

static void iowfetch(void)
{
#ifndef FCOMPILER
	cell reg = POP();
	PUSH(inw(reg));
#else
        (void)POP();
        PUSH(0);
#endif
}

/*
 *   iol@       ( reg -- val )
 */

static void iolfetch(void)
{
#ifndef FCOMPILER
	cell reg = POP();
	PUSH(inl(reg));
#else
        (void)POP();
        PUSH(0);
#endif
}


/*
 *   ioc!       ( val reg --  )
 */

static void iocstore(void)
{
#ifndef FCOMPILER
	cell reg = POP();
	cell val = POP();

	outb(reg, val);
#else
        (void)POP();
        (void)POP();
#endif
}


/*
 *   iow!       ( val reg --  )
 */

static void iowstore(void)
{
#ifndef FCOMPILER
	cell reg = POP();
	cell val = POP();

	outw(reg, val);
#else
        (void)POP();
        (void)POP();
#endif
}


/*
 *   iol!       ( val reg --  )
 */

static void iolstore(void)
{
#ifndef FCOMPILER
	ucell reg = POP();
	ucell val = POP();

	outl(reg, val);
#else
        (void)POP();
        (void)POP();
#endif
}

/*
 *   i         ( -- i )
 */

static void loop_i(void)
{
	PUSH(rstack[rstackcnt]);
}

/*
 *   j         ( -- i )
 */

static void loop_j(void)
{
	PUSH(rstack[rstackcnt - 2]);
}
