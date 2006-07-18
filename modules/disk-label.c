/* 
 *   Creation Date: <2003/12/03 22:10:45 samuel>
 *   Time-stamp: <2004/01/07 19:17:45 samuel>
 *   
 *	<disk-label.c>
 *	
 *	Partition support
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
#include "libc/diskio.h"
#include "modules.h"

typedef struct {
	int		fd;
	
	ullong		offs;
	ullong		size;
	int		type;		/* partition type or -1 */

	ihandle_t	part_ih;
} dlabel_info_t;

DECLARE_NODE( dlabel, 0, sizeof(dlabel_info_t), "/packages/disk-label" );


/* ( -- ) */
static void
dlabel_close( dlabel_info_t *di )
{
	if( di->part_ih )
		close_package( di->part_ih );

	if( di->fd )
		close_io( di->fd );
}

/* ( -- success? ) */
static void
dlabel_open( dlabel_info_t *di )
{
	char *s, *filename, *parstr;
	char block0[512];
	phandle_t ph;
	int fd, success=0;

	parstr = my_args_copy();
	/* printk("dlabel-open '%s'\n", parstr ); */

	if( (fd=open_ih(my_parent())) == -1 )
		goto out;
	di->fd = fd;

	/* argument format: parnum,filename */
	s = parstr;
	filename = NULL;
	if( s ) {
            if( *s == '-' || isdigit(*s) ||
                (*s >= 'a' && *s < ('a' + 8)
                 && (*(s + 1) == ',' || *(s + 1) == '\0'))) {
			if( (s=strpbrk(parstr,",")) ) {
				filename = s+1;
				*s = 0;
			}
		} else {
			filename = s;
			parstr = NULL;
			if( *s == ',' )
				filename++;
		}
	}

	/* find partition handler */
	seek_io( fd, 0 );
	if( read_io(fd, block0, sizeof(block0)) != sizeof(block0) )
		goto out;
	PUSH( (ucell)block0 );
	selfword("find-part-handler");
	ph = POP_ph();
	
	/* open partition package */
	if( ph ) {
		xt_t xt;
		if( !(di->part_ih=open_package(parstr, ph)) )
			goto out;
		if( !(xt=find_ih_method("get-info", di->part_ih)) )
			goto out;
		call_package( xt , di->part_ih );
		di->size = DPOP();
		di->offs = DPOP();
		di->type = POP();
	}

	/* probe for filesystem */
	PUSH_ih( my_self() );
	selfword("find-filesystem");
	ph = POP_ph();
	if( ph ) {
		push_str( filename );
		PUSH_ph( ph );
		fword("interpose");
	} else if( filename ) {
		goto out;
	}
	success = 1;
	
 out:
	if( parstr )
		free( parstr );
	if( !success ) {
		dlabel_close( di );
		RET(0);
	}
	PUSH(-1);
}

/* ( addr len -- actual ) */
static void
dlabel_read( dlabel_info_t *di )
{
	int ret, len = POP();
	char *buf = (char*)POP();

	ret = read_io( di->fd, buf, len );
	PUSH( ret );
}

/* ( pos.d -- status ) */
static void
dlabel_seek( dlabel_info_t *di )
{
	llong pos = DPOP();
	int ret;

	if( pos != -1 )
		pos += di->offs;
	else if( di->size ) {
		/* printk("Seek EOF\n"); */
		pos = di->offs + di->size;
	} else {
		/* let parent handle the EOF seek. */
	}

	/* printk("dlabel_seek: %x %08x\n", (int)(pos>>32), (int)pos ); */
	ret = seek_io( di->fd, pos );
	PUSH( ret );
}

/* ( -- filepos.d ) */
static void
dlabel_tell( dlabel_info_t *di )
{
	llong pos = tell( di->fd );
	if( pos != -1 )
		pos -= di->offs;

	DPUSH( pos );
}


/* ( addr len -- actual ) */
static void
dlabel_write( __attribute__((unused)) dlabel_info_t *di )
{
	DDROP();
	PUSH( -1 );
}

/* ( rel.d -- abs.d ) */
static void
dlabel_offset( dlabel_info_t *di )
{
	ullong rel = DPOP();
	rel += di->offs;
	DPUSH( rel );
}

/* ( addr -- size ) */
static void
dlabel_load( __attribute__((unused)) dlabel_info_t *di )
{
	/* XXX: try the load method of the part package */

	printk("Can't load from this device!\n");
	POP();
	PUSH(0);
}

NODE_METHODS( dlabel ) = {
	{ "open",	dlabel_open 	},
	{ "close",	dlabel_close 	},
	{ "offset",	dlabel_offset 	},
	{ "load",	dlabel_load 	},

	{ "read",	dlabel_read 	},
	{ "write",	dlabel_write 	},
	{ "seek",	dlabel_seek 	},
	{ "tell",	dlabel_tell 	},
};

void
disklabel_init( void )
{
	REGISTER_NODE( dlabel );
}
