/* lib.c
 * tag: simple function library
 *
 * Copyright (C) 2003 Stefan Reinauer
 *
 * See the file "COPYING" for further information about
 * the copyright and warranty status of this work.
 */

#include "openbios/config.h"
#include "asm/types.h"
#include <stdarg.h>
#include "libc/stdlib.h"
#include "libc/vsprintf.h"
#include "openbios/kernel.h"

/* Format a string and print it on the screen, just like the libc
 * function printf. 
 */
int printk( const char *fmt, ... )
{
	char *p, buf[512];	/* XXX: no buffer overflow protection... */
	va_list args;
	int i;

	va_start(args, fmt);
	i=vsprintf(buf,fmt,args);
	va_end(args);

	for( p=buf; *p; p++ )
		putchar(*p);
	return i;
}

void *malloc(int size)
{
    extern struct mem cmem;

    return mem_alloc(&cmem, size, 4);
}

void free(void *ptr)
{
	/* Nothing yet */
}


