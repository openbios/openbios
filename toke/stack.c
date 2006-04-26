/*
 *                     OpenBIOS - free your system! 
 *                         ( FCode tokenizer )
 *                          
 *  stack.c - data and return stack handling for fcode tokenizer.
 *  
 *  This program is part of a free implementation of the IEEE 1275-1994 
 *  Standard for Boot (Initialization Configuration) Firmware.
 *
 *  Copyright (C) 2001-2005 by Stefan Reinauer <stepan@openbios.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA, 02110-1301 USA
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "toke.h"
#include "stack.h"


#define GUARD_STACK
#define EXIT_STACKERR

#ifdef GLOBALSTACK
#define STATIC static
#else
#define STATIC
#endif
STATIC long *dstack,*startdstack,*enddstack;
#undef STATIC

/* internal stack functions */

int init_stack(void)
{
	startdstack=enddstack=malloc(MAX_ELEMENTS*sizeof(long));
	enddstack+=MAX_ELEMENTS;
	dstack=enddstack;
	return 0;
}

#ifdef GLOBALSTACK 

#ifdef GUARD_STACK
static void stackerror(int stat)
{
  	printf ("FATAL: stack %sflow\n",
		(stat)?"under":"over" );
#ifdef EXIT_STACKERR
	exit(-1);
#endif
}
#endif

void dpush(long data)
{
#ifdef DEBUG_DSTACK
	printf("dpush: sp=%p, data=0x%lx, ", dstack, data);
#endif
	*(--dstack)=data;
#ifdef GUARD_STACK
	if (dstack<startdstack) stackerror(0);
#endif
}

long dpop(void)
{
  	long val;
#ifdef DEBUG_DSTACK
	printf("dpop: sp=%p, data=0x%lx, ",dstack, *dstack);
#endif
	val=*(dstack++);
#ifdef GUARD_STACK
	if (dstack>enddstack) stackerror(1);
#endif
	return val;
}

long dget(void)
{
	return *(dstack);
}
#endif

/* Stack helper functions */

u8 *get_stackstring(void)
{
	long size;
	u8   *fstring, *retstring;

	size=dpop();
	fstring=(u8 *)dpop();

	retstring=malloc(size+1);
	strncpy((char *)retstring, (const char *)fstring, size);
	retstring[size]=0;

	return retstring;
}


