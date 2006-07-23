/* tag: openbios loader prototypes for sparc32
 *
 * Copyright (C) 2004 Stefan Reinauer
 *
 * See the file "COPYING" for further information about
 * the copyright and warranty status of this work.
 */

int forth_load(struct sys_info *info, const char *filename, const char *cmdline);
int elf_load(struct sys_info *, const char *filename, const char *cmdline,
             const void *romvec);
int aout_load(struct sys_info *, const char *filename, const char *cmdline,
              const void *romvec);
int linux_load(struct sys_info *, const char *filename, const char *cmdline,
               const void *romvec);

unsigned int start_elf(unsigned long entry_point, unsigned long param);
