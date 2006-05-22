/*
 *
 */
#undef BOOTSTRAP
#include "openbios/config.h"
#include "openbios/bindings.h"
#include "openbios/elfload.h"
#include "openbios/nvram.h"
#include "libc/diskio.h"
#include "sys_info.h"

int elf_load(struct sys_info *, const char *filename, const char *cmdline);
int aout_load(struct sys_info *, const char *filename, const char *cmdline);
int linux_load(struct sys_info *, const char *filename, const char *cmdline);

void boot(void);

struct sys_info sys_info;                                                       
uint32_t kernel_image;
uint32_t kernel_size;
uint32_t cmdline;
uint32_t cmdline_size;
char boot_device;

void boot(void)
{
	char *path=pop_fstr_copy(), *param;
	
        if (kernel_size) {
            extern unsigned int qemu_mem_size;
            void *init_openprom(unsigned long memsize, const char *cmdline, char boot_device);

            int (*entry)(const void *romvec, int p2, int p3, int p4, int p5);
            const void *romvec;

            printk("[sparc] Kernel already loaded\n");
            romvec = init_openprom(qemu_mem_size, (void *)cmdline, boot_device);
            entry = (void *) kernel_image;
            entry(romvec, 0, 0, 0, 0);
        }

	if(!path) {
            switch(boot_device) {
            case 'a':
                path = "/obio/SUNW,fdtwo";
                break;
            case 'c':
                path = "disk";
                break;
            default:
            case 'd':
                path = "cdrom";
                break;
            case 'n':
                path = "net";
                break;
            }
	}

	param = strchr(path, ' ');
	if(param) {
		*param = '\0';
		param++;
	} else if (cmdline_size) {
            param = (char *)cmdline;
        }
	
	printk("[sparc] Booting file '%s' with parameters '%s'\n",path, param);

	if (elf_load(&sys_info, path, param) == LOADER_NOT_SUPPORT)
            if (linux_load(&sys_info, path, param) == LOADER_NOT_SUPPORT)
                if (aout_load(&sys_info, path, param) == LOADER_NOT_SUPPORT)
                    printk("Unsupported image format\n");

	free(path);
}
