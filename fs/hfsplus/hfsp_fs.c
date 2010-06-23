/*
 *   Creation Date: <2001/05/05 23:33:49 samuel>
 *   Time-stamp: <2004/01/12 10:25:39 samuel>
 *
 *	<hfsp_fs.c>
 *
 *	HFS+ file system interface (and ROM lookup support)
 *
 *   Copyright (C) 2001, 2002, 2003, 2004 Samuel Rydh (samuel@ibrium.se)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation
 *
 */

#include "config.h"
#include "fs/fs.h"
#include "libhfsp.h"
#include "volume.h"
#include "record.h"
#include "unicode.h"
#include "blockiter.h"


#define MAC_OS_ROM_CREATOR	0x63687270	/* 'chrp' */
#define MAC_OS_ROM_TYPE		0x74627869	/* 'tbxi' */
#define MAC_OS_ROM_NAME		"Mac OS ROM"

#define FINDER_TYPE		0x464E4452	/* 'FNDR' */
#define FINDER_CREATOR		0x4D414353	/* 'MACS' */
#define SYSTEM_TYPE		0x7A737973	/* 'zsys' */
#define SYSTEM_CREATOR		0x4D414353	/* 'MACS' */


typedef struct {
	record		rec;
	char		*path;

	off_t		pos;
} hfsp_file_t;


/************************************************************************/
/*	Search implementation						*/
/************************************************************************/

typedef int (*match_proc_t)( record *r, record *parent, const void *match_data, hfsp_file_t *pt );

static int
search_files( record *par, int recursive, match_proc_t proc, const void *match_data, hfsp_file_t *pt )
{
	hfsp_file_t t;
	record r;
	int ret = 1;

	t.path = NULL;

	record_init_parent( &r, par );
	do{
		if( r.record.type == HFSP_FOLDER || r.record.type == HFSP_FILE )
			ret = (*proc)( &r, par, match_data, &t );

		if( ret && r.record.type == HFSP_FOLDER && recursive )
			ret = search_files( &r, 1, proc, match_data, &t );

	} while( ret && !record_next(&r) );

	if( !ret && pt ) {
                char name[256];
                const char *s2 = t.path ? t.path : "";

		unicode_uni2asc( name, &r.key.name, sizeof(name));

		pt->rec = t.rec;
		pt->path = malloc( strlen(name) + strlen(s2) + 2 );
		strcpy( pt->path, name );
		if( strlen(s2) ) {
			strcat( pt->path, "\\" );
			strcat( pt->path, s2 );
		}
	}

	if( t.path )
		free( t.path );

	return ret;
}

static int
root_search_files( fs_ops_t *fs, int recursive, match_proc_t proc, const void *match_data, hfsp_file_t *pt )
{
	volume *vol = (volume*)fs->fs_data;
	record r;

	record_init_root( &r, &vol->catalog );
	return search_files( &r, recursive, proc, match_data, pt );
}

static int
match_file( record *r, record *parent, const void *match_data, hfsp_file_t *pt )
{
        const char *p = (const char*)match_data;
	char name[256];
	int ret=1;

	if( r->record.type != HFSP_FILE )
		return 1;

	(void) unicode_uni2asc(name, &r->key.name, sizeof(name));
	if( !(ret=strcasecmp(p, name)) && pt )
		pt->rec = *r;

	return ret;
}

static int
match_rom( record *r, record *par, const void *match_data, hfsp_file_t *pt )
{
	hfsp_cat_file *file = &r->record.u.file;
	FInfo *fi = &file->user_info;
	int ret = 1;
	char buf[256];

	if( r->record.type == HFSP_FILE && fi->fdCreator == MAC_OS_ROM_CREATOR && fi->fdType == MAC_OS_ROM_TYPE ) {
		ret = search_files( par, 0, match_file, "System", NULL )
			|| search_files( par, 0, match_file, "Finder", NULL );

		(void) unicode_uni2asc(buf, &r->key.name, sizeof(buf));
		if( !strcasecmp("BootX", buf) )
			return 1;

		if( !ret && pt )
			pt->rec = *r;
	}
	return ret;
}

static int
match_path( record *r, record *par, const void *match_data, hfsp_file_t *pt )
{
	char name[256], *s, *next, *org;
	int ret=1;

 	next = org = strdup( (char*)match_data );
	while( (s=strsep( &next, "\\/" )) && !strlen(s) )
		;
	if( !s ) {
		free( org );
		return 1;
	}

	if( *s == ':' && strlen(s) == 5 ) {
		if( r->record.type == HFSP_FILE && !next ) {
			/* match type */
			hfsp_cat_file *file = &r->record.u.file;
			FInfo *fi = &file->user_info;
			int i, type=0;
			for( i=1; s[i] && i<=4; i++ )
				type = (type << 8) | s[i];
			/* printk("fi->fdType: %s / %s\n", s+1, b ); */
			if( fi->fdType == type ) {
				if( pt )
					pt->rec = *r;
				ret = 0;
			}
		}
	} else {
		(void) unicode_uni2asc(name, &r->key.name, sizeof(name));

		if( !strcasecmp(s, name) ) {
			if( r->record.type == HFSP_FILE && !next ) {
				if( pt )
					pt->rec = *r;
				ret = 0;
			} else /* must be a directory */
				ret = search_files( r, 0, match_path, next, pt );
		}
	}
	free( org );
	return ret;
}


