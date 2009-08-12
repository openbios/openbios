/*
 *	<grubfs_fs.c>
 *
 *	grub vfs
 *
 *   Copyright (C) 2004 Stefan Reinauer
 *   Copyright (C) 2004 Samuel Rydh
 *
 *   inspired by HFS code from Samuel Rydh
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation
 *
 */

#include "openbios/config.h"
#include "openbios/bindings.h"
#include "openbios/fs.h"
#include "filesys.h"
#include "glue.h"
#include "libc/diskio.h"

/************************************************************************/
/* 	grub GLOBALS (horrible... but difficult to fix)			*/
/************************************************************************/

/* the grub drivers want these: */
int		filepos;
int		filemax;
grub_error_t	errnum;
char		FSYS_BUF[FSYS_BUFLEN];

/* these are not even used by us, instead
 * the grub fs drivers want them:
 */
int		fsmax;
void		(*disk_read_hook) (int, int, int);
void		(*disk_read_func) (int, int, int);


/************************************************************************/
/*	filsystem table							*/
/************************************************************************/

typedef struct fsys_entry {
        const char *name;
	int	(*mount_func) (void);
	int	(*read_func) (char *buf, int len);
	int	(*dir_func) (char *dirname);
	void	(*close_func) (void);
	int	(*embed_func) (int *start_sector, int needed_sectors);
} fsys_entry_t;

static const struct fsys_entry fsys_table[] = {
# ifdef CONFIG_FSYS_FAT
    {"fat", fat_mount, fat_read, fat_dir, NULL, NULL},
# endif
# ifdef CONFIG_FSYS_EXT2FS
    {"ext2fs", ext2fs_mount, ext2fs_read, ext2fs_dir, NULL, NULL},
# endif
# ifdef CONFIG_FSYS_MINIX
    {"minix", minix_mount, minix_read, minix_dir, NULL, NULL},
# endif
# ifdef CONFIG_FSYS_REISERFS
    {"reiserfs", reiserfs_mount, reiserfs_read, reiserfs_dir, NULL, reiserfs_embed},
# endif
# ifdef CONFIG_FSYS_JFS
    {"jfs", jfs_mount, jfs_read, jfs_dir, NULL, jfs_embed},
# endif
# ifdef CONFIG_FSYS_XFS
    {"xfs", xfs_mount, xfs_read, xfs_dir, NULL, NULL},
# endif
# ifdef CONFIG_FSYS_UFS
    {"ufs", ufs_mount, ufs_read, ufs_dir, NULL, ufs_embed},
# endif
# ifdef CONFIG_FSYS_ISO9660
    {"iso9660", iso9660_mount, iso9660_read, iso9660_dir, NULL, NULL},
# endif
# ifdef CONFIG_FSYS_NTFS
    {"ntfs", ntfs_mount, ntfs_read, ntfs_dir, NULL, NULL},
# endif
# ifdef CONFIG_FSYS_AFFS
    {"affs", affs_mount, affs_read, affs_dir, NULL, NULL},
# endif
};

/* We don't provide a file search mechanism (yet) */
typedef struct {
	unsigned long	pos;
	unsigned long	len;
	const char	*path;
	const fs_ops_t	*fs;
} grubfile_t;

typedef struct {
	const struct fsys_entry *fsys;
	grubfile_t *fd;
	int		dev_fd;
} grubfs_t;

static grubfs_t dummy_fs;
static grubfs_t 	*curfs=&dummy_fs;

/************************************************************************/
/*	file/fs ops							*/
/************************************************************************/

static void
grubfs_file_close( file_desc_t *fd )
{
	grubfile_t *gf = (grubfile_t *)fd;

	if (gf->path)
		free((void *)(gf->path));
	free(fd);
	filepos=0;
	filemax=0;
}

static int
grubfs_file_lseek( file_desc_t *fd, off_t offs, int whence )
{
	grubfile_t *file = (grubfile_t*)fd;
	unsigned long newpos;

	switch( whence ) {
	case SEEK_CUR:
		if (offs < 0 && (unsigned long) -offs > file->pos)
			newpos = 0;
		else
			newpos = file->pos + offs;
		break;
	case SEEK_END:
		if (offs < 0 && (unsigned long) -offs > file->len)
			newpos = 0;
		else
			newpos = file->len + offs;
		break;
	default:
	case SEEK_SET:
		newpos = (offs < 0) ? 0 : offs;
		break;
	}
	if (newpos > file->len)
		newpos = file->len;

	file->pos = newpos;

	return newpos;
}

