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
	int bs, parnum=0;
	desc_map_t dmap;
	part_entry_t par;
	int ret = 0;

	if( str ) {
		parnum = atol(str);
		if( *str == 0 || *str == ',' )
			parnum = -1;
	}

	DPRINTF("macparts_open %d\n", parnum);
	SEEK( 0 );
	if( READ(&dmap, sizeof(dmap)) != sizeof(dmap) )
		goto out;

	/* partition maps might support multiple block sizes; in this case,
	 * pmPyPartStart is typically given in terms of 512 byte blocks.
	 */
	bs = dmap.sbBlockSize;
	if( bs != 512 ) {
		SEEK( 512 );
		READ( &par, sizeof(par) );
		if( par.pmSig == DESC_PART_SIGNATURE )
			bs = 512;
	}
	SEEK( bs );
	if( READ(&par, sizeof(par)) != sizeof(par) )
		goto out;
        if (par.pmSig != DESC_PART_SIGNATURE)
		goto out;

        if (parnum == -1) {
		/* search a bootable partition */
		/* see PowerPC Microprocessor CHRP bindings */

		parnum = 1;
		while (parnum <= par.pmMapBlkCnt) {
			SEEK( (bs * parnum) );
			READ( &par, sizeof(par) );
			if( par.pmSig != DESC_PART_SIGNATURE ||
                            !par.pmPartBlkCnt )
				goto out;

			if( (par.pmPartStatus & kPartitionAUXIsBootValid) &&
			    (par.pmPartStatus & kPartitionAUXIsValid) &&
			    (par.pmPartStatus & kPartitionAUXIsAllocated) &&
			    (par.pmPartStatus & kPartitionAUXIsReadable) &&
			    (strcmp(par.pmProcessor, "PowerPC") == 0) ) {
				di->blocksize =(uint)bs;
				di->offs = (llong)par.pmPyPartStart * bs;
				di->size = (llong)par.pmPartBlkCnt * bs;
				ret = -1;
				goto out;
			}

			parnum++;
		}
		/* not found */
		ret = 0;
		goto out;
        }

	if (parnum == 0) {
		di->blocksize =(uint)bs;
		di->offs = (llong)0;
		di->size = (llong)dmap.sbBlkCount * bs;
		ret = -1;
		goto out;
	}

	if( parnum > par.pmMapBlkCnt)
		goto out;

	SEEK( (bs * parnum) );
	READ( &par, sizeof(par) );
	if( par.pmSig != DESC_PART_SIGNATURE || !par.pmPartBlkCnt )
		goto out;
	if( !(par.pmPartStatus & kPartitionAUXIsValid) ||
	    !(par.pmPartStatus & kPartitionAUXIsAllocated) ||
	    !(par.pmPartStatus & kPartitionAUXIsReadable) )
		goto out;

	ret = -1;
	di->blocksize =(uint)bs;
	di->offs = (llong)par.pmPyPartStart * bs;
	di->size = (llong)par.pmPartBlkCnt * bs;

out:
	if (str)
		free(str);
	PUSH( ret);
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
