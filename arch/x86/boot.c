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
#include "libopenbios/initprogram.h"
#include "libopenbios/sys_info.h"
#include "boot.h"

void go(void)
{
	ucell address, type;
	int image_retval = 0;

	/* Get the entry point and the type (see forth/debugging/client.fs) */
	feval("load-state >ls.entry @");
	address = POP();
	feval("load-state >ls.file-type @");
	type = POP();

	printk("\nJumping to entry point " FMT_ucellx " for type " FMT_ucellx "...\n", address, type);

	switch (type) {
		case 0x0:
			/* Start ELF boot image */
			image_retval = start_elf(address);
			break;

		case 0x1:
			/* Start ELF image */
			image_retval = start_elf(address);
			break;

		case 0x5:
			/* Start a.out image */
			image_retval = start_elf(address);
			break;

		case 0x10:
			/* Start Fcode image */
			image_retval = start_elf((unsigned long)&init_fcode_context);
			break;

		case 0x11:
			/* Start Forth image */
			image_retval = start_elf((unsigned long)&init_forth_context);
			break;
	}

	printk("Image returned with return value %#x\n", image_retval);
}


void boot(void)
{
	/* No platform-specific boot code */
	return;
}
