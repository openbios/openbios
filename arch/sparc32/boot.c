/*
 *
 */
#undef BOOTSTRAP
#include "openbios/config.h"
#include "openbios/bindings.h"
#include "openbios/elfload.h"
#include "openbios/nvram.h"
#include "openbios/drivers.h"
#include "libc/diskio.h"
#include "libc/vsprintf.h"
#include "sys_info.h"
#include "openprom.h"
#include "boot.h"

struct sys_info sys_info;
uint32_t kernel_image;
uint32_t kernel_size;
uint32_t qemu_cmdline;
uint32_t cmdline_size;
char boot_device;

void boot(void)
{
        char *path = pop_fstr_copy(), *param, *oldpath = path;
        char altpath[256];
        int unit = 0;
        const void *romvec;

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
                    oldpath = "fd()";
                    break;
                case 'c':
                    path = strdup("disk");
                    oldpath = "sd(0,0,0):d";
                    break;
                default:
                case 'd':
                    path = strdup("cdrom");
                    // FIXME: hardcoding this looks almost definitely wrong.
                    // With sd(0,2,0):b we get to see the solaris kernel though
                    //oldpath = "sd(0,2,0):d";
                    oldpath = "sd(0,2,0):b";
                    unit = 2;
                    break;
                case 'n':
                    path = strdup("net");
                    oldpath = "le()";
                    break;
                }
            }
	}

        obp_arg.boot_dev_ctrl = 0;
        obp_arg.boot_dev_unit = unit;
        obp_arg.dev_partition = 0;
        obp_arg.boot_dev[0] = oldpath[0];
        obp_arg.boot_dev[1] = oldpath[1];
        obp_arg.argv[0] = oldpath;
        obp_arg.argv[1] = (void *)(long)qemu_cmdline;

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
            obp_arg.argv[1] = param;
        }

        romvec = init_openprom(qemu_mem_size, path);

        if (kernel_size) {
            int (*entry)(const void *romvec_ptr, int p2, int p3, int p4, int p5);

            printk("[sparc] Kernel already loaded\n");
            entry = (void *) kernel_image;
            entry(romvec, 0, 0, 0, 0);
        }

	printk("[sparc] Booting file '%s' ", path);
	if (param)
		printk("with parameters '%s'\n", param);
	else
		printk("without parameters.\n");

	if (elf_load(&sys_info, path, param, romvec) == LOADER_NOT_SUPPORT)
            if (linux_load(&sys_info, path, param) == LOADER_NOT_SUPPORT)
                if (aout_load(&sys_info, path, romvec) == LOADER_NOT_SUPPORT) {

                    snprintf(altpath, sizeof(altpath), "%s:d", path);

                    if (elf_load(&sys_info, altpath, param, romvec)
                        == LOADER_NOT_SUPPORT)
                        if (linux_load(&sys_info, altpath, param)
                            == LOADER_NOT_SUPPORT)
                            if (aout_load(&sys_info, altpath, romvec)
                                == LOADER_NOT_SUPPORT)
                                printk("Unsupported image format\n");
                }

	free(path);
}
