/* tag: openbios loader prototypes for sparc64
 *
 * Copyright (C) 2004 Stefan Reinauer
 *
 * See the file "COPYING" for further information about
 * the copyright and warranty status of this work.
 */

// forthload.c
int forth_load(const char *filename);

// elfload.c
int elf_load(struct sys_info *info, const char *filename, const char *cmdline);

// aout_load.c
int aout_load(struct sys_info *info, const char *filename);

// linux_load.c
int linux_load(struct sys_info *info, const char *file, const char *cmdline);

// fcodeload.c
int fcode_load(const char *filename);

// context.c
extern struct context * volatile __context;
uint64_t start_elf(uint64_t entry_point, uint64_t param);
uint64_t start_client_image(uint64_t entry_point, uint64_t cif_handler);

// boot.c
extern struct sys_info sys_info;
extern uint64_t kernel_image;
extern uint64_t kernel_size;
extern uint64_t qemu_cmdline;
extern uint64_t cmdline_size;
extern char boot_device;
void boot(void);

// sys_info.c
extern uint64_t qemu_mem_size;
extern void collect_sys_info(struct sys_info *info);

// console.c
void ob_su_init(uint64_t base, uint64_t offset, int intr);
void cls(void);

// lib.c
void ob_mmu_init(const char *cpuname, uint64_t ram_size);
