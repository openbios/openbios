/*
 *   pc partition support
 *
 *   Copyright (C) 2004 Stefan Reinauer
 *
 *   This code is based (and copied in many places) from
 *   mac partition support by Samuel Rydh (samuel@ibrium.se)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   version 2
 *
 */

#include "config.h"
#include "libopenbios/bindings.h"
#include "libc/byteorder.h"
#include "packages.h"

typedef struct {
	ullong		offs;
	ullong		size;
} pcparts_info_t;

DECLARE_NODE( pcparts, INSTALL_OPEN, sizeof(pcparts_info_t), "+/packages/pc-parts" );


#define SEEK( pos )		({ DPUSH(pos); call_parent(seek_xt); POP(); })
#define READ( buf, size )	({ PUSH((ucell)buf); PUSH(size); call_parent(read_xt); POP(); })

/* two helper functions */

static inline int has_pc_part_magic(unsigned char *sect)
{
	return sect[510]==0x55 && sect[511]==0xAA;
}

static inline int is_pc_extended_part(unsigned char type)
{
	return type==5 || type==0xf || type==0x85;
}

/* ( open -- flag ) */
static void
pcparts_open( pcparts_info_t *di )
{
	char *str = my_args_copy();
	xt_t seek_xt = find_parent_method("seek");
	xt_t read_xt = find_parent_method("read");
	int bs, parnum=-1;
	/* Layout of PC partition table */
	struct pc_partition {
		unsigned char boot;
		unsigned char head;
		unsigned char sector;
		unsigned char cyl;
		unsigned char type;
		unsigned char e_head;
		unsigned char e_sector;
		unsigned char e_cyl;
		u32 start_sect; /* unaligned little endian */
		u32 nr_sects; /* ditto */
	} *p;
	unsigned char buf[512];

	/* printk("pcparts_open '%s'\n", str ); */

	if( str ) {
		parnum = atol(str);
		if( !strlen(str) )
			parnum = 1;
		free( str );
	}
	if( parnum < 0 )
		parnum = 1;

	SEEK( 0 );
	if( READ(buf, 512) != 512 )
		RET(0);

	/* Check Magic */
	if (!has_pc_part_magic(buf)) {
		printk("pc partition magic not found.\n");
		RET(0);
	}

	/* get partition data */
	p = (struct pc_partition *) (buf + 0x1be);

	bs=512;

	if (parnum < 4) {
		/* primary partition */
		p += parnum;
		if (p->type==0 || is_pc_extended_part(p->type)) {
			printk("partition %d does not exist\n", parnum+1 );
			RET( 0 );
		}
		di->offs = (llong)(__le32_to_cpu(p->start_sect)) * bs;
		di->size = (llong)(__le32_to_cpu(p->nr_sects)) * bs;

		/* printk("Primary partition at sector %x\n",
				__le32_to_cpu(p->start_sect)); */

		RET( -1 );
	} else {
		/* Extended partition */
		int i, cur_part;
		unsigned long ext_start, cur_table;

		/* Search for the extended partition
		 * which contains logical partitions */
		for (i = 0; i < 4; i++) {
			if (is_pc_extended_part(p[i].type))
				break;
		}

		if (i >= 4) {
			printk("Extended partition not found\n");
			RET( 0 );
		}

		printk("Extended partition at %d\n", i+1);

		/* Visit each logical partition labels */
		ext_start = __le32_to_cpu(p[i].start_sect);
		cur_table = ext_start;
		cur_part = 4;

		for (;;) {
			/* printk("cur_part=%d at %x\n", cur_part, cur_table); */

			SEEK( cur_table*bs );
			if( READ(buf, sizeof(512)) != sizeof(512) )
				RET( 0 );

			if (!has_pc_part_magic(buf)) {
				printk("Extended partition has no magic\n");
				break;
			}

			p = (struct pc_partition *) (buf + 0x1be);
			/* First entry is the logical partition */
			if (cur_part == parnum) {
				if (p->type==0) {
					printk("Partition %d is empty\n", parnum+1);
					RET( 0 );
				}
				di->offs =
					(llong)(cur_table+__le32_to_cpu(p->start_sect)) * bs;
				di->size = (llong)__le32_to_cpu(p->nr_sects) * bs;
				RET ( -1 );
			}

			/* Second entry is link to next partition */
			if (!is_pc_extended_part(p[1].type)) {
				printk("no link\n");
				break;
			}
			cur_table = ext_start + __le32_to_cpu(p[1].start_sect);

			cur_part++;
		}
		printk("Logical partition %d does not exist\n", parnum+1);
		RET( 0 );
	}
	/* we should never reach this point */
}

/* ( block0 -- flag? ) */
static void
pcparts_probe( pcparts_info_t *dummy )
{
	unsigned char *buf = (unsigned char *)POP();

	RET ( has_pc_part_magic(buf) );
}

/* ( -- type offset.d size.d ) */
static void
pcparts_get_info( pcparts_info_t *di )
{
	PUSH( -1 );		/* no type */
	DPUSH( di->offs );
	DPUSH( di->size );
}

static void
pcparts_block_size( __attribute__((unused))pcparts_info_t *di )
{
	PUSH(512);
}

static void
pcparts_initialize( pcparts_info_t *di )
{
	fword("register-partition-package");
}

NODE_METHODS( pcparts ) = {
	{ "probe",	pcparts_probe 		},
	{ "open",	pcparts_open 		},
	{ "get-info",	pcparts_get_info 	},
	{ "block-size",	pcparts_block_size 	},
	{ NULL,		pcparts_initialize	},
};

void
pcparts_init( void )
{
	REGISTER_NODE( pcparts );
}
