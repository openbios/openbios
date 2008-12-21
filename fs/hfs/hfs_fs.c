/*
 *   Creation Date: <2001/05/06 22:47:23 samuel>
 *   Time-stamp: <2004/01/12 10:24:35 samuel>
 *
 *	<hfs_fs.c>
 *
 *	HFS world interface
 *
 *   Copyright (C) 2001-2004 Samuel Rydh (samuel@ibrium.se)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation
 *
 */

#include "openbios/config.h"
#include "openbios/fs.h"
#include "libhfs.h"

#define MAC_OS_ROM_CREATOR	0x63687270	/* 'chrp' */
#define MAC_OS_ROM_TYPE		0x74627869	/* 'tbxi' */
#define MAC_OS_ROM_NAME		"Mac OS ROM"

#define FINDER_TYPE		0x464E4452	/* 'FNDR' */
#define FINDER_CREATOR		0x4D414353	/* 'MACS' */
#define SYSTEM_TYPE		0x7A737973	/* 'zsys' */
#define SYSTEM_CREATOR		0x4D414353	/* 'MACS' */



/************************************************************************/
/*	Search Functions						*/
/************************************************************************/

static int
_find_file( hfsvol *vol, const char *path, ulong type, ulong creator )
{
	hfsdirent ent;
	hfsdir *dir;
	int ret=1;

	if( !(dir=hfs_opendir(vol, path)) )
		return 1;

	while( ret && !hfs_readdir(dir, &ent) ) {
		if( ent.flags & HFS_ISDIR )
			continue;
		ret = !(*(ulong*)ent.u.file.type == type && *(ulong*)ent.u.file.creator == creator );
	}

	hfs_closedir( dir );
	return ret;
}


/* ret: 0=success, 1=not_found, 2=not_a_dir */
static int
_search( hfsvol *vol, const char *path, const char *sname, file_desc_t **ret_fd )
{
	hfsdir *dir;
	hfsdirent ent;
	int topdir=0, status = 1;
	char *p, buf[256];

	strncpy( buf, path, sizeof(buf) );
	if( buf[strlen(buf)-1] != ':' )
		strncat( buf, ":", sizeof(buf) );
	buf[sizeof(buf)-1] = 0;
	p = buf + strlen( buf );

	if( !(dir=hfs_opendir(vol, path)) )
		return 2;

	/* printk("DIRECTORY: %s\n", path ); */

	while( status && !hfs_readdir(dir, &ent) ) {
		ulong type, creator;

		*p = 0;
		topdir = 0;

		strncat( buf, ent.name, sizeof(buf) );
		if( (status=_search(vol, buf, sname, ret_fd)) != 2 )
			continue;
		topdir = 1;

		/* name search? */
		if( sname ) {
			status = strcasecmp( ent.name, sname );
			continue;
		}

		type = *(ulong*)ent.u.file.type;
		creator = *(ulong*)ent.u.file.creator;

		/* look for Mac OS ROM, System and Finder in the same directory */
		if( type == MAC_OS_ROM_TYPE && creator == MAC_OS_ROM_CREATOR ) {
			if( strcasecmp(ent.name, MAC_OS_ROM_NAME) )
				continue;

			status = _find_file( vol, path, FINDER_TYPE, FINDER_CREATOR )
				|| _find_file( vol, path, SYSTEM_TYPE, SYSTEM_CREATOR );
		}
	}
	if( !status && topdir && ret_fd && !(*ret_fd=(file_desc_t*)hfs_open(vol, buf)) ) {
		printk("Unexpected error: failed to open matched ROM\n");
		status = 1;
	}

	hfs_closedir( dir );
	return status;
}

static file_desc_t *
_do_search( fs_ops_t *fs, const char *sname )
{
	hfsvol *vol = hfs_getvol( NULL );
	file_desc_t *ret_fd = NULL;

	(void)_search( vol, ":", sname, &ret_fd );
	return ret_fd;
}

static file_desc_t *
search_rom( fs_ops_t *fs )
{
	return _do_search( fs, NULL );
}

static file_desc_t *
search_file( fs_ops_t *fs, const char *sname )
{
	return _do_search( fs, sname );
}


/************************************************************************/
/*	file/fs ops							*/
/************************************************************************/

static void
file_close( file_desc_t *fd )
{
	hfsfile *file = (hfsfile*)fd;
	hfs_close( file );
}

