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
        char *p, buf[512];
	va_list args;
	int i;

	va_start(args, fmt);
        i = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	for( p=buf; *p; p++ )
		putchar(*p);
	return i;
}

typedef struct alloc_desc {
	struct alloc_desc 	*next;
	int			size;			/* size (including) this struct */
} alloc_desc_t;

typedef struct mem_range {
	struct mem_range	*next;
	ulong			start;
	ulong			size;
} range_t;

typedef struct trans {
	struct trans		*next;
	ulong			virt;			/* chain is sorted by virt */
	ulong			size;
	ulong			phys;
	int			mode;
} translation_t;

static struct {
	char 			*next_malloc;
        int                     left;
	alloc_desc_t		*mfree;			/* list of free malloc blocks */

	range_t			*phys_range;
	range_t			*virt_range;

	translation_t		*trans;			/* this is really a translation_t */
} ofmem;
#define ALLOC_BLOCK (64 * 1024)

void *malloc(int size)
{
	alloc_desc_t *d, **pp;
	char *ret;
        extern struct mem cmem;

	if( !size )
		return NULL;

        size = (size + 7) & ~7;
	size += sizeof(alloc_desc_t);

	/* look in the freelist */
	for( pp=&ofmem.mfree; *pp && (**pp).size < size; pp = &(**pp).next )
		;

	/* waste at most 4K by taking an entry from the freelist */
	if( *pp && (**pp).size < size + 0x1000 ) {
		ret = (char*)*pp + sizeof(alloc_desc_t);
		memset( ret, 0, (**pp).size - sizeof(alloc_desc_t) );
		*pp = (**pp).next;
		return ret;
	}

	if( !ofmem.next_malloc || ofmem.left < size) {
                unsigned long alloc_size = ALLOC_BLOCK;
                if (size > ALLOC_BLOCK)
                    alloc_size = size;
                // Recover possible leftover
                if ((size_t)ofmem.left > sizeof(alloc_desc_t) + 4) {
                    alloc_desc_t *d_leftover;

                    d_leftover = (alloc_desc_t*)ofmem.next_malloc;
                    d_leftover->size = ofmem.left - sizeof(alloc_desc_t);
                    free((void *)((unsigned long)d_leftover +
                                  sizeof(alloc_desc_t)));
                }

                ofmem.next_malloc = mem_alloc(&cmem, alloc_size, 4);
                ofmem.left = alloc_size;
        }

	if( ofmem.left < size) {
		printk("out of malloc memory (%x)!\n", size );
		return NULL;
	}
	d = (alloc_desc_t*) ofmem.next_malloc;
	ofmem.next_malloc += size;
	ofmem.left -= size;

	d->next = NULL;
	d->size = size;

	ret = (char*)d + sizeof(alloc_desc_t);
	memset( ret, 0, size - sizeof(alloc_desc_t) );
	return ret;
}

void free(void *ptr)
{
	alloc_desc_t **pp, *d;

	/* it is legal to free NULL pointers (size zero allocations) */
	if( !ptr )
		return;

	d = (alloc_desc_t*)((unsigned long)ptr - sizeof(alloc_desc_t));
	d->next = ofmem.mfree;

	/* insert in the (sorted) freelist */
	for( pp=&ofmem.mfree; *pp && (**pp).size < d->size ; pp = &(**pp).next )
		;
	d->next = *pp;
	*pp = d;
}

void *
realloc( void *ptr, size_t size )
{
	alloc_desc_t *d = (alloc_desc_t*)((unsigned long)ptr - sizeof(alloc_desc_t));
	char *p;

	if( !ptr )
		return malloc( size );
	if( !size ) {
		free( ptr );
		return NULL;
	}
	p = malloc( size );
	memcpy( p, ptr, MIN(d->size - sizeof(alloc_desc_t),size) );
	free( ptr );
	return p;
}
