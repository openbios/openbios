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
load(const char *path)
{
	char buffer[1024];
	sprintf(buffer, "load %s", path);
	feval(buffer);
}

static void
transfer_control_to_elf( ulong elf_entry )
{
	ELF_DPRINTF("Starting ELF boot loader\n");
        call_elf( 0, 0, elf_entry );

	fatal_error("call_elf returned unexpectedly\n");
}

static int
load_elf_rom( ulong *elf_entry, int fd )
{
	int i, lszz_offs, elf_offs;
        char *addr;
	Elf_ehdr ehdr;
	Elf_phdr *phdr;
	size_t s;

	ELF_DPRINTF("Loading '%s' from '%s'\n", get_file_path(fd),
	       get_volume_name(fd) );

	/* the ELF-image (usually) starts at offset 0x4000 */
	if( (elf_offs=find_elf(fd)) < 0 ) {
                ELF_DPRINTF("----> %s is not an ELF image\n", get_file_path(fd));
		return -1;
	}
	if( !(phdr=elf_readhdrs(fd, elf_offs, &ehdr)) ) {
		ELF_DPRINTF("elf_readhdrs failed\n");
		return -1;
	}

	*elf_entry = ehdr.e_entry;

	/* load segments. Compressed ROM-image assumed to be located immediately
	 * after the last segment */
	lszz_offs = elf_offs;
	for( i=0; i<ehdr.e_phnum; i++ ) {
		/* p_memsz, p_flags */
		s = MIN( phdr[i].p_filesz, phdr[i].p_memsz );
		seek_io( fd, elf_offs + phdr[i].p_offset );

		ELF_DPRINTF("filesz: %08lX memsz: %08lX p_offset: %08lX p_vaddr %08lX\n",
                   (ulong)phdr[i].p_filesz, (ulong)phdr[i].p_memsz, (ulong)phdr[i].p_offset,
                   (ulong)phdr[i].p_vaddr );

		if( phdr[i].p_vaddr != phdr[i].p_paddr )
			ELF_DPRINTF("WARNING: ELF segment virtual addr != physical addr\n");
		lszz_offs = MAX( lszz_offs, elf_offs + phdr[i].p_offset + phdr[i].p_filesz );
		if( !s )
			continue;
		if( ofmem_claim( phdr[i].p_vaddr, phdr[i].p_memsz, 0 ) == -1 )
			fatal_error("Claim failed!\n");

		addr = (char*)phdr[i].p_vaddr;
		if( read_io(fd, addr, s) != s ) {
			ELF_DPRINTF("read failed\n");
			return -1;
		}

		flush_icache_range( addr, addr+s );

		ELF_DPRINTF("ELF ROM-section loaded at %08lX (size %08lX)\n",
			    (ulong)phdr[i].p_vaddr, (ulong)phdr[i].p_memsz );
	}
	free( phdr );
	return lszz_offs;
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
try_path(const char *path, const char *param)
{
    ulong elf_entry;
    int fd, ret;

    ELF_DPRINTF("Trying %s %s\n", path, param);
    if ((fd = open_io(path)) == -1) {
        ELF_DPRINTF("Can't open %s\n", path);
        return;
    }
    ret = load_elf_rom( &elf_entry, fd );
    if (ret < 0)
        return;
    close_io( fd );
    encode_bootpath( path, param );

    update_nvram();
    ELF_DPRINTF("Transfering control to %s %s\n",
                path, param);
    transfer_control_to_elf( elf_entry );
    /* won't come here */
}

static void
try_chrp_script(const char *of_path, const char *param, const char *script_path)
{
    char  path[1024];
    char *device, *filename, *directory;
    int partition;
    int size;

    device = get_device(of_path);
    partition = get_partition(of_path);
    filename = get_filename(of_path, &directory);

    CHRP_DPRINTF("device %s partition %d filename %s\n", device, partition, filename);

    /* read boot script */

    if (partition == -1)
        snprintf(path, sizeof(path), "%s:,%s",
                 device, script_path);
    else
        snprintf(path, sizeof(path), "%s:%d,%s",
                 device, partition, script_path);

    CHRP_DPRINTF("Trying %s\n", path);
    load(path);
    feval("load-size");
    size = POP();
    if (size == 0) {
        ELF_DPRINTF("Can't open %s\n", of_path);
        return;
    }
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
                try_path(path, param);
	        try_chrp_script(path, param, chrp_path);
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
	        try_chrp_script(path, param, chrp_path);
            }
        } else {
            NEWWORLD_DPRINTF("Entering boot, path %s\n", path);
            try_path(path, param);
            try_chrp_script(path, param, chrp_path);
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
