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

//#define DEBUG_ELF

#ifdef DEBUG_ELF
#define ELF_DPRINTF(fmt, args...) \
do { printk("ELF - %s: " fmt, __func__ , ##args); } while (0)
#else
#define ELF_DPRINTF(fmt, args...) do { } while (0)
#endif

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
		return 0;
	path++;

	if (!strchr(path, ','))	/* check if there is a ',' */
		return 0;

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

	filename = get_filename(spec, &directory);
	snprintf(path, sizeof(path), "%s:%d,%s\\%s", get_device(spec),
		 get_partition(spec), directory, filename);

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

/*
  Parse SGML structure like:
  <chrp-boot>
  <description>Debian/GNU Linux Installation on IBM CHRP hardware</description>
  <os-name>Debian/GNU Linux for PowerPC</os-name>
  <boot-script>boot &device;:\install\yaboot</boot-script>
  <icon size=64,64 color-space=3,3,2>

  CHRP system bindings are described at:
  http://playground.sun.com/1275/bindings/chrp/chrp1_7a.ps
*/
static void
try_bootinfo(const char *path, const char *param)
{
    int fd, len, tag, taglen, script, scriptlen, entity;
    char tagbuf[256], bootscript[256], c;
    char *device, *filename, *directory;
    int partition;

    device = get_device(path);
    partition = get_partition(path);
    filename = get_filename(path, &directory);

    /* read boot script */

    snprintf(bootscript, sizeof(bootscript), "%s:%d,ppc\\bootinfo.txt",
             device, partition);

    ELF_DPRINTF("Trying %s\n", bootscript);
    if ((fd = open_io(bootscript)) == -1) {
        ELF_DPRINTF("Can't open %s\n", bootscript);
        return;
    }
    len = read_io(fd, tagbuf, 11);
    tagbuf[11] = '\0';
    if (len < 0 || strcasecmp(tagbuf, "<chrp-boot>") != 0)
        goto badf;

    tag = 0;
    taglen = 0;
    script = 0;
    scriptlen = 0;
    entity = 0;
    do {
        len = read_io(fd, &c, 1);
        if (len < 0)
            goto badf;
        if (c == '<') {
            script = 0;
            tag = 1;
            taglen = 0;
        } else if (c == '>') {
            tag = 0;
            tagbuf[taglen] = '\0';
            if (strcasecmp(tagbuf, "boot-script") == 0)
                script = 1;
            else if (strcasecmp(tagbuf, "/boot-script") == 0) {
                bootscript[scriptlen] = '\0';
                break;
            }
        } else if (tag && taglen < sizeof(tagbuf)) {
            tagbuf[taglen++] = c;
        } else if (script && c == '&') {
            entity = 1;
            taglen = 0;
        } else if (entity && c ==';') {
            entity = 0;
            tagbuf[taglen] = '\0';
            if (strcasecmp(tagbuf, "device") == 0) {
                strcpy(bootscript + scriptlen, device);
                scriptlen += strlen(device);
            } else if (strcasecmp(tagbuf, "partition") == 0) {
		sprintf(bootscript + scriptlen, "%d", partition);
                scriptlen = strlen(bootscript);
            } else if (strcasecmp(tagbuf, "directory") == 0) {
                strcpy(bootscript + scriptlen, directory);
                scriptlen += strlen(directory);
            } else if (strcasecmp(tagbuf, "filename") == 0) {
                strcpy(bootscript + scriptlen, filename);
                scriptlen += strlen(filename);
            } else if (strcasecmp(tagbuf, "full-path") == 0) {
                strcpy(bootscript + scriptlen, path);
                scriptlen += strlen(path);
            } else { /* unknown, keep it */
                bootscript[scriptlen] = '&';
                strcpy(bootscript + scriptlen + 1, tagbuf);
                scriptlen += taglen + 1;
                bootscript[scriptlen] = ';';
                scriptlen++;
            }
        } else if (entity && taglen < sizeof(tagbuf)) {
            tagbuf[taglen++] = c;
        } else if (script && scriptlen < sizeof(bootscript)) {
            bootscript[scriptlen++] = c;
        }
    } while (1);

    ELF_DPRINTF("got bootscript %s\n", bootscript);

    encode_bootpath(path, param);

    feval(bootscript);
 badf:
    close_io( fd );
}

#define QUIK_SECOND_BASEADDR	0x3e0000
#define QUIK_FIRST_BASEADDR	0x3f4000
#define QUIK_SECOND_SIZE	(QUIK_FIRST_BASEADDR - QUIK_SECOND_BASEADDR)
#define QUIK_FIRST_INFO_OFF	0x2c8

struct first_info {
	char		quik_vers[8];
	int		nblocks;
	int		blocksize;
	unsigned	second_base;
	int		conf_part;
	char		conf_file[32];
	unsigned	blknos[64];
};

static void
quik_startup( void )
{
	int fd;
	int len;
	const char *path = "hd:2";
	union {
		char buffer[1024];
		int first_word;
		struct {
			char pad[QUIK_FIRST_INFO_OFF];
			struct first_info fi;
		} fi;
	} u;

	if ((fd = open_io(path)) == -1) {
		ELF_DPRINTF("Can't open %s\n", path);
		return;
	}

	seek_io(fd, 0);
	len = read_io(fd, u.buffer, sizeof(u));
	close_io( fd );

	if (len == -1) {
		ELF_DPRINTF("Can't read %s\n", path);
		return;
	}

	/* is it quik ? */

	if (memcmp(u.fi.fi.quik_vers, "QUIK", 4))
		return;

	encode_bootpath(path, "Linux");

	if( ofmem_claim( QUIK_FIRST_BASEADDR, len, 0 ) == -1 )
		fatal_error("Claim failed!\n");

	memcpy((char*)QUIK_FIRST_BASEADDR, u.buffer, len);

	/* quik fist level doesn't claim second level memory */

	if( ofmem_claim( QUIK_SECOND_BASEADDR, QUIK_SECOND_SIZE, 0 ) == -1 )
		fatal_error("Claim failed!\n");

	call_elf(0, 0, QUIK_FIRST_BASEADDR);

        return;
}

static void
yaboot_startup( void )
{
        static const char * const paths[] = { "hd:2,\\ofclient", "hd:2,\\yaboot" };
        static const char * const args[] = { "", "conf=hd:2,\\yaboot.conf" };
        char *path = pop_fstr_copy(), *param;
        int i;

	param = strchr(path, ' ');
	if (param) {
		*param = 0;
		param++;
	}

        if (!path) {
            ELF_DPRINTF("Entering boot, no path\n");
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
                try_bootinfo(path, param);
            } else {
                uint16_t boot_device = fw_cfg_read_i16(FW_CFG_BOOT_DEVICE);
                switch (boot_device) {
                case 'c':
                    path = strdup("hd:0");
                    break;
                default:
                case 'd':
                    path = strdup("cd:0");
                    break;
                }
                try_bootinfo(path, NULL);
            }
        } else {
            ELF_DPRINTF("Entering boot, path %s\n", path);
            try_path(path, param);
            try_bootinfo(path, param);
        }
        for( i=0; i < sizeof(paths) / sizeof(paths[0]); i++ ) {
            try_path(paths[i], args[i]);
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
		quik_startup();
	}
	yaboot_startup();
}
