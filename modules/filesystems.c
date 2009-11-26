/*
 *   Creation Date: <2003/12/08 19:19:29 samuel>
 *   Time-stamp: <2004/01/07 19:22:40 samuel>
 *
 *	<filesystems.c>
 *
 *	generic filesystem support node
 *
 *   Copyright (C) 2003, 2004 Samuel Rydh (samuel@ibrium.se)
 *   Copyright (C) 2004 Stefan Reinauer
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   version 2
 *
 */

#include "openbios/config.h"
#include "openbios/bindings.h"
#include "openbios/fs.h"
#include "libc/diskio.h"
#include "modules.h"

#define PATHBUF_SIZE	256
#define VOLNAME_SIZE	64

typedef struct {
	fs_ops_t	*fs;
	file_desc_t	*file;
	char		*pathbuf;
	char		*volname;

	cell		filepos;
} files_info_t;

DECLARE_NODE( files, 0, sizeof(files_info_t), "+/packages/misc-files" );


static fs_ops_t *
do_open( ihandle_t ih )
{
	fs_ops_t *fs = malloc( sizeof(*fs) );
	int err, fd;

	err = (fd=open_ih(ih)) == -1;

	if( !err ) {
		err=fs_hfsp_open(fd, fs);
		if( err ) err = fs_hfs_open(fd, fs);
		if( err ) err = fs_iso9660_open(fd, fs);
		if( err ) err = fs_ext2_open(fd, fs);
		if( err ) err = fs_grubfs_open(fd, fs);
	}

	fs->fd = fd;

	if( err ) {
		if( fd != -1 )
			close_io( fd );
		free( fs );
		return NULL;
	}

	return fs;
}

static void
do_close( fs_ops_t *fs )
{
	fs->close_fs( fs );
	close_io( fs->fd );
	free( fs );
}

/* ( -- success? ) */
static void
files_open( files_info_t *mi )
{
	fs_ops_t *fs = do_open( my_parent() );
	char *name;

	if( !fs )
		RET( 0 );

	name = my_args_copy();
	if( name ) {
		if( !(mi->file=fs_open_path(fs, name)) ) {
			free( name );
			do_close( fs );
			RET(0);
		}
		/* printk("PATH: %s\n", fs->get_path(mi->file, mi->pathbuf, PATHBUF_SIZE) ); */
	}
	mi->fs = fs;

	if( name )
		free( name );

	RET( -1 );
}

static void
do_reopen( files_info_t *mi, file_desc_t *file )
{
	if( !file )
		return;
	if( mi->file )
		mi->fs->close( mi->file );
	mi->file = file;
	mi->filepos = 0;
}

/* ( file-str len -- success? ) */
static void
files_reopen( files_info_t *mi )
{
	file_desc_t *file = NULL;
	char *name = pop_fstr_copy();

	if( name ) {
		file = fs_open_path( mi->fs, name );
		free( name );
		do_reopen( mi, file );
	}
	PUSH( file? -1:0 );
}

/* ( -- cstr ) */
static void
files_get_path( files_info_t *mi )
{
	char *ret;

	if( !mi->file )
		RET(0);
	if( !mi->pathbuf )
		mi->pathbuf = malloc( PATHBUF_SIZE );
	ret = mi->fs->get_path( mi->file, mi->pathbuf, PATHBUF_SIZE );
	PUSH( (ucell)ret );
}

/* ( -- cstr|0 ) */
static void
files_volume_name( files_info_t *mi )
{
	char *ret;

	if( !mi->volname )
		mi->volname = malloc( VOLNAME_SIZE );
	/* handle case where there is no vol_name function in fs */
	if( !mi->fs->vol_name ) {
		mi->volname[0] = '\0';
		ret = mi->volname;
	} else
		ret = mi->fs->vol_name( mi->fs, mi->volname, VOLNAME_SIZE );
	PUSH( (ucell)ret );
}

