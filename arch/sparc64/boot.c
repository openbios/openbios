/*
 *
 */
#undef BOOTSTRAP
#include "config.h"
#include "libopenbios/bindings.h"
#include "libopenbios/elfload.h"
#include "openbios/nvram.h"
#include "libc/diskio.h"
#include "libc/vsprintf.h"
#include "libopenbios/sys_info.h"
#include "boot.h"

struct sys_info sys_info;
uint64_t kernel_image;
uint64_t kernel_size;
uint64_t qemu_cmdline;
uint64_t cmdline_size;
char boot_device;

void boot(void)
{
	char *path=pop_fstr_copy(), *param;
        char altpath[256];

        if (kernel_size) {
            void (*entry)(unsigned long p1, unsigned long p2, unsigned long p3,
                          unsigned long p4, unsigned long p5);
            extern int sparc64_of_client_interface( int *params );

            printk("[sparc64] Kernel already loaded\n");
            entry = (void *) (unsigned long)kernel_image;
            entry(0, 0, 0, 0, (unsigned long)&sparc64_of_client_interface);
        }

	if(!path) {
            push_str("boot-device");
            push_str("/options");
            fword("(find-dev)");
            POP();
            fword("get-package-property");
            if (!POP()) {
                path = pop_fstr_copy();
            } else {
                switch (boot_device) {
                case 'a':
                    path = strdup("/obio/SUNW,fdtwo");
                    break;
                case 'c':
                    path = strdup("disk");
                    break;
                default:
                case 'd':
                    path = strdup("cdrom");
                    break;
                case 'n':
                    path = strdup("net");
                    break;
                }
            }
	}

	param = strchr(path, ' ');
	if(param) {
		*param = '\0';
		param++;
	} else if (cmdline_size) {
            param = (char *)qemu_cmdline;
        } else {
            push_str("boot-args");
            push_str("/options");
            fword("(find-dev)");
            POP();
            fword("get-package-property");
            POP();
            param = pop_fstr_copy();
        }

	printk("[sparc64] Booting file '%s' ", path);
	if (param)
		printk("with parameters '%s'\n", param);
	else
		printk("without parameters.\n");


	if (elf_load(&sys_info, path, param) == LOADER_NOT_SUPPORT)
            if (linux_load(&sys_info, path, param) == LOADER_NOT_SUPPORT)
                if (aout_load(&sys_info, path) == LOADER_NOT_SUPPORT)
                    if (fcode_load(path) == LOADER_NOT_SUPPORT) {

                        snprintf(altpath, sizeof(altpath), "%s:f", path);

                        if (elf_load(&sys_info, altpath, param)
                            == LOADER_NOT_SUPPORT)
                            if (linux_load(&sys_info, altpath, param)
                                == LOADER_NOT_SUPPORT)
                                if (aout_load(&sys_info, altpath)
                                    == LOADER_NOT_SUPPORT)
                                    if (fcode_load(altpath)
                                        == LOADER_NOT_SUPPORT)
                                        printk("Unsupported image format\n");
                    }

	free(path);
}
