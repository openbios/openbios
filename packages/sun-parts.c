/*
 *   Sun (Sparc32/64) partition support
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

#include "openbios/config.h"
#include "openbios/bindings.h"
#include "libc/byteorder.h"
#include "packages.h"

#ifdef CONFIG_DEBUG_SUN_PARTS
#define DPRINTF(fmt, args...)                   \
    do { printk(fmt , ##args); } while (0)
#else
#define DPRINTF(fmt, args...)
#endif

typedef struct {
        ucell	        offs_hi, offs_lo;
        ucell	        size_hi, size_lo;
	int		type;
} sunparts_info_t;

DECLARE_NODE( sunparts, INSTALL_OPEN, sizeof(sunparts_info_t), "+/packages/sun-parts" );


#define SEEK( pos )		({ DPUSH(pos); call_parent(seek_xt); POP(); })
#define READ( buf, size )	({ PUSH((ucell)buf); PUSH(size); call_parent(read_xt); POP(); })

/* Layout of SUN partition table */
struct sun_disklabel {
    uint8_t info[128];   /* Informative text string */
    uint8_t spare0[14];
    struct sun_info {
        uint8_t spare1;
        uint8_t id;
        uint8_t spare2;
        uint8_t flags;
    } infos[8];
    uint8_t spare[246];  /* Boot information etc. */
    uint16_t rspeed;     /* Disk rotational speed */
    uint16_t pcylcount;  /* Physical cylinder count */
    uint16_t sparecyl;   /* extra sects per cylinder */
    uint8_t spare2[4];   /* More magic... */
    uint16_t ilfact;     /* Interleave factor */
    uint16_t ncyl;       /* Data cylinder count */
    uint16_t nacyl;      /* Alt. cylinder count */
    uint16_t ntrks;      /* Tracks per cylinder */
    uint16_t nsect;      /* Sectors per track */
    uint8_t spare3[4];   /* Even more magic... */
    struct sun_partition {
        uint32_t start_cylinder;
        uint32_t num_sectors;
    } partitions[8];
    uint16_t magic;      /* Magic number */
    uint16_t csum;       /* Label xor'd checksum */
};

/* two helper functions */

static inline int
has_sun_part_magic(unsigned char *sect)
{
    struct sun_disklabel *p = (struct sun_disklabel *)sect;
    uint16_t csum, *ush, tmp16;

    if (__be16_to_cpu(p->magic) != 0xDABE)
        return 0;

    csum = 0;
    for (ush = (uint16_t *)p; ush < (uint16_t *)(p + 1); ush++) {
        tmp16 = __be16_to_cpu(*ush);
	csum ^= tmp16;
    }
    return csum == 0;
}

/* ( open -- flag ) */
static void
sunparts_open( sunparts_info_t *di )
{
	char *str = my_args_copy();
	xt_t seek_xt = find_parent_method("seek");
	xt_t read_xt = find_parent_method("read");
	int parnum = -1;
	unsigned char buf[512];
        struct sun_disklabel *p;
        unsigned int i, bs;
        ducell offs, size;

	DPRINTF("sunparts_open '%s'\n", str );

	if( str ) {
            if( !strlen(str) )
                parnum = -1;
            else if (str[0] >= 'a' && str[0] < ('a' + 8))
                parnum = str[0] - 'a';
            else
                parnum = atol(str);

            free( str );
	}

	SEEK( 0 );
	if( READ(buf, 512) != 512 )
		RET(0);

	/* Check Magic */
	if (!has_sun_part_magic(buf)) {
		DPRINTF("Sun partition magic not found.\n");
		RET(0);
	}

	bs = 512;
	/* get partition data */
	p = (struct sun_disklabel *)buf;

        for (i = 0; i < 8; i++) {
	    DPRINTF("%c: %d + %d, id %x, flags %x\n", 'a' + i, p->partitions[i].start_cylinder,
		   p->partitions[i].num_sectors, p->infos[i].id, p->infos[i].flags);
            if (parnum < 0) {
                if (p->partitions[i].num_sectors != 0 && p->infos[i].id != 0)
                    parnum = i;
            }
        }

        if (parnum < 0)
            parnum = 0;

	DPRINTF("Selected partition %d\n", parnum);
        offs = (llong)__be32_to_cpu(p->partitions[parnum].start_cylinder) *
            __be16_to_cpu(p->ntrks) * __be16_to_cpu(p->nsect) * bs;

        di->offs_hi = offs >> BITS;
        di->offs_lo = offs & (ucell) -1;
        size = (llong)__be32_to_cpu(p->partitions[parnum].num_sectors) * bs;
        di->size_hi = size >> BITS;
        di->size_lo = size & (ucell) -1;
        di->type = __be32_to_cpu(p->infos[parnum].id);
        DPRINTF("Found Sun partition table, offs %lld size %lld\n",
                (llong)offs, (llong)size);

        RET( -1 );
}

/* ( block0 -- flag? ) */
static void
sunparts_probe( __attribute__((unused))sunparts_info_t *dummy )
{
	unsigned char *buf = (unsigned char *)POP();

	RET ( has_sun_part_magic(buf) );
}

/* ( -- type offset.d size.d ) */
static void
sunparts_get_info( sunparts_info_t *di )
{
	DPRINTF("Sun get_info\n");
	PUSH( di->type );
	PUSH( di->offs_lo );
	PUSH( di->offs_hi );
	PUSH( di->size_lo );
	PUSH( di->size_hi );
}

static void
sunparts_block_size( __attribute__((unused))sunparts_info_t *di )
{
	PUSH(512);
}

static void
sunparts_initialize( __attribute__((unused))sunparts_info_t *di )
{
	fword("register-partition-package");
}

NODE_METHODS( sunparts ) = {
	{ "probe",	sunparts_probe 		},
	{ "open",	sunparts_open 		},
	{ "get-info",	sunparts_get_info 	},
	{ "block-size",	sunparts_block_size 	},
	{ NULL,		sunparts_initialize	},
};

void
sunparts_init( void )
{
	REGISTER_NODE( sunparts );
}
