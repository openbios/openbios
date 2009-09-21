/*
 *
 *       <bootinfo-loader.c>
 *
 *       bootinfo file loader
 *
 *   Copyright (C) 2009 Laurent Vivier (Laurent@vivier.eu)
 *
 *   Original XML parser by Blue Swirl <blauwirbel@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   version 2
 *
 */

#include "openbios/config.h"
#include "openbios/bindings.h"
#include "modules.h"
#include "ofmem.h"
#include "libc/vsprintf.h"

//#define DEBUG_BOOTINFO

#ifdef DEBUG_BOOTINFO
#define DPRINTF(fmt, args...) \
    do { printk("%s: " fmt, __func__ , ##args); } while (0)
#else
#define DPRINTF(fmt, args...) \
    do { } while (0)
#endif

DECLARE_NODE(bootinfo_loader, INSTALL_OPEN, 0, "+/packages/bootinfo-loader" );

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

	if (!strchr(path, ',')) /* check if there is a ',' */
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
bootinfo_loader_init_program( void *dummy )
{
	char *base;
	int proplen;
	phandle_t chosen;
	int tag, taglen, script, scriptlen, entity, chrp;
	char tagbuf[128], c;
	char *device, *filename, *directory;
	int partition;
	int current, size;
	char *bootscript;
        char *tmp;
	char bootpath[1024];

	feval("0 state-valid !");

	chosen = find_dev("/chosen");
	tmp = get_property(chosen, "bootpath", &proplen);
	memcpy(bootpath, tmp, proplen);
	bootpath[proplen] = 0;

	DPRINTF("bootpath %s\n", bootpath);

	device = get_device(bootpath);
	partition = get_partition(bootpath);
	filename = get_filename(bootpath, &directory);

	feval("load-base");
	base = (char*)POP();

	feval("load-size");
	size = POP();

	bootscript = malloc(size);
	if (bootscript == NULL) {
		DPRINTF("Can't malloc %d bytes\n", size);
		return;
	}

	chrp = 0;
	tag = 0;
	taglen = 0;
	script = 0;
	scriptlen = 0;
	entity = 0;
	current = 0;
	while (current < size) {

		c = base[current++];

		if (c == '<') {
			script = 0;
			tag = 1;
			taglen = 0;
		} else if (c == '>') {
			tag = 0;
			tagbuf[taglen] = '\0';
			if (strcasecmp(tagbuf, "chrp-boot") == 0) {
				chrp = 1;
			} else if (chrp == 1) {
				if (strcasecmp(tagbuf, "boot-script") == 0) {
					script = 1;
					scriptlen = 0;
				} else if (strcasecmp(tagbuf, "/boot-script") == 0) {

					script = 0;
					bootscript[scriptlen] = '\0';

					DPRINTF("got bootscript %s\n",
						bootscript);

					feval("-1 state-valid !");

					break;
				} else if (strcasecmp(tagbuf, "/chrp-boot") == 0)
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
			if (strcasecmp(tagbuf, "lt") == 0) {
				bootscript[scriptlen++] = '<';
			} else if (strcasecmp(tagbuf, "gt") == 0) {
				bootscript[scriptlen++] = '>';
			} else if (strcasecmp(tagbuf, "device") == 0) {
				strcpy(bootscript + scriptlen, device);
				scriptlen += strlen(device);
			} else if (strcasecmp(tagbuf, "partition") == 0) {
				if (partition != -1)
			sprintf(bootscript + scriptlen, "%d", partition);
				else
					*(bootscript + scriptlen) = 0;
				scriptlen = strlen(bootscript);
			} else if (strcasecmp(tagbuf, "directory") == 0) {
				strcpy(bootscript + scriptlen, directory);
				scriptlen += strlen(directory);
			} else if (strcasecmp(tagbuf, "filename") == 0) {
				strcpy(bootscript + scriptlen, filename);
				scriptlen += strlen(filename);
			} else if (strcasecmp(tagbuf, "full-path") == 0) {
				strcpy(bootscript + scriptlen, bootpath);
				scriptlen += strlen(bootpath);
			} else { /* unknown, keep it */
				bootscript[scriptlen] = '&';
				strcpy(bootscript + scriptlen + 1, tagbuf);
				scriptlen += taglen + 1;
				bootscript[scriptlen] = ';';
				scriptlen++;
			}
		} else if (entity && taglen < sizeof(tagbuf)) {
			tagbuf[taglen++] = c;
		} else if (script && scriptlen < size) {
			bootscript[scriptlen++] = c;
		}
	}
	/* FIXME: should initialize saved-program-state. */
	push_str(bootscript);
	feval("bootinfo-size ! bootinfo-entry !");
}

NODE_METHODS( bootinfo_loader ) = {
	{ "init-program", bootinfo_loader_init_program },
};

void bootinfo_loader_init( void )
{
	REGISTER_NODE( bootinfo_loader );
}
