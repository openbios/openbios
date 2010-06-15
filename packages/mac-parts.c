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

#include "config.h"
#include "libopenbios/bindings.h"
#include "mac-parts.h"
#include "libc/byteorder.h"
#include "libc/vsprintf.h"
#include "packages.h"

//#define CONFIG_DEBUG_MAC_PARTS

#ifdef CONFIG_DEBUG_MAC_PARTS
#define DPRINTF(fmt, args...) \
do { printk("MAC-PARTS: " fmt , ##args); } while (0)
#else
#define DPRINTF(fmt, args...) do {} while(0)
#endif

typedef struct {
	xt_t		seek_xt, read_xt;
	ucell	        offs_hi, offs_lo;
        ucell	        size_hi, size_lo;
	uint		blocksize;
} macparts_info_t;

DECLARE_NODE( macparts, INSTALL_OPEN, sizeof(macparts_info_t), "+/packages/mac-parts" );

#define SEEK( pos )		({ DPUSH(pos); call_parent(di->seek_xt); POP(); })
#define READ( buf, size )	({ PUSH((ucell)buf); PUSH(size); call_parent(di->read_xt); POP(); })

/* ( open -- flag ) */
static void
macparts_open( macparts_info_t *di )
{
	char *str = my_args_copy();
	char *argstr = strdup("");
	char *parstr = strdup("");
	int bs, parnum=-1;
	desc_map_t dmap;
	part_entry_t par;
	int ret = 0;
	int want_bootcode = 0;
	phandle_t ph;
	ducell offs, size;

	DPRINTF("macparts_open '%s'\n", str );

	/* 
		Arguments that we accept:
		id: [0-7]
		[(id)][,][filespec]
	*/

	if( str ) {
		if ( !strlen(str) )
			parnum = -1;
		else {
			/* Detect the boot parameters */
			char *ptr;
			ptr = str;

			/* <id>,<file> */
			if (*ptr >= '0' && *ptr <= '9' && *(ptr + 1) == ',') {
				parstr = ptr;
				*(ptr + 1) = '\0';
				argstr = ptr + 2;
			}

			/* <id> */
			else if (*ptr >= '0' && *ptr <='9' && *(ptr + 1) == '\0') {
				parstr = ptr;
			}

			/* ,<file> */
			else if (*ptr == ',') {
				argstr = ptr + 1;
			}	

			/* <file> */
			else {
				argstr = str;
			}
		
			/* Convert the id to a partition number */
			if (strlen(parstr))
				parnum = atol(parstr);

			/* Detect if we are looking for the bootcode */
			if (strcmp(argstr, "%BOOT") == 0)
				want_bootcode = 1;
		}
	}

	DPRINTF("parstr: %s  argstr: %s  parnum: %d\n", parstr, argstr, parnum);

	DPRINTF("want_bootcode %d\n", want_bootcode);
	DPRINTF("macparts_open %d\n", parnum);

	di->read_xt = find_parent_method("read");
	di->seek_xt = find_parent_method("seek");

	SEEK( 0 );
	if( READ(&dmap, sizeof(dmap)) != sizeof(dmap) )
		goto out;

	/* partition maps might support multiple block sizes; in this case,
	 * pmPyPartStart is typically given in terms of 512 byte blocks.
	 */
	bs = __be16_to_cpu(dmap.sbBlockSize);
	if( bs != 512 ) {
		SEEK( 512 );
		READ( &par, sizeof(par) );
		if( __be16_to_cpu(par.pmSig) == DESC_PART_SIGNATURE )
			bs = 512;
	}
	SEEK( bs );
	if( READ(&par, sizeof(par)) != sizeof(par) )
		goto out;
        if (__be16_to_cpu(par.pmSig) != DESC_PART_SIGNATURE)
		goto out;

        if (parnum == -1) {
		int firstHFS = -1;
		/* search a bootable partition */
		/* see PowerPC Microprocessor CHRP bindings */

		for (parnum = 0; parnum <= __be32_to_cpu(par.pmMapBlkCnt); parnum++) {
			SEEK( (bs * (parnum + 1)) );
			READ( &par, sizeof(par) );
			if( __be16_to_cpu(par.pmSig) != DESC_PART_SIGNATURE ||
                            !__be16_to_cpu(par.pmPartBlkCnt) )
				break;

			DPRINTF("found partition type: %s\n", par.pmPartType);

			if (firstHFS == -1 &&
			    strcmp(par.pmPartType, "Apple_HFS") == 0)
				firstHFS = parnum;

			if( (__be32_to_cpu(par.pmPartStatus) & kPartitionAUXIsBootValid) &&
			    (__be32_to_cpu(par.pmPartStatus) & kPartitionAUXIsValid) &&
			    (__be32_to_cpu(par.pmPartStatus) & kPartitionAUXIsAllocated) &&
			    (__be32_to_cpu(par.pmPartStatus) & kPartitionAUXIsReadable) &&
			    (strcmp(par.pmProcessor, "PowerPC") == 0) ) {
				di->blocksize =(uint)bs;

				offs = (llong)(__be32_to_cpu(par.pmPyPartStart)) * bs;
				di->offs_hi = offs >> BITS;
				di->offs_lo = offs & (ucell) -1;

				size = (llong)(__be32_to_cpu(par.pmPartBlkCnt)) * bs;
				di->size_hi = size >> BITS;
				di->size_lo = size & (ucell) -1;

				if (want_bootcode) {
					offs = (llong)(__be32_to_cpu(par.pmLgBootStart)) * bs;
					di->offs_hi = offs >> BITS;
					di->offs_lo = offs & (ucell) -1;

					size = (llong)(__be32_to_cpu(par.pmBootSize)) * bs;
					di->size_hi = size >> BITS;
					di->size_lo = size & (ucell) -1;
				}
				ret = -1;
				goto out;
			}

		}
		/* not found */
		if (firstHFS != -1) {
			parnum = firstHFS;
			goto found;
		}
		ret = 0;
		goto out;
        }

	if (parnum == 0) {
		di->blocksize =(uint)bs;

		offs = (llong)0;
		di->offs_hi = offs >> BITS;
		di->offs_lo = offs & (ucell) -1;

		size = (llong)__be32_to_cpu(dmap.sbBlkCount) * bs;
		di->size_hi = size >> BITS;
		di->size_lo = size & (ucell) -1;

		ret = -1;
		goto out;
	}

	if( parnum > __be32_to_cpu(par.pmMapBlkCnt))
		goto out;

found:
	SEEK( (bs * (parnum + 1)) );
	READ( &par, sizeof(par) );
	if( __be16_to_cpu(par.pmSig) != DESC_PART_SIGNATURE || !__be32_to_cpu(par.pmPartBlkCnt) )
		goto out;
	if( !(__be32_to_cpu(par.pmPartStatus) & kPartitionAUXIsValid) ||
	    !(__be32_to_cpu(par.pmPartStatus) & kPartitionAUXIsAllocated) ||
	    !(__be32_to_cpu(par.pmPartStatus) & kPartitionAUXIsReadable) )
		goto out;

	ret = -1;
	di->blocksize = (uint)bs;

	offs = (llong)__be32_to_cpu(par.pmPyPartStart) * bs;
	size = (llong)__be32_to_cpu(par.pmPartBlkCnt) * bs;

	if (want_bootcode) {
		offs += (llong)__be32_to_cpu(par.pmLgBootStart) * bs;
		size = (llong)__be32_to_cpu(par.pmBootSize);
	}

	di->offs_hi = offs >> BITS;
	di->offs_lo = offs & (ucell) -1;

	di->size_hi = size >> BITS;
	di->size_lo = size & (ucell) -1;

	/* We have a valid partition - so probe for a filesystem at the current offset */
	DPRINTF("mac-parts: about to probe for fs\n");
	DPUSH( offs );
	PUSH_ih( my_parent() );
	parword("find-filesystem");
	DPRINTF("mac-parts: done fs probe\n");

	ph = POP_ph();
	if( ph ) {
		DPRINTF("mac-parts: filesystem found with ph " FMT_ucellx " and args %s\n", ph, str);
		push_str( argstr );
		PUSH_ph( ph );
		fword("interpose");
	} else {
		DPRINTF("mac-parts: no filesystem found; bypassing misc-files interpose\n");
	}

	free( str );

out:
	PUSH( ret );
}

/* ( block0 -- flag? ) */
static void
macparts_probe( macparts_info_t *dummy )
{
	desc_map_t *dmap = (desc_map_t*)POP();

	DPRINTF("macparts_probe %x ?= %x\n", dmap->sbSig, DESC_MAP_SIGNATURE);
	if( __be16_to_cpu(dmap->sbSig) != DESC_MAP_SIGNATURE )
		RET(0);
	RET(-1);
}

/* ( -- type offset.d size.d ) */
static void
macparts_get_info( macparts_info_t *di )
{
	DPRINTF("macparts_get_info");

	PUSH( -1 );		/* no type */
	PUSH( di->offs_lo );
	PUSH( di->offs_hi );
	PUSH( di->size_lo );
	PUSH( di->size_hi );
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

/* ( pos.d -- status ) */
static void
macparts_seek(macparts_info_t *di )
{
	llong pos = DPOP();
	llong offs;

	DPRINTF("macparts_seek %llx:\n", pos);

	/* Calculate the seek offset for the parent */
	offs = ((ducell)di->offs_hi << BITS) | di->offs_lo;
	offs += pos;
	DPUSH(offs);

	DPRINTF("macparts_seek parent offset %llx:\n", offs);

	call_package(di->seek_xt, my_parent());
}

/* ( buf len -- actlen ) */
static void
macparts_read(macparts_info_t *di )
{
	DPRINTF("macparts_read\n");

	/* Pass the read back up to the parent */
	call_package(di->read_xt, my_parent());
}

/* ( addr -- size ) */
static void
macparts_load( __attribute__((unused))macparts_info_t *di )
{
	forth_printf("load currently not implemented for /packages/mac-parts\n");
	PUSH(0);
}

NODE_METHODS( macparts ) = {
	{ "probe",	macparts_probe 		},
	{ "open",	macparts_open 		},
	{ "seek",	macparts_seek 		},
	{ "read",	macparts_read 		},
	{ "load",	macparts_load 		},
	{ "get-info",	macparts_get_info 	},
	{ "block-size",	macparts_block_size 	},
	{ NULL,		macparts_initialize	},
};

void
macparts_init( void )
{
	REGISTER_NODE( macparts );
}
