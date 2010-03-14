/*
 *
 * (c) 2009 Laurent Vivier <Laurent@vivier.eu>
 *
 */

#include "libiso9660.h"
#include "fs/fs.h"
#include "libc/vsprintf.h"

typedef struct {
	enum { FILE, DIR } type;
	union {
		iso9660_FILE *file;
		iso9660_DIR * dir;
	};
} iso9660_COMMON;

static void
umount( fs_ops_t *fs )
{
	iso9660_VOLUME *volume = (iso9660_VOLUME *)fs->fs_data;

	iso9660_umount( volume );
}

static void
dir_fs ( file_desc_t *fd )
{
	iso9660_COMMON *common = (iso9660_COMMON *)fd;
	struct iso_directory_record *idr;
	char name_buf[256];

	if (common->type != DIR)
		return;

	forth_printf("\n");
	while ( (idr = iso9660_readdir(common->dir)) ) {

		forth_printf("% 10d ", isonum_733(idr->size));
		forth_printf("%d-%02d-%02d %02d:%02d:%02d ",
			     idr->date[0] + 1900, /* year */
			     idr->date[1], /* month */
                             idr->date[2], /* day */
			     idr->date[3], idr->date[4], idr->date[5]);
		iso9660_name(common->dir->volume, idr, name_buf);
		if (idr->flags[0] & 2)
			forth_printf("%s\\\n", name_buf);
		else
			forth_printf("%s\n", name_buf);
	}
}

static file_desc_t *
open_path( fs_ops_t *fs, const char *path )
{
	iso9660_VOLUME *volume = (iso9660_VOLUME *)fs->fs_data;
	iso9660_COMMON *common;

	common = (iso9660_COMMON *)malloc(sizeof(*common));
	if (common == NULL)
		return NULL;

	common->dir = iso9660_opendir(volume, path);
	if (common->dir == NULL) {
		common->file = iso9660_open(volume, path);
		if (common->file == NULL) {
			free(common);
			return NULL;
		}
		common->type = FILE;
		return (file_desc_t *)common;
	}
	common->type = DIR;
	return (file_desc_t *)common;
}

static char *
get_path( file_desc_t *fd, char *buf, int size )
{
	iso9660_COMMON *common = (iso9660_COMMON *)fd;

	if (common->type != FILE)
		return NULL;

	strncpy(buf, common->file->path, size);

	return buf;
}

static int
file_lseek( file_desc_t *fd, off_t offs, int whence )
{
	iso9660_COMMON *common = (iso9660_COMMON *)fd;

	if (common->type != FILE)
		return -1;

	return iso9660_lseek(common->file, offs, whence);
}

static void
file_close( file_desc_t *fd )
{
	iso9660_COMMON *common = (iso9660_COMMON *)fd;

	if (common->type == FILE)
		iso9660_close(common->file);
	else if (common->type == DIR)
		iso9660_closedir(common->dir);
	free(common);
}

static int
file_read( file_desc_t *fd, void *buf, size_t count )
{
	iso9660_COMMON *common = (iso9660_COMMON *)fd;

	if (common->type != FILE)
		return -1;

	return iso9660_read(common->file, buf, count);
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
	.dir		= dir_fs,
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
