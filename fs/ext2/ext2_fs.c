/*
 *
 * (c) 2008-2009 Laurent Vivier <Laurent@lvivier.info>
 *
 * This file has been copied from EMILE, http://emile.sf.net
 *
 */

#include "libext2.h"
#include "ext2_utils.h"
#include "openbios/fs.h"

static void
umount( fs_ops_t *fs )
{
        ext2_VOLUME *volume = (ext2_VOLUME *)fs->fs_data;

        ext2_umount( volume );
}

static file_desc_t *
open_path( fs_ops_t *fs, const char *path )
{
	ext2_VOLUME *volume = (ext2_VOLUME *)fs->fs_data;
	ext2_FILE *file;

	file = ext2_open(volume, path);

	return (file_desc_t *)file;
}

static char *
get_path( file_desc_t *fd, char *buf, int size )
{
	ext2_FILE *file = (ext2_FILE *)fd;

	strncpy(buf, file->path, size);

	return buf;
}

static int
file_lseek( file_desc_t *fd, off_t offs, int whence )
{
	ext2_FILE *file = (ext2_FILE *)fd;

	return ext2_lseek(file, offs, whence);
}

static void
file_close( file_desc_t *fd )
{
	ext2_FILE *file = (ext2_FILE *)fd;

	ext2_close(file);
}

static int
file_read( file_desc_t *fd, void *buf, size_t count )
{
	ext2_FILE *file = (ext2_FILE *)fd;

	return ext2_read(file, buf, count);
}

static const char *
get_fstype( fs_ops_t *fs)
{
	return "EXT2";
}

static const fs_ops_t ext2_ops = {
	.close_fs	= umount,

	.open_path	= open_path,
	.get_path	= get_path,
	.close		= file_close,
	.read		= file_read,
	.lseek		= file_lseek,

	.get_fstype	= get_fstype,
};

int fs_ext2_open(int fd, fs_ops_t *fs)
{
	ext2_VOLUME *volume;

	volume = ext2_mount(fd);
	if (volume == NULL)
		return -1;

	*fs = ext2_ops;
	fs->fs_data = volume;

	return 0;
}
