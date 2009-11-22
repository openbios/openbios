/*
 *
 * (c) 2009 Laurent Vivier <Laurent@vivier.eu>
 *
 */

#include "libiso9660.h"
#include "openbios/fs.h"

static void
umount( fs_ops_t *fs )
{
	iso9660_VOLUME *volume = (iso9660_VOLUME *)fs->fs_data;

	iso9660_umount( volume );
}

static file_desc_t *
open_path( fs_ops_t *fs, const char *path )
{
	iso9660_VOLUME *volume = (iso9660_VOLUME *)fs->fs_data;
	iso9660_FILE *file;

	file = iso9660_open(volume, path);

	return (file_desc_t *)file;
}

static char *
get_path( file_desc_t *fd, char *buf, int size )
{
	iso9660_FILE *file = (iso9660_FILE*)fd;

	strncpy(buf, file->path, size);

	return buf;
}

static int
file_lseek( file_desc_t *fd, off_t offs, int whence )
{
	iso9660_FILE *file = (iso9660_FILE*)fd;

	return iso9660_lseek(file, offs, whence);
}

static void
file_close( file_desc_t *fd )
{
	iso9660_FILE *file = (iso9660_FILE*)fd;

	iso9660_close(file);
}

static int
file_read( file_desc_t *fd, void *buf, size_t count )
{
	iso9660_FILE *file = (iso9660_FILE*)fd;

	return iso9660_read(file, buf, count);
}

static char *
vol_name( fs_ops_t *fs, char *buf, int size )
{
	iso9660_VOLUME *volume = (iso9660_VOLUME *)fs->fs_data;

	strncpy(buf, volume->descriptor->volume_id, size);

	return buf;
}

static const char *
get_fstype( fs_ops_t *fs )
{
	return "ISO9660";
}

static const fs_ops_t iso9660_ops = {
	.close_fs	= umount,

	.open_path	= open_path,
	.get_path	= get_path,
	.close		= file_close,
	.read		= file_read,
	.lseek		= file_lseek,

	.vol_name	= vol_name,
	.get_fstype	= get_fstype,
};

int fs_iso9660_open(int fd, fs_ops_t *fs)
{
	iso9660_VOLUME *volume;

	volume = iso9660_mount(fd);
	if (volume == NULL)
		return -1;

	*fs = iso9660_ops;
	fs->fs_data = volume;

	return 0;
}
