/* tag: openbios loader prototypes for sparc64
 *
 * Copyright (C) 2004 Stefan Reinauer
 *
 * See the file "COPYING" for further information about
 * the copyright and warranty status of this work.
 */

int forth_load(struct sys_info *info, const char *filename, const char *cmdline);
int elf_load(struct sys_info *info, const char *filename, const char *cmdline);
int linux_load(struct sys_info *info, const char *file, const char *cmdline);
int aout_load(struct sys_info *info, const char *filename, const char *cmdline);

uint64_t start_elf(uint64_t entry_point, uint64_t param);

void boot(void);

extern uint64_t kernel_image;
extern uint64_t kernel_size;
extern uint64_t qemu_cmdline;
extern uint64_t cmdline_size;
extern char boot_device;
extern struct sys_info sys_info;
