int file_open(const char *filename);
int lfile_read(void *buf, unsigned long len);
int file_seek(unsigned long offset);
unsigned long file_size(void);
void file_close(void);
