/*
 *   Creation Date: <2004/08/28 18:38:22 greg>
 *   Time-stamp: <2004/08/28 18:38:22 greg>
 *
 *	<qemu.c>
 *
 *   Copyright (C) 2004, Greg Watson
 *
 *   derived from mol.c
 *
 *   Copyright (C) 2003, 2004 Samuel Rydh (samuel@ibrium.se)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   version 2
 *
 */

#include "openbios/config.h"
#include "openbios/kernel.h"
#include "openbios/nvram.h"
#include "libc/vsprintf.h"
#include "libc/string.h"
#include "libc/byteorder.h"
#include "qemu/qemu.h"
#include <stdarg.h>

#define UART_BASE 0x3f8

// FIXME
unsigned long virt_offset = 0;

//#define DUMP_NVRAM

void
exit( int status )
{
	for (;;);
}

void
fatal_error( const char *err )
{
	printk("Fatal error: %s\n", err );
	exit(0);
}

void
panic( const char *err )
{
	printk("Panic: %s\n", err );
	exit(0);

	/* won't come here... this keeps the gcc happy */
	for( ;; )
		;
}


/************************************************************************/
/*	print using OSI interface					*/
/************************************************************************/

static int do_indent;

int
printk( const char *fmt, ... )
{
	char *p, buf[1024];	/* XXX: no buffer overflow protection... */
	va_list args;
	int i;

	va_start(args, fmt);
	i=vsprintf(buf,fmt,args);
	va_end(args);

	for( p=buf; *p; p++ ) {
		if( *p == '\n' )
			do_indent = 0;
		if( do_indent++ == 1 ) {
			putchar( '>' );
			putchar( '>' );
			putchar( ' ' );
		}
		putchar( *p );
	}
	return i;
}


/************************************************************************/
/*	TTY iface							*/
/************************************************************************/

static int ttychar = -1;

static int
tty_avail( void )
{
	return 1;
}

static int
tty_putchar( int c )
{
	if( tty_avail() ) {
		while (!(inb(UART_BASE + 0x05) & 0x20))
			;
		outb(c, UART_BASE);
		while (!(inb(UART_BASE + 0x05) & 0x40))
			;
	}
	return c;
}

int
availchar( void )
{
	if( !tty_avail() )
		return 0;

	if( ttychar < 0 )
		ttychar = inb(UART_BASE);
	return (ttychar >= 0);
}

int
getchar( void )
{
	int ch;

	if( !tty_avail() )
		return 0;

	if( ttychar < 0 )
		return inb(UART_BASE);
	ch = ttychar;
	ttychar = -1;
	return ch;
}

int
putchar( int c )
{
	if (c == '\n')
		tty_putchar('\r');
	return tty_putchar(c);
}


/************************************************************************/
/*	briQ specific stuff						*/
/************************************************************************/

#define IO_NVRAM_SIZE   0x00020000
#define IO_NVRAM_OFFSET 0x00060000

static char *nvram;

void macio_nvram_init(char *path, uint32_t addr)
{
	phandle_t chosen, aliases;
	phandle_t dnode;
	int props[2];
	char buf[64];

	nvram = (char*)addr + IO_NVRAM_OFFSET;
	sprintf(buf, "%s/nvram", path);
	nvram_init(buf);
	dnode = find_dev(buf);
	set_int_property(dnode, "#bytes", IO_NVRAM_SIZE >> 4);
	set_property(dnode, "compatible", "nvram,flash", 12);
	props[0] = __cpu_to_be32(IO_NVRAM_OFFSET);
	props[1] = __cpu_to_be32(IO_NVRAM_SIZE);
	set_property(dnode, "reg", &props, sizeof(props));
	set_property(dnode, "device_type", "nvram", 6);

	chosen = find_dev("/chosen");
	set_int_property(chosen, "nvram", dnode);

	aliases = find_dev("/aliases");
	set_property(aliases, "nvram", buf, strlen(buf) + 1);
}

#ifdef DUMP_NVRAM
void
dump_nvram(void)
{
  int i, j;
  for (i = 0; i < 10; i++)
    {
      for (j = 0; j < 16; j++)
      printk ("%02x ", nvram[(i*16+j)<<4]);
      printk (" ");
      for (j = 0; j < 16; j++)
        if (isprint(nvram[(i*16+j)<<4]))
            printk("%c", nvram[(i*16+j)<<4]);
        else
          printk(".");
      printk ("\n");
      }
}
#endif


int
arch_nvram_size( void )
{
	return IO_NVRAM_SIZE>>4;
}

void
arch_nvram_put( char *buf )
{
	int i;
	for (i=0; i<IO_NVRAM_SIZE>>4; i++)
		nvram[i<<4]=buf[i];
#ifdef DUMP_NVRAM
	printk("new nvram:\n");
	dump_nvram();
#endif
}

void
arch_nvram_get( char *buf )
{
	int i;
	for (i=0; i<IO_NVRAM_SIZE>>4; i++)
		buf[i]=nvram[i<<4];

#ifdef DUMP_NVRAM
	printk("current nvram:\n");
	dump_nvram();
#endif
}
