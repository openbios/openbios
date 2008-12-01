/* tag: openbios loader prototypes for sparc32
 *
 * Copyright (C) 2004 Stefan Reinauer
 *
 * See the file "COPYING" for further information about
 * the copyright and warranty status of this work.
 */

// forthload.c
int forth_load(const char *filename);

// elfload.c
int elf_load(struct sys_info *, const char *filename, const char *cmdline,
             const void *romvec);

// aout_load.c
int aout_load(struct sys_info *info, const char *filename, const void *romvec);

// linux_load.c
int linux_load(struct sys_info *info, const char *file, const char *cmdline);

// context.c
extern struct context *__context;
unsigned int start_elf(unsigned long entry_point, unsigned long param);

// romvec.c
void *init_openprom(unsigned long memsize);

// boot.c
extern struct sys_info sys_info;
extern const char *bootpath;
void boot(void);

// sys_info.c
extern unsigned int qemu_mem_size;

// romvec.c
extern struct linux_arguments_v0 obp_arg;

// openbios.c
extern int qemu_machine_type;
