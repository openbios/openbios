#include "config.h"
#include "kernel/kernel.h"
#include "libc/diskio.h"
#include "loadfs.h"

static int load_fd=-1;

int file_open(const char *filename)
{
	load_fd=open_io(filename);
	if(load_fd >= 0)
            seek_io(load_fd, 0);
	return load_fd>-1;
}

void file_close(void)
{
	if(load_fd==-1)
		return;

	close_io(load_fd);
	load_fd=-1;
}

int lfile_read(void *buf, unsigned long len)
{
	int ret = 0;

        if (load_fd >= 0)
            ret=read_io(load_fd, buf, len);
	return ret;
}

int file_seek(unsigned long offset)
{
        if (load_fd >= 0)
            return seek_io(load_fd, offset);
        else
            return -1;
}

unsigned long file_size(void)
{
	llong fpos, fsize;

        if (load_fd < 0)
            return 0;

	/* save current position */
	fpos=tell(load_fd);

	/* go to end of file and get position */
	seek_io(load_fd, -1);
	fsize=tell(load_fd);

	/* go back to old position */
	seek_io(load_fd, 0);
	seek_io(load_fd, fpos);

	return fsize;
}