static int
grubfs_file_read( file_desc_t *fd, void *buf, size_t count )
{
	grubfile_t *file = (grubfile_t*)fd;
        int ret;

        curfs = (grubfs_t *)file->fs->fs_data;

	filepos=file->pos;
	filemax=file->len;

	if (count > filemax - filepos)
		count = filemax - filepos;

	ret=curfs->fsys->read_func(buf, count);

	file->pos=filepos;
	return ret;
}

static char *
get_path( file_desc_t *fd, char *retbuf, int len )
{
	const char *path=((grubfile_t *)fd)->path;

	if(strlen(path) > len)
		return NULL;

	strcpy( retbuf, path );

	return retbuf;
}

static file_desc_t *
open_path( fs_ops_t *fs, const char *path )
{
	grubfile_t *ret = NULL;
        char *s = (char *)path;

	curfs = (grubfs_t *)fs->fs_data;

	while(*s) {
		if(*s=='\\') *s='/';
		s++;
	}
#ifdef CONFIG_DEBUG_FS
	printk("Path=%s\n",path);
#endif
	if (!curfs->fsys->dir_func((char *) path)) {
		printk("File not found\n");
		return NULL;
	}
	ret=malloc(sizeof(grubfile_t));

	ret->pos=filepos;
	ret->len=filemax;
	ret->path=strdup(path);
	ret->fs=fs;

	return (file_desc_t *)ret;
}

static void
close_fs( fs_ops_t *fs )
{
	free( fs->fs_data );
	fs->fs_data = NULL;

	/* callers responsibility to call free(fs) */
}

static const char *
grubfs_get_fstype( fs_ops_t *fs )
{
	grubfs_t *gfs = (grubfs_t*)fs->fs_data;
	return gfs->fsys->name;
}

static const fs_ops_t grubfs_ops = {
	.close_fs	= close_fs,
	.open_path	= open_path,
	.get_path	= get_path,
	.close		= grubfs_file_close,
	.read		= grubfs_file_read,
	.lseek		= grubfs_file_lseek,

	.get_fstype	= grubfs_get_fstype,
};

/* mount */
int
fs_grubfs_open( int fd, fs_ops_t *fs )
{
	grubfs_t *gfs;
	int i;

	curfs=&dummy_fs;

	curfs->dev_fd = fd;

	for (i = 0; i < sizeof(fsys_table)/sizeof(fsys_table[0]); i++) {
#ifdef CONFIG_DEBUG_FS
		printk("Trying %s\n", fsys_table[i].name);
#endif
		if (fsys_table[i].mount_func()) {
			const fsys_entry_t *fsys = &fsys_table[i];
#ifdef CONFIG_DEBUG_FS
			printk("Mounted %s\n", fsys->name);
#endif

			gfs = malloc(sizeof(*gfs));
			gfs->fsys = fsys;
			gfs->dev_fd = fd;

			*fs=grubfs_ops;
			fs->fs_data = (void*)gfs;
			return 0;
		}
	}
#ifdef CONFIG_DEBUG_FS
	printk("Unknown filesystem type\n");
#endif
	return -1;
}


/************************************************************************/
/*	I/O glue (called by grub source)				*/
/************************************************************************/

int
devread( unsigned long sector, unsigned long byte_offset,
	 unsigned long byte_len, void *buf )
{
	llong offs = (llong)sector * 512 + byte_offset;

#ifdef CONFIG_DEBUG_FS
	//printk("devread s=%x buf=%x, fd=%x\n",sector, buf, curfs->dev_fd);
#endif

	if( !curfs ) {
#ifdef CONFIG_DEBUG_FS
		printk("devread: fsys == NULL!\n");
#endif
		return -1;
	}

	if( seek_io(curfs->dev_fd, offs) ) {
#ifdef CONFIG_DEBUG_FS
		printk("seek failure\n");
#endif
		return -1;
	}
	return (read_io(curfs->dev_fd, buf, byte_len) == byte_len) ? 1:0;
}

int
file_read( void *buf, unsigned long len )
{
	if (filepos < 0 || filepos > filemax)
		filepos = filemax;
	if (len > filemax-filepos)
		len = filemax - filepos;
	errnum = 0;
	return curfs->fsys->read_func( buf, len );
}
