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
#include "libopenbios/elfload.h"
#include "openbios/nvram.h"
#include "libc/diskio.h"
#include "libopenbios/sys_info.h"
#include "boot.h"

struct sys_info sys_info;

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

	if (elf_load(&sys_info, path, param) != LOADER_NOT_SUPPORT)
		goto loaded;
	if (linux_load(&sys_info, path, param) != LOADER_NOT_SUPPORT)
		goto loaded;
	if (forth_load(&sys_info, path, param) != LOADER_NOT_SUPPORT)
		goto loaded;

	printk("Unsupported image format\n");

loaded:
	free(path);
}
