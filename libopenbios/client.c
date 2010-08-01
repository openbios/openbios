/*
 *   Creation Date: <2003/11/25 14:29:08 samuel>
 *   Time-stamp: <2004/03/27 01:13:53 samuel>
 *
 *	<client.c>
 *
 *	OpenFirmware client interface
 *
 *   Copyright (C) 2003, 2004 Samuel Rydh (samuel@ibrium.se)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   version 2
 *
 */

#include "config.h"
#include "libopenbios/bindings.h"
#include "libopenbios/of.h"

/* Uncomment to enable debug printout of client interface calls */
//#define DEBUG_CIF
//#define DUMP_IO

/* OF client interface. r3 points to the argument array. On return,
 * r3 should contain 0==true or -1==false. r4-r12,cr0,cr1 may
 * be modified freely.
 *
 * -1 should only be returned if the control transfer to OF fails
 * (it doesn't) or if the function is unimplemented.
 */

#define PROM_MAX_ARGS	10
typedef struct prom_args {
        const char 	*service;
        long 		nargs;
        long 		nret;
        unsigned long	args[PROM_MAX_ARGS];
} prom_args_t;

#ifdef DEBUG_CIF
static void memdump(const char *mem, unsigned long size)
{
	int i;
	
	if (size == (unsigned long) -1)
		return;

	for (i = 0; i < size; i += 16) {
		int j;

		printk("0x%08lx ", (unsigned long)mem + i);

		for (j = 0; j < 16 && i + j < size; j++)
			printk(" %02x", *(unsigned char*)(mem + i + j));

		for ( ; j < 16; j++)
			printk(" __");

		printk("  ");

		for (j = 0; j < 16 && i + j < size; j++) {
			unsigned char c = *(mem + i + j);
			if (isprint(c))
				printk("%c", c);
			else
				printk(".");
		}
		printk("\n");
	}
}

static void dump_service(prom_args_t *pb)
{
	int i;
	if (strcmp(pb->service, "test") == 0) {
		printk("test(\"%s\") = ", (char*)pb->args[0]);
	} else if (strcmp(pb->service, "peer") == 0) {
		printk("peer(0x%08lx) = ", pb->args[0]);
	} else if (strcmp(pb->service, "child") == 0) {
		printk("child(0x%08lx) = ", pb->args[0]);
	} else if (strcmp(pb->service, "parent") == 0) {
		printk("parent(0x%08lx) = ", pb->args[0]);
	} else if (strcmp(pb->service, "instance-to-package") == 0) {
		printk("instance-to-package(0x%08lx) = ", pb->args[0]);
	} else if (strcmp(pb->service, "getproplen") == 0) {
		printk("getproplen(0x%08lx, \"%s\") = ",
			pb->args[0], (char*)pb->args[1]);
	} else if (strcmp(pb->service, "getprop") == 0) {
		printk("getprop(0x%08lx, \"%s\", 0x%08lx, %ld) = ",
			pb->args[0], (char*)pb->args[1],
			pb->args[2], pb->args[3]);
	} else if (strcmp(pb->service, "nextprop") == 0) {
		printk("nextprop(0x%08lx, \"%s\", 0x%08lx) = ",
			pb->args[0], (char*)pb->args[1], pb->args[2]);
	} else if (strcmp(pb->service, "setprop") == 0) {
		printk("setprop(0x%08lx, \"%s\", 0x%08lx, %ld)\n",
			pb->args[0], (char*)pb->args[1],
			pb->args[2], pb->args[3]);
		memdump((char*)pb->args[2], pb->args[3]);
		printk(" = ");
	} else if (strcmp(pb->service, "canon") == 0) {
		printk("canon(\"%s\", 0x%08lx, %ld)\n",
			(char*)pb->args[0], pb->args[1], pb->args[2]);
	} else if (strcmp(pb->service, "finddevice") == 0) {
		printk("finddevice(\"%s\") = ", (char*)pb->args[0]);
	} else if (strcmp(pb->service, "instance-to-path") == 0) {
		printk("instance-to-path(0x%08lx, 0x%08lx, %ld) = ",
			pb->args[0], pb->args[1], pb->args[2]);
	} else if (strcmp(pb->service, "package-to-path") == 0) {
		printk("package-to-path(0x%08lx, 0x%08lx, %ld) = ",
			pb->args[0], pb->args[1], pb->args[2]);
	} else if (strcmp(pb->service, "open") == 0) {
		printk("open(\"%s\") = ", (char*)pb->args[0]);
	} else if (strcmp(pb->service, "close") == 0) {
		printk("close(0x%08lx)\n", pb->args[0]);
	} else if (strcmp(pb->service, "read") == 0) {
#ifdef DUMP_IO
		printk("read(0x%08lx, 0x%08lx, %ld) = ",
			pb->args[0], pb->args[1], pb->args[2]);
#endif
	} else if (strcmp(pb->service, "write") == 0) {
#ifdef DUMP_IO
		printk("write(0x%08lx, 0x%08lx, %ld)\n",
			pb->args[0], pb->args[1], pb->args[2]);
		memdump((char*)pb->args[1], pb->args[2]);
		printk(" = ");
#endif
	} else if (strcmp(pb->service, "seek") == 0) {
#ifdef DUMP_IO
		printk("seek(0x%08lx, 0x%08lx, 0x%08lx) = ",
			pb->args[0], pb->args[1], pb->args[2]);
#endif
	} else if (strcmp(pb->service, "claim") == 0) {
		printk("claim(0x8%lx, %ld, %ld) = ",
			pb->args[0], pb->args[1], pb->args[2]);
	} else if (strcmp(pb->service, "release") == 0) {
		printk("release(0x8%lx, %ld)\n",
			pb->args[0], pb->args[1]);
	} else if (strcmp(pb->service, "boot") == 0) {
		printk("boot \"%s\"\n", (char*)pb->args[0]);
	} else if (strcmp(pb->service, "enter") == 0) {
		printk("enter()\n");
	} else if (strcmp(pb->service, "exit") == 0) {
		printk("exit()\n");
	} else if (strcmp(pb->service, "test-method") == 0) {
		printk("test-method(0x%08lx, \"%s\") = ",
			pb->args[0], (char*)pb->args[1]);
	} else {
		printk("of_client_interface: %s ", pb->service );
		for( i = 0; i < pb->nargs; i++ )
			printk("%lx ", pb->args[i] );
		printk("\n");
	}
}

