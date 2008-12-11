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

#include "openbios/config.h"
#include "openbios/bindings.h"
#include "openbios/of.h"

/* OF client interface. r3 points to the argument array. On return,
 * r3 should contain 0==true or -1==false. r4-r12,cr0,cr1 may
 * be modified freely.
 *
 * -1 should only be returned if the control transfer to OF fails
 * (it doesn't) or if the function is unimplemented.
 */

typedef struct prom_args {
        const char 	*service;
        long 		nargs;
        long 		nret;
        ulong 		args[10];		/* MAX NUM ARGS! */
} prom_args_t;


/* call-method, interpret */
static int
handle_calls( prom_args_t *pb )
{
	int i, dstacksave = dstackcnt;

	/* printk("%s ([%d] -- [%d])\n", pb->service, pb->nargs, pb->nret ); */

	for( i=pb->nargs-1; i>=0; i-- )
		PUSH( pb->args[i] );

	push_str( pb->service );
	fword("client-call-iface");

	for( i=0; i<pb->nret && dstackcnt > dstacksave; i++ ) {
		int val = POP();
		pb->args[pb->nargs + i] = val;

		/* don't pop args if an exception occured */
		if( !i && val )
			break;
	}
#if 0
	/* useful for debug but not necessarily an error */
	if( i != pb->nret || dstacksave != dstacksave ) {
		printk("%s '%s': possible argument error (%d--%d) got %d\n",
		       pb->service, (char*)pb->args[0], pb->nargs-2, pb->nret, i );
	}
#endif

	dstackcnt = dstacksave;
	return 0;
}

int
of_client_interface( int *params )
{
	prom_args_t *pb = (prom_args_t*)params;
	int val, i, dstacksave;

	if( pb->nargs < 0 || pb->nret < 0 )
		return -1;
#if 0
	printk("of_client_interface: %s ", pb->service );
	for( i=0; i<pb->nargs; i++ )
		printk("%lx ", pb->args[i] );
	printk("\n");
#endif

	/* call-method exceptions are special */
	if( !strcmp("call-method", pb->service) || !strcmp("interpret", pb->service) )
		return handle_calls( pb );

	dstacksave = dstackcnt;
	for( i=0; i<pb->nargs; i++ )
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

	for( i=0; i<pb->nret && dstackcnt > dstacksave ; i++ )
		pb->args[pb->nargs + i] = POP();

	if( i != pb->nret || dstackcnt != dstacksave ) {
		printk("service %s: argument count error (%d %d)\n",
		       pb->service, i, dstackcnt - dstacksave );
		dstackcnt = dstacksave;
	}
	return 0;
}