/* ( -- success? ) */
static void
files_open_nwrom( files_info_t *mi )
{
	file_desc_t *file = fs_search_rom( mi->fs );
	do_reopen( mi, file );
	PUSH( file? -1:0 );
}

/* ( -- ) */
static void
files_close( files_info_t *mi )
{
	if( mi->file )
		mi->fs->close( mi->file );
	do_close( mi->fs );

	if( mi->pathbuf )
		free( mi->pathbuf );
	if( mi->volname )
		free( mi->volname );
}

/* ( buf len -- actlen ) */
static void
files_read( files_info_t *mi )
{
	int len = POP();
	char *buf = (char*)POP();
	int ret;

	if( mi->file ) {
		ret = mi->fs->read( mi->file, buf, len );
		mi->filepos += ret;
	} else {
		ret = read_io( mi->fs->fd, buf, len );
	}
	PUSH( ret );
}

/* ( buf len -- actlen ) */
static void
files_write( files_info_t *mi )
{
	DDROP();
	PUSH( 0 );
}

/* ( pos.d -- status ) */
static void
files_seek( files_info_t *mi )
{
	llong pos = DPOP();
	cell ret;

	if( mi->file ) {
		int offs = (int)pos;
		int whence = SEEK_SET;

		if( offs == -1 ) {
			offs = 0;
			whence = SEEK_END;
		}
		mi->filepos = mi->fs->lseek( mi->file, offs, whence );
		ret = (mi->filepos < 0)? -1 : 0;
	} else {
		ret = seek_io( mi->fs->fd, pos );
	}
	PUSH( ret );
}

/* ( -- filepos.d ) */
static void
files_tell( files_info_t *mi )
{
	llong ret = mi->file ? mi->filepos : -1;
	DPUSH( ret );
}

/* ( -- cstr ) */
static void
files_get_fstype( files_info_t *mi )
{
	if (mi->fs->get_fstype)
		PUSH( (ucell)(mi->fs->get_fstype(mi->fs)) );
	else
		PUSH( (ucell)"unspecified");
}

/* ( addr -- size ) */

static void
files_load( files_info_t *mi)
{
	char *buf = (char*)POP();
	int ret, size;

	if (!mi->file) {
		PUSH(0);
		return;
	}

	size = 0;
	while(1) {
		ret = mi->fs->read( mi->file, buf, 512 );
		if (ret <= 0)
			break;
		buf += ret;
		mi->filepos += ret;
		size += ret;
		if (ret != 512)
			break;
	}
	PUSH( size );
}

/* static method, ( ih -- flag? ) */
static void
files_probe( files_info_t *dummy )
{
	ihandle_t ih = POP_ih();
	fs_ops_t *fs;
	int ret = 0;

	if( (fs=do_open(ih)) != NULL ) {
		/* printk("HFS[+] filesystem found\n"); */
		do_close( fs );
		ret = -1;
	}
	PUSH( ret );
}

static void
files_block_size( files_info_t *dummy )
{
	PUSH(512);
}

static void
files_dir( files_info_t *mi )
{
	if (mi->fs->dir)
		mi->fs->dir(mi->file);
}

static void
files_initializer( files_info_t *dummy )
{
	fword("register-fs-package");
}

NODE_METHODS( files ) = {
	{ "probe",		files_probe 		},
	{ "open",		files_open 		},
	{ "close",		files_close 		},
	{ "read",		files_read 		},
	{ "write",		files_write 		},
	{ "seek",		files_seek 		},
	{ "tell",		files_tell		},
	{ "load",		files_load		},
	{ "dir",		files_dir		},
	{ "block-size",		files_block_size	},

	/* special */
	{ "reopen",		files_reopen 		},
	{ "open-nwrom",		files_open_nwrom 	},
	{ "get-path",		files_get_path		},
	{ "get-fstype",		files_get_fstype	},
	{ "volume-name",	files_volume_name	},

	{ NULL,			files_initializer 	},
};


void
filesystem_init( void )
{
	REGISTER_NODE( files );
}