static void dump_return(prom_args_t *pb)
{
	int i;
	if (strcmp(pb->service, "test") == 0) {
		printk(" %ld\n", pb->args[pb->nargs]);
	} else if (strcmp(pb->service, "peer") == 0) {
		printk("0x%08lx\n", pb->args[pb->nargs]);
	} else if (strcmp(pb->service, "child") == 0) {
		printk("0x%08lx\n", pb->args[pb->nargs]);
	} else if (strcmp(pb->service, "parent") == 0) {
		printk("0x%08lx\n", pb->args[pb->nargs]);
	} else if (strcmp(pb->service, "instance-to-package") == 0) {
		printk("0x%08lx\n", pb->args[pb->nargs]);
	} else if (strcmp(pb->service, "getproplen") == 0) {
		printk("0x%08lx\n", pb->args[pb->nargs]);
	} else if (strcmp(pb->service, "getprop") == 0) {
		printk("%ld\n", pb->args[3]);
		memdump((char*)pb->args[2], pb->args[3]);
	} else if (strcmp(pb->service, "nextprop") == 0) {
		printk("%ld\n", pb->args[pb->nargs]);
		memdump((char*)pb->args[2], pb->args[pb->nargs]);
	} else if (strcmp(pb->service, "setprop") == 0) {
		printk("%ld\n", pb->args[pb->nargs]);
	} else if (strcmp(pb->service, "canon") == 0) {
		printk("%ld\n", pb->args[pb->nargs]);
		memdump((char*)pb->args[1], pb->args[pb->nargs]);
	} else if (strcmp(pb->service, "finddevice") == 0) {
		printk("0x%08lx\n", pb->args[pb->nargs]);
	} else if (strcmp(pb->service, "instance-to-path") == 0) {
		printk("%ld\n", pb->args[pb->nargs]);
		memdump((char*)pb->args[1], pb->args[pb->nargs]);
	} else if (strcmp(pb->service, "package-to-path") == 0) {
		printk("%ld\n", pb->args[pb->nargs]);
		memdump((char*)pb->args[1], pb->args[pb->nargs]);
	} else if (strcmp(pb->service, "open") == 0) {
		printk("0x%08lx\n", pb->args[pb->nargs]);
	} else if (strcmp(pb->service, "close") == 0) {
		/* do nothing */
	} else if (strcmp(pb->service, "read") == 0) {
#ifdef DUMP_IO
		printk("%ld\n", pb->args[pb->nargs]);
		memdump((char*)pb->args[1], pb->args[pb->nargs]);
#endif
	} else if (strcmp(pb->service, "write") == 0) {
#ifdef DUMP_IO
		printk("%ld\n", pb->args[pb->nargs]);
#endif
	} else if (strcmp(pb->service, "seek") == 0) {
#ifdef DUMP_IO
		printk("%ld\n", pb->args[pb->nargs]);
#endif
	} else if (strcmp(pb->service, "claim") == 0) {
		printk("0x%08lx\n", pb->args[pb->nargs]);
	} else if (strcmp(pb->service, "release") == 0) {
		/* do nothing */
	} else if (strcmp(pb->service, "boot") == 0) {
		/* do nothing */
	} else if (strcmp(pb->service, "enter") == 0) {
		/* do nothing */
	} else if (strcmp(pb->service, "exit") == 0) {
		/* do nothing */
	} else if (strcmp(pb->service, "test-method") == 0) {
		printk("0x%08lx\n", pb->args[pb->nargs]);
	} else {
		printk("of_client_interface return:");
		for (i = 0; i < pb->nret; i++) {
			printk("%lx ", pb->args[pb->nargs + i]);
		}
		printk("\n");
	}
}
#endif

