/*
 *
 * (c) 2008-2009 Laurent Vivier <Laurent@lvivier.info>
 *
 * This file has been copied from EMILE, http://emile.sf.net
 *
 */

#include "libext2.h"
#include "ext2_utils.h"
#include "fs/fs.h"
#include "libc/vsprintf.h"

typedef struct {
	enum { FILE, DIR } type;
	union {
		ext2_FILE *file;
		ext2_DIR *dir;
	};
} ext2_COMMON;

static void
umount( fs_ops_t *fs )
{
        ext2_VOLUME *volume = (ext2_VOLUME *)fs->fs_data;

        ext2_umount( volume );
}

static const int days_month[12] =
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static const int days_month_leap[12] =
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static inline int is_leap(int year)
{
	return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

static void
print_date(time_t sec)
{
	unsigned int second, minute, hour, month, day, year;
	int current;
	const int *days;

	second = sec % 60;
	sec /= 60;

	minute = sec % 60;
	sec /= 60;

	hour = sec % 24;
	sec /= 24;

	year = sec * 100 / 36525;
	sec -= year * 36525 / 100;
	year += 1970;

	days = is_leap(year) ?  days_month_leap : days_month;

	current = 0;
	month = 0;
	while (month < 12) {
		if (sec <= current + days[month]) {
			break;
		}
		current += days[month];
		month++;
	}
	month++;

	day = sec - current + 1;

	forth_printf("%d-%02d-%02d %02d:%02d:%02d ",
		     year, month, day, hour, minute, second);
}

static void
dir_fs ( file_desc_t *fd)
{
	ext2_COMMON *common = (ext2_COMMON *)fd;
	struct ext2_dir_entry_2 *entry;
	struct ext2_inode inode;

	if (common->type != DIR)
		return;

	forth_printf("\n");
	while ( (entry = ext2_readdir(common->dir)) ) {
		ext2_get_inode(common->dir->volume, entry->inode, &inode);
		forth_printf("% 10d ", inode.i_size);
		print_date(inode.i_mtime);
		if (S_ISDIR(inode.i_mode))
			forth_printf("%s\\\n", entry->name);
		else
			forth_printf("%s\n", entry->name);
	}
}

static file_desc_t *
open_path( fs_ops_t *fs, const char *path )
{
	ext2_VOLUME *volume = (ext2_VOLUME *)fs->fs_data;
	ext2_COMMON *common;

	common = (ext2_COMMON*)malloc(sizeof(*common));
	if (common == NULL)
		return NULL;

	common->dir = ext2_opendir(volume, path);
	if (common->dir == NULL) {
		common->file = ext2_open(volume, path);
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
	ext2_COMMON *common =(ext2_COMMON *)fd;

	if (common->type != FILE)
		return NULL;

	strncpy(buf, common->file->path, size);

	return buf;
}

static int
file_lseek( file_desc_t *fd, off_t offs, int whence )
{
	ext2_COMMON *common =(ext2_COMMON *)fd;

	if (common->type != FILE)
		return -1;

	return ext2_lseek(common->file, offs, whence);
}

static void
file_close( file_desc_t *fd )
{
	ext2_COMMON *common =(ext2_COMMON *)fd;

	if (common->type == FILE)
		ext2_close(common->file);
	else if (common->type == DIR)
		ext2_closedir(common->dir);
	free(common);
}

static int
file_read( file_desc_t *fd, void *buf, size_t count )
{
	ext2_COMMON *common =(ext2_COMMON *)fd;

	if (common->type != FILE)
		return -1;

	return ext2_read(common->file, buf, count);
}

static const char *
get_fstype( fs_ops_t *fs)
{
	return "EXT2";
}

static const fs_ops_t ext2_ops = {
	.dir		= dir_fs,
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

int fs_ext2_probe(int fd, llong offs)
{
	if (ext2_probe(fd, offs))
		return -1;

	return 0;
}
