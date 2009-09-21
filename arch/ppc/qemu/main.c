/*
 *   Creation Date: <2002/10/02 22:24:24 samuel>
 *   Time-stamp: <2004/03/27 01:57:55 samuel>
 *
 *	<main.c>
 *
 *
 *
 *   Copyright (C) 2002, 2003, 2004 Samuel Rydh (samuel@ibrium.se)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation
 *
 */

#include "openbios/config.h"
#include "openbios/bindings.h"
#include "openbios/elfload.h"
#include "openbios/nvram.h"
#include "libc/diskio.h"
#include "libc/vsprintf.h"
#include "kernel.h"
#include "ofmem.h"
#define NO_QEMU_PROTOS
#include "openbios/fw_cfg.h"

//#define DEBUG_QEMU

#ifdef DEBUG_QEMU
#define SUBSYS_DPRINTF(subsys, fmt, args...) \
    do { printk("%s - %s: " fmt, subsys, __func__ , ##args); } while (0)
#else
#define SUBSYS_DPRINTF(subsys, fmt, args...) \
    do { } while (0)
#endif
#define CHRP_DPRINTF(fmt, args...) SUBSYS_DPRINTF("CHRP", fmt, ##args)
#define ELF_DPRINTF(fmt, args...) SUBSYS_DPRINTF("ELF", fmt, ##args)
#define NEWWORLD_DPRINTF(fmt, args...) SUBSYS_DPRINTF("NEWWORLD", fmt, ##args)

static void
load(const char *path, const char *param)
{
	char buffer[1024];
	if (param)
		sprintf(buffer, "load %s %s", path, param);
	else
		sprintf(buffer, "load %s", path);
	feval(buffer);
}

static char *
get_device( const char *path )
{
	int i;
	static char buf[1024];

	for (i = 0; i < sizeof(buf) && path[i] && path[i] != ':'; i++)
		buf[i] = path[i];
	buf[i] = 0;

	return buf;
}

static int
get_partition( const char *path )
{
	while ( *path && *path != ':' )
		path++;

	if (!*path)
		return -1;
	path++;

	if (!strchr(path, ','))	/* check if there is a ',' */
		return -1;

	return atol(path);
}

static char *
get_filename( const char * path , char **dirname)
{
	static char buf[1024];
	char *filename;

	while ( *path && *path != ':' )
		path++;

	if (!*path) {
		*dirname = NULL;
		return NULL;
	}
	path++;

	while ( *path && isdigit(*path) )
		path++;

	if (*path == ',')
		path++;

	strncpy(buf, path, sizeof(buf));
	buf[sizeof(buf) - 1] = 0;

	filename = strrchr(buf, '\\');
	if (filename) {
		*dirname = buf;
		(*filename++) = 0;
	} else {
		*dirname = NULL;
		filename = buf;
	}

	return filename;
}

static void
encode_bootpath( const char *spec, const char *args )
{
	char path[1024];
	phandle_t chosen_ph = find_dev("/chosen");
	char *filename, *directory;
	int partition;

	filename = get_filename(spec, &directory);
	partition = get_partition(spec);
	if (partition == -1)
		snprintf(path, sizeof(path), "%s:,%s\\%s", get_device(spec),
			 directory, filename);
	else
		snprintf(path, sizeof(path), "%s:%d,%s\\%s", get_device(spec),
			 partition, directory, filename);

        ELF_DPRINTF("bootpath %s bootargs %s\n", path, args);
	set_property( chosen_ph, "bootpath", path, strlen(spec)+1 );
	if (args)
		set_property( chosen_ph, "bootargs", args, strlen(args)+1 );
}

/************************************************************************/
/*	qemu booting							*/
/************************************************************************/
static void
try_path(const char *device, const char* filename, const char *param)
{
    char path[1024];

    if (filename)
        snprintf(path, sizeof(path), "%s%s", device, filename);
    else
        snprintf(path, sizeof(path), "%s", device);

    ELF_DPRINTF("Trying %s %s\n", path, param);

    load(path, param);
    update_nvram();
    ELF_DPRINTF("Transfering control to %s %s\n",
                path, param);
    feval("go");
}

#define OLDWORLD_BOOTCODE_BASEADDR	(0x3f4000)

static void
oldworld_boot( void )
{
	int fd;
	int len, total;
	const char *path = "hd:,%BOOT";
	char *bootcode;

	if ((fd = open_io(path)) == -1) {
		ELF_DPRINTF("Can't open %s\n", path);
		return;
	}


	total = 0;
	bootcode = (char*)OLDWORLD_BOOTCODE_BASEADDR;
	while(1) {
		if (seek_io(fd, total) == -1)
			break;
		len = read_io(fd, bootcode, 512);
		bootcode += len;
		total += len;
	}

	close_io( fd );

	if (total == 0) {
		ELF_DPRINTF("Can't read %s\n", path);
		return;
	}

	encode_bootpath(path, "Linux");

	if( ofmem_claim( OLDWORLD_BOOTCODE_BASEADDR, total, 0 ) == -1 )
		fatal_error("Claim failed!\n");

	call_elf(0, 0, OLDWORLD_BOOTCODE_BASEADDR);

        return;
}

static void
newworld_boot( void )
{
        static const char * const chrp_path = "\\\\:tbxi" ;
        char *path = pop_fstr_copy(), *param;

	param = strchr(path, ' ');
	if (param) {
		*param = 0;
		param++;
	}

        if (!path) {
            NEWWORLD_DPRINTF("Entering boot, no path\n");
            push_str("boot-device");
            push_str("/options");
            fword("(find-dev)");
            POP();
            fword("get-package-property");
            if (!POP()) {
                path = pop_fstr_copy();
                param = strchr(path, ' ');
                if (param) {
                    *param = '\0';
                    param++;
                } else {
                    push_str("boot-args");
                    push_str("/options");
                    fword("(find-dev)");
                    POP();
                    fword("get-package-property");
                    POP();
                    param = pop_fstr_copy();
                }
                try_path(path, NULL, param);
	        try_path(path, chrp_path, param);
            } else {
                uint16_t boot_device = fw_cfg_read_i16(FW_CFG_BOOT_DEVICE);
                switch (boot_device) {
                case 'c':
                    path = strdup("hd:");
                    break;
                default:
                case 'd':
                    path = strdup("cd:");
                    break;
                }
	        try_path(path, chrp_path, param);
            }
        } else {
            NEWWORLD_DPRINTF("Entering boot, path %s\n", path);
	    try_path(path, NULL, param);
            try_path(path, chrp_path, param);
        }
	printk("*** Boot failure! No secondary bootloader specified ***\n");
}

static void check_preloaded_kernel(void)
{
    unsigned long kernel_image, kernel_size;
    unsigned long initrd_image, initrd_size;
    const char * kernel_cmdline;

    kernel_size = fw_cfg_read_i32(FW_CFG_KERNEL_SIZE);
    if (kernel_size) {
        kernel_image = fw_cfg_read_i32(FW_CFG_KERNEL_ADDR);
        kernel_cmdline = (const char *) fw_cfg_read_i32(FW_CFG_KERNEL_CMDLINE);
        initrd_image = fw_cfg_read_i32(FW_CFG_INITRD_ADDR);
        initrd_size = fw_cfg_read_i32(FW_CFG_INITRD_SIZE);
        printk("[ppc] Kernel already loaded (0x%8.8lx + 0x%8.8lx) "
               "(initrd 0x%8.8lx + 0x%8.8lx)\n",
               kernel_image, kernel_size, initrd_image, initrd_size);
        if (kernel_cmdline) {
               phandle_t ph;
	       printk("[ppc] Kernel command line: %s\n", kernel_cmdline);
	       ph = find_dev("/chosen");
               set_property(ph, "bootargs", strdup(kernel_cmdline), strlen(kernel_cmdline) + 1);
        }
        call_elf(initrd_image, initrd_size, kernel_image);
    }
}

/************************************************************************/
/*	entry								*/
/************************************************************************/

void
boot( void )
{
        uint16_t boot_device = fw_cfg_read_i16(FW_CFG_BOOT_DEVICE);

	fword("update-chosen");
	if (boot_device == 'm') {
	        check_preloaded_kernel();
	}
	if (boot_device == 'c') {
		oldworld_boot();
	}
	newworld_boot();
}
