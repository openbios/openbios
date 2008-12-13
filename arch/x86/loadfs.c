#include "openbios/config.h"
#include "openbios/kernel.h"
#include "libc/diskio.h"
#include "loadfs.h"

static int load_fd=-1;

int file_open(const char *filename)
{
	load_fd=open_io(filename);
	/* if(load_fd!=-1)  */ seek_io(load_fd, 0);
	return load_fd>-1;
}

int lfile_read(void *buf, unsigned long len)
{
	int ret;
	ret=read_io(load_fd, buf, len);
	return ret;
}

int file_seek(unsigned long offset)
{
	return seek_io(load_fd, offset);
}

unsigned long file_size(void)
{
	llong fpos, fsize;

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
