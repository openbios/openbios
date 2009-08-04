/* tag: openbios loader prototypes for x86
 *
 * Copyright (C) 2004 Stefan Reinauer
 *
 * See the file "COPYING" for further information about
 * the copyright and warranty status of this work.
 */

/* forthload.c */
int forth_load(struct sys_info *info, const char *filename, const char *cmdline);

/* elfload.c */
int elf_load(struct sys_info *info, const char *filename, const char *cmdline);

/* linux_load.c */
int linux_load(struct sys_info *info, const char *file, const char *cmdline);

/* context.c */
extern struct context *__context;
unsigned int start_elf(unsigned long entry_point, unsigned long param);

/* boot.c */
extern struct sys_info sys_info;
void boot(void);
