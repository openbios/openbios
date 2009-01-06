/* tag: openbios loader prototypes for x86
 *
 * Copyright (C) 2004 Stefan Reinauer
 *
 * See the file "COPYING" for further information about
 * the copyright and warranty status of this work.
 */

int forth_load(struct sys_info *info, const char *filename, const char *cmdline);
int elf_load(struct sys_info *info, const char *filename, const char *cmdline);
int linux_load(struct sys_info *info, const char *file, const char *cmdline);

unsigned int start_elf(unsigned long entry_point, unsigned long param);
void boot(void);