/************************************************************************/
/*	File System Operations						*/
/************************************************************************/

static void
close_fs( fs_ops_t *fs )
{
	volume *vol = (volume*)fs->fs_data;
	volume_close( vol );
	free( vol );
}

static file_desc_t *
_create_fops( hfsp_file_t *t )
{
	hfsp_file_t *r = malloc( sizeof(hfsp_file_t) );

	*r = *t;
	r->pos = 0;
	return (file_desc_t*)r;
}

static file_desc_t *
open_path( fs_ops_t *fs, const char *path )
{
	hfsp_file_t t;
	volume *vol = (volume*)fs->fs_data;
	record r;

	/* Leading \\ means system folder. The finder info block has
	 * the following meaning.
	 *
	 *  [0] Prefered boot directory ID
	 *  [3] MacOS 9 boot directory ID
	 *  [5] MacOS X boot directory ID
	 */
	if( !strncmp(path, "\\\\", 2) ) {
		int *p = (int*)&vol->vol.finder_info[0];
		int cnid = p[0];
		/* printk(" p[0] = %x, p[3] = %x, p[5] = %x\n", p[0], p[3], p[5] ); */
		if( p[0] == p[5] && p[3] )
			cnid = p[3];
		if( record_init_cnid(&r, &vol->catalog, cnid) )
			return NULL;
		path += 2;
	} else {
		record_init_root( &r, &vol->catalog );
	}

	if( !search_files(&r, 0, match_path, path, &t) )
		return _create_fops( &t );
	return NULL;
}

static file_desc_t *
search_rom( fs_ops_t *fs )
{
	hfsp_file_t t;

	if( !root_search_files(fs, 1, match_rom, NULL, &t) )
		return _create_fops( &t );
	return NULL;
}

static file_desc_t *
search_file( fs_ops_t *fs, const char *name )
{
	hfsp_file_t t;

	if( !root_search_files(fs, 1, match_file, name, &t) )
		return _create_fops( &t );
	return NULL;
}


/************************************************************************/
/*	File Operations							*/
/************************************************************************/

static char *
get_path( file_desc_t *fd, char *buf, int len )
{
	hfsp_file_t *t = (hfsp_file_t*)fd;
	if( !t->path )
		return NULL;

	strncpy( buf, t->path, len );
	buf[len-1] = 0;
	return buf;
}

static void
file_close( file_desc_t *fd )
{
	hfsp_file_t *t = (hfsp_file_t*)fd;
	if( t->path )
		free( t->path );
	free( t );
}

static int
file_lseek( file_desc_t *fd, off_t offs, int whence )
{
	hfsp_file_t *t = (hfsp_file_t*)fd;
	hfsp_cat_file *file = &t->rec.record.u.file;
	int total = file->data_fork.total_size;

	switch( whence ){
	case SEEK_CUR:
		t->pos += offs;
		break;
	case SEEK_END:
		t->pos = total + offs;
		break;
	default:
	case SEEK_SET:
		t->pos = offs;
		break;
	}

	if( t->pos < 0 )
		t->pos = 0;

	if( t->pos > total )
		t->pos = total;

	return t->pos;
}

static int
file_read( file_desc_t *fd, void *buf, size_t count )
{
	hfsp_file_t *t = (hfsp_file_t*)fd;
	volume *vol = t->rec.tree->vol;
	UInt32 blksize = vol->blksize;
	hfsp_cat_file *file = &t->rec.record.u.file;
	blockiter iter;
	char buf2[blksize];
	int act_count, curpos=0;

	blockiter_init( &iter, vol, &file->data_fork, HFSP_EXTENT_DATA, file->id );
	while( curpos + blksize < t->pos ) {
		if( blockiter_next( &iter ) )
			return -1;
		curpos += blksize;
	}
	act_count = 0;

	while( act_count < count ){
		UInt32 block = blockiter_curr(&iter);
		int max = blksize, add = 0, size;

		if( volume_readinbuf( vol, buf2, block ) )
			break;

		if( curpos < t->pos ){
			add += t->pos - curpos;
			max -= t->pos - curpos;
		}
		size = (count-act_count > max)? max : count-act_count;
		memcpy( (char *)buf + act_count, &buf2[add], size );

		curpos += blksize;
		act_count += size;

		if( blockiter_next( &iter ) )
			break;
	}

	t->pos += act_count;
	return (act_count > 0)? act_count : -1;
}

static char *
vol_name( fs_ops_t *fs, char *buf, int size )
{
	return get_hfs_vol_name( fs->fd, buf, size );
}

static const char *
get_fstype( fs_ops_t *fs )
{
	return ("HFS+");
}


static const fs_ops_t fs_ops = {
	.close_fs	= close_fs,
	.open_path	= open_path,
	.search_rom	= search_rom,
	.search_file	= search_file,
	.vol_name	= vol_name,

	.get_path	= get_path,
	.close		= file_close,
	.read		= file_read,
	.lseek		= file_lseek,

	.get_fstype     = get_fstype
};

int
fs_hfsp_open( int os_fd, fs_ops_t *fs )
{
	volume *vol = malloc( sizeof(volume) );

	if( volume_open(vol, os_fd) ) {
		free( vol );
		return -1;
	}
	*fs = fs_ops;
	fs->fs_data = vol;

	return 0;
}

int 
fs_hfsp_probe(int fd, llong offs)
{
	if (volume_probe(fd, offs))
		return 0;

	return -1;
}
