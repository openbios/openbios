/*
 *   derived from mol/mol.c,
 *   Copyright (C) 2003, 2004 Samuel Rydh (samuel@ibrium.se)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   version 2
 *
 */

#include "openbios/config.h"
#include "openbios/nvram.h"
#include "openbios/bindings.h"
#include "libc/byteorder.h"
#include "libc/vsprintf.h"

#include "macio.h"
#include "cuda.h"

#define IO_NVRAM_SIZE   0x00020000
#define IO_NVRAM_OFFSET 0x00060000

static char *nvram;

int
arch_nvram_size( void )
{
	return IO_NVRAM_SIZE>>4;
}

void macio_nvram_init(const char *path, uint32_t addr)
{
	phandle_t chosen, aliases;
	phandle_t dnode;
	int props[2];
	char buf[64];

	nvram = (char*)addr + IO_NVRAM_OFFSET;
        snprintf(buf, sizeof(buf), "%s/nvram", path);
	nvram_init(buf);
	dnode = find_dev(buf);
	set_int_property(dnode, "#bytes", arch_nvram_size() );
	props[0] = __cpu_to_be32(IO_NVRAM_OFFSET);
	props[1] = __cpu_to_be32(IO_NVRAM_SIZE);
	set_property(dnode, "reg", (char *)&props, sizeof(props));
	set_property(dnode, "device_type", "nvram", 6);

	chosen = find_dev("/chosen");
	push_str(buf);
	fword("open-dev");
	set_int_property(chosen, "nvram", POP());

	aliases = find_dev("/aliases");
	set_property(aliases, "nvram", buf, strlen(buf) + 1);
}

#ifdef DUMP_NVRAM
static void
dump_nvram(void)
{
  int i, j;
  for (i = 0; i < 10; i++)
    {
      for (j = 0; j < 16; j++)
      printk ("%02x ", nvram[(i*16+j)<<4]);
      printk (" ");
      for (j = 0; j < 16; j++)
        if (isprint(nvram[(i*16+j)<<4]))
            printk("%c", nvram[(i*16+j)<<4]);
        else
          printk(".");
      printk ("\n");
      }
}
#endif


void
arch_nvram_put( char *buf )
{
	int i;
	for (i=0; i< arch_nvram_size() ; i++)
		nvram[i<<4]=buf[i];
#ifdef DUMP_NVRAM
	printk("new nvram:\n");
	dump_nvram();
#endif
}

void
arch_nvram_get( char *buf )
{
	int i;
	for (i=0; i< arch_nvram_size(); i++)
		buf[i]=nvram[i<<4];

#ifdef DUMP_NVRAM
	printk("current nvram:\n");
	dump_nvram();
#endif
}

DECLARE_UNNAMED_NODE( ob_intctrl_node, INSTALL_OPEN, 2*sizeof(int) );

static void
ob_intctrl_open(int *idx)
{
        int ret=1;
        RET ( -ret );
}

static void
ob_intctrl_close(int *idx)
{
}

static void
ob_intctrl_initialize(int *idx)
{
}

NODE_METHODS(ob_intctrl_node) = {
        { NULL,                 ob_intctrl_initialize       },
        { "open",               ob_intctrl_open             },
        { "close",              ob_intctrl_close            },
};

phandle_t pic_handle;

void
ob_macio_init(const char *path, uint32_t addr)
{
        phandle_t aliases;

	aliases = find_dev("/aliases");
	set_property(aliases, "mac-io", path, strlen(path) + 1);

	cuda_init(path, addr);
	macio_nvram_init(path, addr);
}
