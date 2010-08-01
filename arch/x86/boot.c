/* tag: openbios boot command for x86
 *
 * Copyright (C) 2003-2004 Stefan Reinauer
 *
 * See the file "COPYING" for further information about
 * the copyright and warranty status of this work.
 */

#undef BOOTSTRAP
#include "config.h"
#include "libopenbios/bindings.h"
#include "arch/common/nvram.h"
#include "libc/diskio.h"
#include "libopenbios/sys_info.h"
#include "libopenbios/elf_load.h"
#include "libopenbios/aout_load.h"
#include "libopenbios/fcode_load.h"
#include "libopenbios/forth_load.h"
#include "boot.h"

void *boot_notes = NULL;

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
			image_retval = start_elf(address, (uint32_t)&boot_notes);
			break;

		case 0x1:
			/* Start ELF image */
			image_retval = start_elf(address, (uint32_t)NULL);
			break;

		case 0x5:
			/* Start a.out image */
			image_retval = start_elf(address, (uint32_t)NULL);
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

	if(!path) {
		printk("[x86] Booting default not supported.\n");
		return;
	}

	param = strchr(path, ' ');
	if(param) {
		*param = '\0';
		param++;
	}

	printk("[x86] Booting file '%s' with parameters '%s'\n",path, param);

	try_path(path, param);

	printk("Unsupported image format\n");

	free(path);
}
