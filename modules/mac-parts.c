/*
 *   Creation Date: <2003/12/04 17:07:05 samuel>
 *   Time-stamp: <2004/01/07 19:36:09 samuel>
 *
 *	<mac-parts.c>
 *
 *	macintosh partition support
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
#include "mac-parts.h"
#include "modules.h"

#ifdef CONFIG_DEBUG_MAC_PARTS
#define DPRINTF(fmt, args...) \
do { printk("MAC-PARTS: " fmt , ##args); } while (0)
#else
#define DPRINTF(fmt, args...) do {} while(0)
#endif

typedef struct {
	ullong		offs;
	ullong		size;
	uint		blocksize;
} macparts_info_t;

DECLARE_NODE( macparts, INSTALL_OPEN, sizeof(macparts_info_t), "+/packages/mac-parts" );


#define SEEK( pos )		({ DPUSH(pos); call_parent(seek_xt); POP(); })
#define READ( buf, size )	({ PUSH((ucell)buf); PUSH(size); call_parent(read_xt); POP(); })

/* ( open -- flag ) */
static void
macparts_open( macparts_info_t *di )
{
	char *str = my_args_copy();
	xt_t seek_xt = find_parent_method("seek");
	xt_t read_xt = find_parent_method("read");
	int bs, parnum=-1;
	desc_map_t dmap;
	part_entry_t par;

	if( str ) {
		parnum = atol(str);
		if( !strlen(str) )
			parnum = 1;
		free( str );
	}
	if( parnum < 0 )
		parnum = 0;

	DPRINTF("macparts_open %d\n", parnum);
	SEEK( 0 );
	if( READ(&dmap, sizeof(dmap)) != sizeof(dmap) )
		RET(0);

	/* partition maps might support multiple block sizes; in this case,
	 * pmPyPartStart is typically given in terms of 512 byte blocks.
	 */
	bs = dmap.sbBlockSize;
	if( bs != 512 ) {
		SEEK( 512 );
		READ( &par, sizeof(par) );
		if( par.pmSig == 0x504d /* 'PM' */ )
			bs = 512;
	}
	if (parnum == 0) {
		di->blocksize =(uint)bs;
		di->offs = (llong)0;
		di->size = (llong)dmap.sbBlkCount * bs;
		PUSH( -1 );
		return;
	}
	SEEK( bs );
	if( READ(&par, sizeof(par)) != sizeof(par) )
		RET(0);
	if( parnum > par.pmMapBlkCnt || par.pmSig != 0x504d /* 'PM' */ )
		RET(0);

	SEEK( (bs * parnum) );
	READ( &par, sizeof(par) );

	if( par.pmSig != 0x504d /* 'PM' */ || !par.pmPartBlkCnt )
		RET(0);

	di->blocksize =(uint)bs;
	di->offs = (llong)par.pmPyPartStart * bs;
	di->size = (llong)par.pmPartBlkCnt * bs;

	PUSH( -1 );
}

/* ( block0 -- flag? ) */
static void
macparts_probe( macparts_info_t *dummy )
{
	desc_map_t *dmap = (desc_map_t*)POP();

	DPRINTF("macparts_probe %x ?= %x\n", dmap->sbSig, DESC_MAP_SIGNATURE);
	if( dmap->sbSig != DESC_MAP_SIGNATURE )
		RET(0);
	RET(-1);
}

/* ( -- type offset.d size.d ) */
static void
macparts_get_info( macparts_info_t *di )
{
	PUSH( -1 );		/* no type */
	DPUSH( di->offs );
	DPUSH( di->size );
	DPRINTF("macparts_get_info %lld %lld\n", di->offs, di->size);
}

static void
macparts_block_size( macparts_info_t *di )
{
	DPRINTF("macparts_block_size = %x\n", di->blocksize);
	PUSH(di->blocksize);
}

static void
macparts_initialize( macparts_info_t *di )
{
	fword("register-partition-package");
}

NODE_METHODS( macparts ) = {
	{ "probe",	macparts_probe 		},
	{ "open",	macparts_open 		},
	{ "get-info",	macparts_get_info 	},
	{ "block-size",	macparts_block_size 	},
	{ NULL,		macparts_initialize	},
};

void
macparts_init( void )
{
	REGISTER_NODE( macparts );
}
