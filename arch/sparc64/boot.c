/*
 *
 */
#undef BOOTSTRAP
#include "config.h"
#include "libopenbios/bindings.h"
#include "arch/common/nvram.h"
#include "libc/diskio.h"
#include "libc/vsprintf.h"
#include "libopenbios/sys_info.h"
#include "libopenbios/elf_load.h"
#include "libopenbios/aout_load.h"
#include "libopenbios/fcode_load.h"
#include "libopenbios/forth_load.h"
#include "boot.h"

struct sys_info sys_info;
uint64_t kernel_image;
uint64_t kernel_size;
uint64_t qemu_cmdline;
uint64_t cmdline_size;
char boot_device;
void *boot_notes = NULL;
extern int sparc64_of_client_interface( int *params );


static int try_path(const char *path, char *param)
{
	ucell valid;
	ihandle_t dev;

	/* Open device used by this path */
	dev = open_dev(path);

#ifdef CONFIG_LOADER_ELF
	/* ELF Boot loader */
	elf_load(&sys_info, path, param, &boot_notes);
	feval("state-valid @");
	valid = POP();
	if (valid)
		goto start_image;
#endif

	/* Linux loader (not using Forth) */
	linux_load(&sys_info, path, param);

#ifdef CONFIG_LOADER_AOUT
	/* a.out loader */
	aout_load(&sys_info, dev);
	feval("state-valid @");
	valid = POP();
	if (valid)
		goto start_image;
#endif

#ifdef CONFIG_LOADER_FCODE
	/* Fcode loader */
	fcode_load(dev);
	feval("state-valid @");
	valid = POP();
	if (valid)
		goto start_image;
#endif

#ifdef CONFIG_LOADER_FORTH
	/* Forth loader */
	forth_load(dev);
	feval("state-valid @");
	valid = POP();
	if (valid)
		goto start_image;
#endif

	close_dev(dev);

	return 0;


start_image:
	go();
	return -1;
}


void go(void)
{
	ucell address, type, size;
	int image_retval = 0;

	/* Get the entry point and the type (see forth/debugging/client.fs) */
	feval("saved-program-state >sps.entry @");
	address = POP();
	feval("saved-program-state >sps.file-type @");
	type = POP();
	feval("saved-program-state >sps.file-size @");
	size = POP();

	printk("\nJumping to entry point " FMT_ucellx " for type " FMT_ucellx "...\n", address, type);

	switch (type) {
		case 0x0:
			/* Start ELF boot image */
			image_retval = start_elf(address, (uint64_t)&boot_notes);
			break;

		case 0x1:
			/* Start ELF image */
			image_retval = start_client_image(address, (uint64_t)&sparc64_of_client_interface);
			break;

		case 0x5:
			/* Start a.out image */
			image_retval = start_client_image(address, (uint64_t)&sparc64_of_client_interface);
			break;

		case 0x10:
			/* Start Fcode image */
			printk("Evaluating FCode...\n");
			PUSH(address);
			PUSH(1);
			fword("byte-load");
			image_retval = 0;
			break;

		case 0x11:
			/* Start Forth image */
			PUSH(address);
			PUSH(size);
			fword("eval2");
			image_retval = 0;
			break;
	}

	printk("Image returned with return value %#x\n", image_retval);
}


void boot(void)
{
	char *path=pop_fstr_copy(), *param;
        char altpath[256];
	int result;

        if (kernel_size) {
            void (*entry)(unsigned long p1, unsigned long p2, unsigned long p3,
                          unsigned long p4, unsigned long p5);;

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

	result = try_path(path, param);
	if (!result) {
		snprintf(altpath, sizeof(altpath), "%s:f", path);
		try_path(altpath, param);
	}

	printk("Unsupported image format\n");

	free(path);
}