static int
file_lseek( file_desc_t *fd, off_t offs, int whence )
{
	hfsfile *file = (hfsfile*)fd;

	switch( whence ) {
	case SEEK_CUR:
		whence = HFS_SEEK_CUR;
		break;
	case SEEK_END:
		whence = HFS_SEEK_END;
		break;
	default:
	case SEEK_SET:
		whence = HFS_SEEK_SET;
		break;
	}

	return hfs_seek( file, offs, whence );
}

static int
file_read( file_desc_t *fd, void *buf, size_t count )
{
	hfsfile *file = (hfsfile*)fd;
	return hfs_read( file, buf, count );
}

static char *
get_path( file_desc_t *fd, char *retbuf, int len )
{
	char buf[256], buf2[256];
	hfsvol *vol = hfs_getvol( NULL );
	hfsfile *file = (hfsfile*)fd;
	hfsdirent ent;
	int start, ns;
	ulong id;

	hfs_fstat( file, &ent );
	start = sizeof(buf) - strlen(ent.name) - 1;
	if( start <= 0 )
		return NULL;
	strcpy( buf+start, ent.name );
	buf[--start] = '\\';

	ns = start;
	for( id=ent.parid ; !hfs_dirinfo(vol, &id, buf2) ; ) {
		start = ns;
		ns -= strlen(buf2);
		if( ns <= 0 )
			return NULL;
		strcpy( buf+ns, buf2 );
		buf[--ns] = buf[start] = '\\';
	}
	if( strlen(buf + start) >= len )
		return NULL;

	strcpy( retbuf, buf+start );
	return retbuf;
}

static char *
vol_name( fs_ops_t *fs, char *buf, int size )
{
	return get_hfs_vol_name( fs->fd, buf, size );
}


static file_desc_t *
open_path( fs_ops_t *fs, const char *path )
{
	hfsvol *vol = (hfsvol*)fs->fs_data;
	const char *s;
	char buf[256];

	if( !strncmp(path, "\\\\", 2) ) {
		hfsvolent ent;

		/* \\ is an alias for the (blessed) system folder */
		if( hfs_vstat(vol, &ent) < 0 || hfs_setcwd(vol, ent.blessed) )
			return NULL;
		path += 2;
	} else {
		hfs_chdir( vol, ":" );
	}

	for( path-- ;; ) {
		int n;

		s = ++path;
		if( !(path=strchr(s, '\\')) )
			break;
		n = MIN( sizeof(buf)-1, (path-s) );
		if( !n )
			continue;

		strncpy( buf, s, n );
		buf[n] = 0;
		if( hfs_chdir(vol, buf) )
			return NULL;
	}

	/* support the ':filetype' syntax */
	if( *s == ':' ) {
		unsigned long id, oldid = hfs_getcwd(vol);
		file_desc_t *ret = NULL;
		hfsdirent ent;
		hfsdir *dir;

		s++;
		id = oldid;
		hfs_dirinfo( vol, &id, buf );
		hfs_setcwd( vol, id );

		if( !(dir=hfs_opendir(vol, buf)) )
			return NULL;
		hfs_setcwd( vol, oldid );

		while( !hfs_readdir(dir, &ent) ) {
			if( ent.flags & HFS_ISDIR )
				continue;
			if( !strncmp(s, ent.u.file.type, 4) ) {
				ret = (file_desc_t*)hfs_open( vol, ent.name );
				break;
			}
		}
		hfs_closedir( dir );
		return ret;
	}

	return (file_desc_t*)hfs_open( vol, s );
}

static void
close_fs( fs_ops_t *fs )
{
	hfsvol *vol = (hfsvol*)fs->fs_data;

	hfs_umount( vol );
	/* callers responsibility to call free(fs) */
}

static const char *
get_fstype( fs_ops_t *fs )
{
	return ("HFS");
}

static const fs_ops_t hfs_ops = {
	.close_fs	= close_fs,
	.open_path	= open_path,
	.search_rom	= search_rom,
	.search_file	= search_file,
	.vol_name	= vol_name,

	.get_path	= get_path,
	.close		= file_close,
	.read		= file_read,
	.lseek		= file_lseek,

	.get_fstype	= get_fstype,
};

int
fs_hfs_open( int os_fd, fs_ops_t *fs )
{
	hfsvol *vol = hfs_mount( os_fd, 0 );

	if( !vol )
		return -1;

	*fs = hfs_ops;
	fs->fs_data = vol;

	return 0;
}
