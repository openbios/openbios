/*
 *   Creation Date: <2004/08/28 18:38:22 greg>
 *   Time-stamp: <2004/08/28 18:38:22 greg>
 *
 *	<init.c>
 *
 *	Initialization for qemu
 *
 *   Copyright (C) 2004 Greg Watson
 *   Copyright (C) 2005 Stefan Reinauer
 *
 *   based on mol/init.c:
 *
 *   Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Samuel & David Rydh
 *      (samuel@ibrium.se, dary@lindesign.se)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation
 *
 */

#include "openbios/config.h"
#include "openbios/bindings.h"
#include "openbios/pci.h"
#include "openbios/nvram.h"
#include "qemu/qemu.h"
#include "ofmem.h"
#include "openbios-version.h"

extern void unexpected_excep( int vector );
extern void ob_ide_init( void );
extern void ob_pci_init( void );
extern void setup_timers( void );

void
unexpected_excep( int vector )
{
	printk("openbios panic: Unexpected exception %x\n", vector );
	for( ;; )
		;
}

enum {
    ARCH_PREP = 0,
    ARCH_MAC99,
    ARCH_HEATHROW,
};

static pci_arch_t known_arch[] = {
	[ARCH_PREP] = { "PREP", 0x1057, 0x4801, 0x80800000, 0x800c0000,
			0x80000000, 0x00100000, 0xf0000000, 0x10000000,
			0x80000000, 0x00010000, 0x00000000, 0x00400000,
		      },
	[ARCH_MAC99] = { "MAC99", 0x106b, 0x001F, 0xf2800000, 0xf2c00000,
			  0xf2000000, 0x02000000, 0x80000000, 0x10000000,
			  0xf2000000, 0x00800000, 0x00000000, 0x01000000,
		       },
	[ARCH_HEATHROW] = { "HEATHROW", 0x1057, 0x0002, 0xfec00000, 0xfee00000,
			    0x80000000, 0x7f000000, 0x80000000, 0x01000000,
			    0xfe000000, 0x00800000, 0xfd000000, 0x01000000,
			  },
};
uint32_t isa_io_base;

void
entry( void )
{
	arch = &known_arch[ARCH_HEATHROW];
	isa_io_base = arch->io_base;
#ifdef CONFIG_DEBUG_CONSOLE
#ifdef CONFIG_DEBUG_CONSOLE_SERIAL
	serial_init();
#endif
#endif
	printk("\n");
	printk("=============================================================\n");
	printk("OpenBIOS %s [%s]\n", OPENBIOS_RELEASE, OPENBIOS_BUILD_DATE );

	ofmem_init();
	initialize_forth();
	/* won't return */

	printk("of_startup returned!\n");
	for( ;; )
		;
}

static void
setenv( char *env, char *value )
{
	push_str( value );
	push_str( env );
	fword("$setenv");
}

void
arch_of_init( void )
{
#if USE_RTAS
	phandle_t ph;
#endif
	int autoboot;

	devtree_init();
	modules_init();
	setup_timers();
#ifdef CONFIG_DRIVER_PCI
	ob_pci_init();
#endif
#ifdef CONFIG_DRIVER_IDE
        ob_ide_init();
#endif

	node_methods_init();

#if USE_RTAS
	if( !(ph=find_dev("/rtas")) )
		printk("Warning: No /rtas node\n");
	else {
		ulong size = 0x1000;
		while( size < (ulong)of_rtas_end - (ulong)of_rtas_start )
			size *= 2;
		set_property( ph, "rtas-size", (char*)&size, sizeof(size) );
	}
#endif
#if 0
	/* tweak boot settings */
	autoboot = !!getbool("autoboot?");
#endif
	autoboot = 0;
	if( !autoboot )
		printk("Autobooting disabled - dropping into OpenFirmware\n");
	setenv("auto-boot?", autoboot ? "true" : "false" );
	setenv("boot-command", "qemuboot");

#if 0
	if( getbool("tty-interface?") == 1 )
#endif
		fword("activate-tty-interface");

	/* hack */
	device_end();
	bind_func("qemuboot", boot );
}
