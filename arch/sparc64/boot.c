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
        char altpath[256];
	
        if (kernel_size) {
            void (*entry)(unsigned long p1, unsigned long p2, unsigned long p3,
                          unsigned long p4, unsigned long p5);
            extern int of_client_interface( int *params );

            printk("[sparc64] Kernel already loaded\n");
            entry = (void *) (unsigned long)kernel_image;
            entry(0, 0, 0, 0, (unsigned long)&of_client_interface);
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
	
	printk("[sparc64] Booting file '%s' ", path);
	if(param) 
		printk("with parameters '%s'\n", param);
	else
		printk("without parameters.\n");


	if (elf_load(&sys_info, path, param) == LOADER_NOT_SUPPORT)
            if (linux_load(&sys_info, path, param) == LOADER_NOT_SUPPORT) {

                    sprintf(altpath, "%s:d", path);

                    if (elf_load(&sys_info, altpath, param) == LOADER_NOT_SUPPORT)
                        if (linux_load(&sys_info, altpath, param) == LOADER_NOT_SUPPORT)
                            printk("Unsupported image format\n");
                }

	free(path);
}
