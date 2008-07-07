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

void *init_openprom(unsigned long memsize);
void boot(void);

extern struct sys_info sys_info;
extern uint32_t kernel_image;
extern uint32_t kernel_size;
extern uint32_t qemu_cmdline;
extern uint32_t cmdline_size;
extern char boot_device;
extern unsigned int qemu_mem_size;
extern struct linux_arguments_v0 obp_arg;
extern int qemu_machine_type;
