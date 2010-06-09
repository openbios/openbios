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

#include "config.h"
#include "libopenbios/bindings.h"
#include "fs/fs.h"
#include "libc/diskio.h"
#include "libc/vsprintf.h"
#include "packages.h"

//#define CONFIG_DEBUG_MISC_FILES

#ifdef CONFIG_DEBUG_MISC_FILES
#define DPRINTF(fmt, args...)                   \
    do { printk(fmt , ##args); } while (0)
#else
#define DPRINTF(fmt, args...)
#endif

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

	DPRINTF("misc-files doing open with ih " FMT_ucellX "\n", ih);

	err = (fd = open_ih(ih)) == -1;

	if( !err ) {
		err = fs_hfsp_open(fd, fs);
		DPRINTF("--- HFSP returned %d\n", err);

		if( err ) {
			err = fs_hfs_open(fd, fs);
			DPRINTF("--- HFS returned %d\n", err);
		}

		if( err ) {
			err = fs_iso9660_open(fd, fs);
			DPRINTF("--- ISO9660 returned %d\n", err);
		}

		if( err ) {
			err = fs_ext2_open(fd, fs);
			DPRINTF("--- ext2 returned %d\n", err);
		}

		if( err ) {
			err = fs_grubfs_open(fd, fs);
			DPRINTF("--- grubfs returned %d\n", err);
		}

		fs->fd = fd;
	}

	if( err ) {
		if( fd != -1 )
			close_io( fd );
		free( fs );
		return NULL;
	}

	DPRINTF("misc-files open returns %p\n", fs);

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

	mi->fs = fs;

	DPRINTF("misc-files open arguments: %s\n", name);

	/* If we have been passed a specific file, open it */
	if( name ) {
		if( !(mi->file = fs_open_path(fs, name)) ) {
			forth_printf("Unable to open path %s\n", name);
			free( name );
			do_close( fs );
			RET(0);
		}

		DPRINTF("Successfully opened %s\n", name);
		// printk("PATH: %s\n", fs->get_path(mi->file, mi->pathbuf, PATHBUF_SIZE) );
	} else {
		/* No file was specified, but return success so that we can read
		the filesystem. If mi->file is not set, routines simply call their
		parent which in this case is the partition handler. */

		RET(-1);
	}

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
	int len;
	char *buf;
	int ret;

	if( mi->file ) {
		len = POP();
		buf = (char*)POP();

		ret = mi->fs->read( mi->file, buf, len );
		mi->filepos += ret;

		PUSH( ret );
	} else {
		DPRINTF("misc-files read: no valid FS so calling parent\n");

		call_parent_method("read");
	}
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
	if( mi->file ) {
		llong pos = DPOP();
		cell ret;
		int offs = (int)pos;
		int whence = SEEK_SET;

		DPRINTF("misc-files seek: using FS handle\n");

		if( offs == -1 ) {
			offs = 0;
			whence = SEEK_END;
		}
		mi->filepos = mi->fs->lseek( mi->file, offs, whence );
		ret = (mi->filepos < 0)? -1 : 0;

		PUSH( ret );
	} else {
		DPRINTF("misc-files seek: no valid FS so calling parent\n");

		call_parent_method("seek");
	}
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
	char *buf;
	int ret, size;

	if (mi->file) {
		buf = (char*)POP();
		size = 0;

		DPRINTF("misc-files load at address %p\n", buf);

		while (1) {
			ret = mi->fs->read( mi->file, buf, 8192 );
			if (ret <= 0)
				break;
			buf += ret;
			mi->filepos += ret;
			size += ret;
	
			if (size % 0x100000 == 0)
				DPRINTF("----> size is %dM\n", size / 0x100000);
	
			if (ret != 8192)
				break;
		}
	
		DPRINTF("load complete with size: %d\n", size);
		PUSH( size );
	} else {

		DPRINTF("misc-files load: no valid FS so calling parent\n");

		call_parent_method("load");
	}
}

/* static method, ( pos.d ih -- flag? ) */
static void
files_probe( files_info_t *mi )
{
	ihandle_t ih = POP_ih();
	llong offs = DPOP();
	int fd, err = 0;

	DPRINTF("misc-files probe with offset %llx\n", offs);

	err = (fd = open_ih(ih)) == -1;
	if( !err ) {

		err = fs_hfsp_probe(fd, offs);
		DPRINTF("--- HFSP returned %d\n", err);

		if( err ) {
			err = fs_hfs_probe(fd, offs);
			DPRINTF("--- HFS returned %d\n", err);
		}
/*
		if( err ) {
			err = fs_iso9660_open(fd, fs);
			DPRINTF("--- ISO9660 returned %d\n", err);
		}
*/
		if( err ) {
			err = fs_ext2_probe(fd, offs);
			DPRINTF("--- ext2 returned %d\n", err);
		}

		if( err ) {
			err = fs_grubfs_probe(fd, offs);
			DPRINTF("--- grubfs returned %d\n", err);
		}
	}

	if (fd)
		close_io(fd);

	/* If no errors occurred, indicate success */
	if (!err) {
		DPRINTF("misc-files probe found filesystem\n");
		PUSH(-1);
	} else {
		DPRINTF("misc-files probe could not find filesystem\n");
		PUSH(0);
	}
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
files_init( void )
{
	REGISTER_NODE( files );
}