/* call-method, interpret */
static int
handle_calls( prom_args_t *pb )
{
	int i, dstacksave = dstackcnt;
        long val;

#ifdef DEBUG_CIF
        printk("%s %s ([%ld] -- [%ld])\n", pb->service, (char *)pb->args[0],
               pb->nargs, pb->nret);
#endif

	for( i=pb->nargs-1; i>=0; i-- )
		PUSH( pb->args[i] );

	push_str( pb->service );
	fword("client-call-iface");

	/* Drop the return code from client-call-iface (status is handled by the 
	   catch result which is the first parameter below) */
	POP();

	for( i=0; i<pb->nret; i++ ) {
                val = POP();
		pb->args[pb->nargs + i] = val;

		/* don't pop args if an exception occured */
		if( !i && val )
			break;
	}

#ifdef DEBUG_CIF
	/* useful for debug but not necessarily an error */
	if( i != pb->nret || dstackcnt != dstacksave ) {
                printk("%s '%s': possible argument error (%ld--%ld) got %d\n",
		       pb->service, (char*)pb->args[0], pb->nargs-2, pb->nret, i );
	}

        printk("handle_calls return: ");
        for (i = 0; i < pb->nret; i++) {
            printk("%lx ", pb->args[pb->nargs + i]);
        }
        printk("\n");
#endif

	dstackcnt = dstacksave;
	return 0;
}

int
of_client_interface( int *params )
{
	prom_args_t *pb = (prom_args_t*)params;
	int val, i, dstacksave;

	if( pb->nargs < 0 || pb->nret < 0 ||
            pb->nargs + pb->nret > PROM_MAX_ARGS)
		return -1;

#ifdef DEBUG_CIF
	dump_service(pb);
#endif

	/* call-method exceptions are special */
	if( !strcmp("call-method", pb->service) || !strcmp("interpret", pb->service) )
		return handle_calls( pb );

	dstacksave = dstackcnt;
	for( i=pb->nargs-1; i>=0; i-- )
		PUSH( pb->args[i] );

	push_str( pb->service );
	fword("client-iface");

	if( (val=POP()) ) {
		dstackcnt = dstacksave;
		if( val == -1 )
			printk("Unimplemented service %s ([%ld] -- [%ld])\n",
			       pb->service, pb->nargs, pb->nret );
		return -1;
	}

	for( i=0; i<pb->nret ; i++ )
		pb->args[pb->nargs + i] = POP();

	if( dstackcnt != dstacksave ) {
#ifdef DEBUG_CIF
		printk("service %s: possible argument error (%d %d)\n",
		       pb->service, i, dstackcnt - dstacksave );
#endif
		/* Some clients request less parameters than the CIF method
		returns, e.g. getprop with OpenSolaris. Hence we drop any
		stack parameters after issuing a warning above */
		dstackcnt = dstacksave;
	}

#ifdef DEBUG_CIF
	dump_return(pb);
#endif
	return 0;
}
