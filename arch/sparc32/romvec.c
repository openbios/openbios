/*
 * PROM interface support
 * Copyright 1996 The Australian National University.
 * Copyright 1996 Fujitsu Laboratories Limited
 * Copyright 1999 Pete A. Zaitcev
 * This software may be distributed under the terms of the Gnu
 * Public License version 2 or later
 */

#include "openprom.h"
#include "stdint.h"
#include "asm/io.h"
#include "asm/types.h"
#include "libc/vsprintf.h"
#include "openbios/config.h"
#include "openbios/bindings.h"
#include "openbios/kernel.h"
#include "openbios/sysinclude.h"

#ifdef CONFIG_DEBUG_OBP
#define DPRINTF(fmt, args...)                   \
    do { printk(fmt , ##args); } while (0)
#else
#define DPRINTF(fmt, args...)
#endif

#define PAGE_SIZE 4096

struct ob_property {
    struct ob_property *next;
    char *name;
    char *value;
    int length;
};

struct ob_node {
    struct ob_node *next;
    struct ob_node *peer;
    struct ob_node *child;
    struct ob_property *prop;
    int id;
};

// Workarounds:

// Proll device tree happens to work, why?
#define PROLL_TREE

// Build a static device tree to avoid Forth access from deep kernel
//#define STATIC_TREE

#ifdef PROLL_TREE
struct property {
	const char *name;
	const char *value;
	int length;
};

struct node {
	const struct property *properties;
	/* short */ const int sibling;
	/* short */ const int child;
};

static const unsigned char obp_idprom[32] = { 1, 0x80, 0x52, 0x54, 0x00, 0x12, 0x34, 0x56,
                                              0,0,0,0, 0,0,0, 0xf7 };
static const struct property null_properties = { NULL, NULL, -1 };
static const int prop_true = -1;

static const struct property propv_root[] = {
	{"name",	"SUNW,SparcStation-5", sizeof("SUNW,SparcStation-5") },
	{"idprom",	(char *)obp_idprom, 32},
	{"banner-name", "SparcStation", sizeof("SparcStation")},
	{"compatible",	"sun4m", 6},
	{NULL, NULL, -1}
};

static const int prop_iommu_reg[] = {
	0x0, 0x10000000, 0x00000300,
};
static const struct property propv_iommu[] = {
	{"name",	"iommu", sizeof("iommu")},
	{"reg",		(char*)&prop_iommu_reg[0], sizeof(prop_iommu_reg) },
	{NULL, NULL, -1}
};

static const int prop_sbus_ranges[] = {
	0x0, 0x0, 0x0, 0x30000000, 0x10000000,
	0x1, 0x0, 0x0, 0x40000000, 0x10000000,
	0x2, 0x0, 0x0, 0x50000000, 0x10000000,
	0x3, 0x0, 0x0, 0x60000000, 0x10000000,
	0x4, 0x0, 0x0, 0x70000000, 0x10000000,
};
static const struct property propv_sbus[] = {
	{"name",	"sbus", 5},
	{"ranges",	(char*)&prop_sbus_ranges[0], sizeof(prop_sbus_ranges)},
	{"device_type",	"hierarchical", sizeof("hierarchical") },
	{NULL, NULL, -1}
};

static const int prop_tcx_regs[] = {
	0x2, 0x00800000, 0x00100000,
	0x2, 0x02000000, 0x00000001,
	0x2, 0x04000000, 0x00800000,
	0x2, 0x06000000, 0x00800000,
	0x2, 0x0a000000, 0x00000001,
	0x2, 0x0c000000, 0x00000001,
	0x2, 0x0e000000, 0x00000001,
	0x2, 0x00700000, 0x00001000,
	0x2, 0x00200000, 0x00000004,
	0x2, 0x00300000, 0x0000081c,
	0x2, 0x00000000, 0x00010000,
	0x2, 0x00240000, 0x00000004,
	0x2, 0x00280000, 0x00000001,
};

#if 1	/* Zaitcev */
static const int pixfreq = 0x03dfd240;
static const int hbporch = 0xa0;
static const int vfreq = 0x3c;
#endif
#if 0	/* Kevin Boone - 70Hz refresh */
static const int pixfreq = 0x047868C0;
static const int hbporch = 0x90;
static const int vfreq = 0x46;
#endif

static const int vbporch = 0x1d;
static const int vsync = 0x6;
static const int hsync = 0x88;
static const int vfporch = 0x3;
static const int hfporch = 0x18;
static const int height = 0x300;
static const int width = 0x400;
static const int linebytes = 0x400;
static const int depth = 24;
static const int tcx_intr[] = { 5, 0 };
static const int tcx_interrupts = 5;
static const struct property propv_sbus_tcx[] = {
	{"name",	"SUNW,tcx", sizeof("SUNW,tcx")},
	{"vbporch",	(char*)&vbporch, sizeof(int)},
	{"hbporch",	(char*)&hbporch, sizeof(int)},
	{"vsync",	(char*)&vsync, sizeof(int)},
	{"hsync",	(char*)&hsync, sizeof(int)},
	{"vfporch",	(char*)&vfporch, sizeof(int)},
	{"hfporch",	(char*)&hfporch, sizeof(int)},
	{"pixfreq",	(char*)&pixfreq, sizeof(int)},
	{"vfreq",	(char*)&vfreq, sizeof(int)},
	{"height",	(char*)&height, sizeof(int)},
	{"width",	(char*)&width, sizeof(int)},
	{"linebytes",	(char*)&linebytes, sizeof(int)},
	{"depth",	(char*)&depth, sizeof(int)},
	{"reg",		(char*)&prop_tcx_regs[0], sizeof(prop_tcx_regs)},
	{"tcx-8-bit",	0, -1},
	{"intr",	(char*)&tcx_intr[0], sizeof(tcx_intr)},
	{"interrupts",	(char*)&tcx_interrupts, sizeof(tcx_interrupts)},
	{"device_type",	"display", sizeof("display")},
	{NULL, NULL, -1}
};

static const int prop_cs4231_reg[] = {
	0x3, 0x0C000000, 0x00000040
};
static const int cs4231_interrupts = 5;
static const int cs4231_intr[] = { 5, 0 };

static const struct property propv_sbus_cs4231[] = {
	{"name",	"SUNW,CS4231", sizeof("SUNW,CS4231") },
	{"intr",	(char*)&cs4231_intr[0], sizeof(cs4231_intr) },
	{"interrupts",  (char*)&cs4231_interrupts, sizeof(cs4231_interrupts) },	
	{"reg",		(char*)&prop_cs4231_reg[0], sizeof(prop_cs4231_reg) },
	{"device_type", "serial", sizeof("serial") },
	{"alias",	"audio", sizeof("audio") },
	{NULL, NULL, -1}
};

static const int cpu_nctx = 256;
static const int cpu_cache_line_size = 0x20;
static const int cpu_cache_nlines = 0x200;
static const struct property propv_cpu[] = {
	{"name",	"STP1012PGA", sizeof("STP1012PGA") },
	{"device_type",	"cpu", 4 },
	{"mmu-nctx",	(char*)&cpu_nctx, sizeof(int)},
	{"cache-line-size",	(char*)&cpu_cache_line_size, sizeof(int)},
	{"cache-nlines",	(char*)&cpu_cache_nlines, sizeof(int)},
	{NULL, NULL, -1}
};

static const int prop_obio_ranges[] = {
	0x0, 0x0, 0x0, 0x71000000, 0x01000000,
};
static const struct property propv_obio[] = {
	{"name",	"obio", 5 },
	{"ranges",	(char*)&prop_obio_ranges[0], sizeof(prop_obio_ranges) },
	{"device_type",	"hierarchical", sizeof("hierarchical") },
	{NULL, NULL, -1}
};

static const int prop_auxio_reg[] = {
	0x0, 0x00900000, 0x00000001,
};
static const struct property propv_obio_auxio[] = {
	{"name",	"auxio", sizeof("auxio") },
	{"reg",		(char*)&prop_auxio_reg[0], sizeof(prop_auxio_reg) },
	{NULL, NULL, -1}
};

static const int prop_int_reg[] = {
	0x0, 0x00e00000, 0x00000010,
	0x0, 0x00e10000, 0x00000010,
};
static const struct property propv_obio_int[] = {
	{"name",	"interrupt", sizeof("interrupt")},
	{"reg",		(char*)&prop_int_reg[0], sizeof(prop_int_reg) },
	{NULL, NULL, -1}
};

static const int prop_cnt_reg[] = {
	0x0, 0x00d00000, 0x00000010,
	0x0, 0x00d10000, 0x00000010,
};
static const struct property propv_obio_cnt[] = {
	{"name",	"counter", sizeof("counter")},
	{"reg",		(char*)&prop_cnt_reg[0], sizeof(prop_cnt_reg) },
	{NULL, NULL, -1}
};

static const int prop_eeprom_reg[] = {
	0x0, 0x00200000, 0x00002000,
};
static const struct property propv_obio_eep[] = {
	{"name",	"eeprom", sizeof("eeprom")},
	{"reg",		(char*)&prop_eeprom_reg[0], sizeof(prop_eeprom_reg) },
	{"model",	"mk48t08", sizeof("mk48t08")},
	{NULL, NULL, -1}
};

static const int prop_su_reg[] = {
	0x0, 0x003002f8, 0x00000008,
};
static const struct property propv_obio_su[] = {
	{"name",	"su", sizeof("su")},
	{"reg",		(char*)&prop_su_reg[0], sizeof(prop_su_reg) },
	{NULL, NULL, -1}
};

static const int prop_zs_intr[] = { 0x2c, 0x0 };
static const int prop_zs_reg[] = {
	0x0, 0x00000000, 0x00000008,
};
static void *prop_zs_addr;
static const int prop_zs_slave = 1;
static const struct property propv_obio_zs[] = {
	{"name",	"zs", sizeof("zs")},
	{"reg",		(char*)&prop_zs_reg[0], sizeof(prop_zs_reg) },
	{"slave",	(char*)&prop_zs_slave, sizeof(prop_zs_slave) },
	{"device_type", "serial", sizeof("serial") },
	{"intr",	(char*)&prop_zs_intr[0], sizeof(prop_zs_intr) },
	{"address",	(char*)&prop_zs_addr, sizeof(prop_zs_addr) },
	{"keyboard",	(char*)&prop_true, 0},
	{"mouse",	(char*)&prop_true, 0},
	{NULL, NULL, -1}
};

static const int prop_zs1_intr[] = { 0x2c, 0x0 };
static const int prop_zs1_reg[] = {
	0x0, 0x00100000, 0x00000008,
};
static void *prop_zs1_addr;
static const int prop_zs1_slave = 0;
static const struct property propv_obio_zs1[] = {
	{"name",	"zs", sizeof("zs")},
	{"reg",		(char*)&prop_zs1_reg[0], sizeof(prop_zs1_reg) },
	{"slave",	(char*)&prop_zs1_slave, sizeof(prop_zs1_slave) },
	{"device_type", "serial", sizeof("serial") },
	{"intr",	(char*)&prop_zs1_intr[0], sizeof(prop_zs1_intr) },
	{"address",	(char*)&prop_zs1_addr, sizeof(prop_zs1_addr) },
	{NULL, NULL, -1}
};

static const int prop_ledma_reg[] = {
	0x4, 0x08400010, 0x00000020,
};
static const int prop_ledma_burst = 0x3f;
static const struct property propv_sbus_ledma[] = {
	{"name",	"ledma", sizeof("ledma")},
	{"reg",		(char*)&prop_ledma_reg[0], sizeof(prop_ledma_reg) },
	{"burst-sizes",	(char*)&prop_ledma_burst, sizeof(int) },
	{NULL, NULL, -1}
};

static const int prop_le_reg[] = {
	0x4, 0x08c00000, 0x00000004,
};
static const int prop_le_busmaster_regval = 0x7;
static const int prop_le_intr[] = { 0x26, 0x0 };
static const struct property propv_sbus_ledma_le[] = {
	{"name",	"le", sizeof("le")},
	{"reg",		(char*)&prop_le_reg[0], sizeof(prop_le_reg) },
	{"busmaster-regval",	(char*)&prop_le_busmaster_regval, sizeof(int)},
	{"intr",	(char*)&prop_le_intr[0], sizeof(prop_le_intr) },
	{NULL, NULL, -1}
};

static const int prop_espdma_reg[] = {
	0x4, 0x08400000, 0x00000010,
};

static const struct property propv_sbus_espdma[] = {
	{"name",	"espdma", sizeof("espdma")}, 
	{"reg",		(char*)&prop_espdma_reg[0], sizeof(prop_espdma_reg) },
	{NULL, NULL, -1}
};

static const int prop_esp_reg[] = {
	0x4, 0x08800000, 0x00000040,
};
static const int prop_esp_intr[] = { 0x24, 0x0 };
static const struct property propv_sbus_espdma_esp[] = {
	{"name",	"esp", sizeof("esp")},
	{"reg",		(char*)&prop_esp_reg[0], sizeof(prop_esp_reg) },
	{"intr",	(char*)&prop_esp_intr[0], sizeof(prop_esp_intr) },
	{NULL, NULL, -1}
};

static const int prop_bpp_reg[] = {
	0x4, 0x0c800000, 0x0000001c,
};
static const int prop_bpp_intr[] = { 0x33, 0x0 };
static const struct property propv_sbus_bpp[] = {
	{"name",	"SUNW,bpp", sizeof("SUNW,bpp")},
	{"reg",		(char*)&prop_bpp_reg[0], sizeof(prop_bpp_reg) },
	{"intr",	(char*)&prop_bpp_intr[0], sizeof(prop_bpp_intr) },
	{NULL, NULL, -1}
};

static const int prop_apc_reg[] = {
	0x4, 0x0a000000, 0x00000010,
};
static const struct property propv_sbus_apc[] = {
	{"name",	"xxxpower-management", sizeof("xxxpower-management")},
	{"reg",		(char*)&prop_apc_reg[0], sizeof(prop_apc_reg) },
	{NULL, NULL, -1}
};

static const int prop_fd_intr[] = { 0x2b, 0x0 };
static const int prop_fd_reg[] = {
	0x0, 0x00400000, 0x0000000f,
};
static const struct property propv_obio_fd[] = {
	{"name",	"SUNW,fdtwo", sizeof("SUNW,fdtwo")},
	{"reg",		(char*)&prop_fd_reg[0], sizeof(prop_fd_reg) },
	{"device_type", "block", sizeof("block") },
	{"intr",	(char*)&prop_fd_intr[0], sizeof(prop_fd_intr) },
	{NULL, NULL, -1}
};

static const int prop_pw_intr[] = { 0x22, 0x0 };
static const int prop_pw_reg[] = {
	0x0, 0x00910000, 0x00000001,
};
static const struct property propv_obio_pw[] = {
	{"name",	"power", sizeof("power")},
	{"reg",		(char*)&prop_pw_reg[0], sizeof(prop_pw_reg) },
	{"intr",	(char*)&prop_pw_intr[0], sizeof(prop_pw_intr) },
	{NULL, NULL, -1}
};

static const int prop_cf_reg[] = {
	0x0, 0x00800000, 0x00000001,
};
static const struct property propv_obio_cf[] = {
	{"name",	"slavioconfig", sizeof("slavioconfig")},
	{"reg",		(char*)&prop_cf_reg[0], sizeof(prop_cf_reg) },
	{NULL, NULL, -1}
};

static const struct property propv_options[] = {
	{"name",	"options", sizeof("options")},
	{"screen-#columns",	"80", sizeof("80")},
	{"screen-#rows",	"25", sizeof("25")},
	{"tpe-link-test?",	(char *)&prop_true, 0},
	{"ttya-mode",		"9600,8,n,1,-", sizeof("9600,8,n,1,-")},
	{"ttya-ignore-cd",	(char *)&prop_true, 0},
	{"ttya-rts-dtr-off",	0, -1},
	{"ttyb-mode",		"9600,8,n,1,-", sizeof("9600,8,n,1,-")},
	{"ttyb-ignore-cd",	(char *)&prop_true, 0},
	{"ttyb-rts-dtr-off",	0, -1},
	{NULL, NULL, -1}
};

static int prop_mem_reg[3];
static int prop_mem_avail[3];

static const struct property propv_memory[] = {
	{"name",	"memory", sizeof("memory")},
	{"reg",		(char*)&prop_mem_reg[0], sizeof(prop_mem_reg) },
	{"available",	(char*)&prop_mem_avail[0], sizeof(prop_mem_avail) },
	{NULL, NULL, -1}
};

static int prop_vmem_avail[6];

static const struct property propv_vmemory[] = {
	{"name",	"virtual-memory", sizeof("virtual-memory")},
	{"available",	(char*)&prop_vmem_avail[0], sizeof(prop_vmem_avail) },
	{NULL, NULL, -1}
};

static const struct node nodes[] = {
	{ &null_properties,	 1,  0 }, /* 0 = big brother of root */
	{ propv_root,		 0,  2 }, /*  1 "/" */
	{ propv_iommu,		12,  3 }, /*  2 "/iommu" */
	{ propv_sbus,		 0,  4 }, /*  3 "/iommu/sbus" */
	{ propv_sbus_tcx,	 5,  0 }, /*  4 "/iommu/sbus/SUNW,tcx" */
	{ propv_sbus_ledma,	 7,  6 }, /*  5 "/iommu/sbus/ledma" */
	{ propv_sbus_ledma_le,	 0,  0 }, /*  6 "/iommu/sbus/ledma/le" */
	{ propv_sbus_cs4231,	 8,  0 }, /*  7 "/iommu/sbus/SUNW,CS4231 */
	{ propv_sbus_bpp,	 9,  0 }, /*  8 "/iommu/sbus/SUNW,bpp */
	{ propv_sbus_espdma,	11, 10 }, /*  9 "/iommu/sbus/espdma" */
	{ propv_sbus_espdma_esp, 0,  0 }, /* 10 "/iommu/sbus/espdma/esp" */
	{ propv_sbus_apc,	 0,  0 }, /* 11 "/iommu/sbus/power-management */
	{ propv_cpu,		13,  0 }, /* 12 "/STP1012PGA" */
	{ propv_obio,		23, 14 }, /* 13 "/obio" */
	{ propv_obio_int,	15,  0 }, /* 14 "/obio/interrupt" */
	{ propv_obio_cnt,	16,  0 }, /* 15 "/obio/counter" */
	{ propv_obio_eep,	17,  0 }, /* 16 "/obio/eeprom" */
	{ propv_obio_auxio,	18,  0 }, /* 17 "/obio/auxio" */
	{ propv_obio_zs1,	19,  0 }, /* 18 "/obio/zs@0,100000"
					     Must be before zs@0,0! */
	{ propv_obio_zs,	20,  0 }, /* 19 "/obio/zs@0,0" */
	{ propv_obio_fd,	21,  0 }, /* 20 "/obio/SUNW,fdtwo" */
	{ propv_obio_pw,	22,  0 }, /* 21 "/obio/power" */
	{ propv_obio_cf,	 0,  0 }, /* 22 "/obio/slavioconfig@0,800000" */
	{ propv_options,	24,  0 }, /* 23 "/options" */
	{ propv_memory,		25,  0 }, /* 24 "/memory" */
	{ propv_vmemory,	 0,  0 }, /* 25 "/virtual-memory" */
};
#endif

static struct linux_mlist_v0 totphys[1];
static struct linux_mlist_v0 totmap[1];
static struct linux_mlist_v0 totavail[1];

static struct linux_mlist_v0 *ptphys;
static struct linux_mlist_v0 *ptmap;
static struct linux_mlist_v0 *ptavail;
static char obp_stdin, obp_stdout;
static int obp_fd_stdin, obp_fd_stdout;

static int obp_nextnode(int node);
static int obp_child(int node);
static int obp_proplen(int node, char *name);
static int obp_getprop(int node, char *name, char *val);
static int obp_setprop(int node, char *name, char *val, int len);
static const char *obp_nextprop(int node, char *name);
static int obp_devread(int dev_desc, char *buf, int nbytes);
static int obp_devseek(int dev_desc, int hi, int lo);

static struct linux_arguments_v0 obp_arg;
static const struct linux_arguments_v0 * const obp_argp = &obp_arg;

static void (*synch_hook)(void);

static struct linux_romvec romvec0;

static void doublewalk(__attribute__((unused)) unsigned int ptab1,
                       __attribute__((unused)) unsigned int va)
{
}

#ifdef PROLL_TREE
static const struct property *find_property(int node,char *name)
{
	const struct property *prop = &nodes[node].properties[0];
	while (prop && prop->name) {
		if (strcmp(prop->name, name) == 0)
                    return prop;
		prop++;
	}
	return NULL;
}

static int obp_nextnode(int node)
{
        DPRINTF("obp_nextnode(%d) = %d\n", node, nodes[node].sibling);
	return nodes[node].sibling;
}

static int obp_child(int node)
{
        DPRINTF("obp_child(%d) = %d\n", node, nodes[node].child);
	return nodes[node].child;
}

static int obp_proplen(int node, char *name)
{
	const struct property *prop = find_property(node,name);
	if (prop) {
	    DPRINTF("obp_proplen(%d, %s) = %d\n", node, name, prop->length);
	    return prop->length;
	}
	DPRINTF("obp_proplen(%d, %s) (no prop)\n", node, name);
	return -1;
}

static int obp_getprop(int node, char *name, char *value)
{
	const struct property *prop;

	if (!name || *name == '\0') {
	    // NULL name means get first property
	    DPRINTF("obp_getprop(%d, (NULL)) = %s\n", node,
		   nodes[node].properties[0].name);
	    return (int)nodes[node].properties[0].name;
	}
	prop = find_property(node,name);
	if (prop) {
		memcpy(value,prop->value,prop->length);
		DPRINTF("obp_getprop(%d, %s) = %s\n", node, name, value);
		return prop->length;
	}
        DPRINTF("obp_getprop(%d, %s): not found\n", node, name);
	return -1;
}

static const char *obp_nextprop(int node,char *name)
{
	const struct property *prop;
	
	if (!name || *name == '\0') {
	    // NULL name means get first property
	    DPRINTF("obp_nextprop(%d, NULL) = %s\n", node,
		   nodes[node].properties[0].name);
	    return nodes[node].properties[0].name;
	}
	prop = find_property(node,name);
	if (prop && prop[1].name) {
	    DPRINTF("obp_nextprop(%d, %s) = %s\n", node, name, prop[1].name);
	    return prop[1].name;
	}
        DPRINTF("obp_nextprop(%d, %s): not found\n", node, name);
	return "";
}

#else
static int obp_nextnode(int node)
{
    int peer;

    PUSH(node);
    fword("peer");
    peer = POP();
    DPRINTF("obp_nextnode(0x%x) = 0x%x\n", node, peer);

    return peer;
}

static int obp_child(int node)
{
    int child;

    PUSH(node);
    fword("child");
    child = POP();
    DPRINTF("obp_child(0x%x) = 0x%x\n", node, child);

    return child;
}

static int obp_proplen(int node, char *name)
{
    int notfound;

    push_str(name);
    PUSH(node);
    fword("get-package-property");
    notfound = POP();

    if (notfound) {
        DPRINTF("obp_proplen(0x%x, %s) (not found)\n", node, name);

        return -1;
    } else {
        int len;

        len = POP();
        (void) POP();
        DPRINTF("obp_proplen(0x%x, %s) = %d\n", node, name, len);

        return len;
    }
}

static int obp_getprop(int node, char *name, char *value)
{
    int notfound, found;
    int len;
    char *str;

    if (!name) {
        // NULL name means get first property
        push_str("");
        PUSH(node);
        fword("next-property");
        found = POP();
        if (found) {
            len = POP();
            str = (char *) POP();
            DPRINTF("obp_getprop(0x%x, NULL) = %s\n", node, str);

            return (int)str;
        }
        DPRINTF("obp_getprop(0x%x, NULL) (not found)\n", node);

        return -1;
    } else {
        push_str(name);
        PUSH(node);
        fword("get-package-property");
        notfound = POP();
    }
    if (notfound) {
        DPRINTF("obp_getprop(0x%x, %s) (not found)\n", node, name);

        return -1;
    } else {
        len = POP();
        str = (char *) POP();
        if (len > 0)
            memcpy(value, str, len);
        else
            str = "NULL";

        DPRINTF("obp_getprop(0x%x, %s) = %s\n", node, name, str);

        return len;
    }
}

static const char *obp_nextprop(int node, char *name)
{
    int found;

    if (!name || *name == '\0') {
        // NULL name means get first property
        push_str("");
        name = "NULL";
    } else {
        push_str(name);
    }
    PUSH(node);
    fword("next-property");
    found = POP();
    if (!found) {
        DPRINTF("obp_nextprop(0x%x, %s) (not found)\n", node, name);

        return "";
    } else {
        int len;
        char *str;
            
        len = POP();
        str = (char *) POP();

        DPRINTF("obp_nextprop(0x%x, %s) = %s\n", node, name, str);

        return str;
    }
}
#endif

#ifdef STATIC_TREE
static void
get_props(struct ob_node *node)
{
    struct ob_property *currprop;
    const char *name;

    name = obp_nextprop(node->id, NULL);
    if (!name || (int)name == -1) {
        node->prop = NULL;
        return;
    }
    node->prop = malloc(sizeof(*node->prop));
    currprop = node->prop;
    for (;;) {
        currprop->length = obp_proplen(node->id, (char *)name);
        currprop->name = malloc(strlen(name) + 1);
        strcpy(currprop->name, name);
        if (currprop->length > 0) {
            currprop->value = malloc(currprop->length);
            obp_getprop(node->id, currprop->name, currprop->value);
        }

        name = obp_nextprop(node->id, currprop->name);
        if (!name || *name == '\0') {
            currprop->next = NULL;
            return;
        } else {
            currprop->next = malloc(sizeof(*currprop->next));
            currprop = currprop->next;
        }
    }
}

static struct ob_node *
get_node(struct ob_node *tree)
{
    struct ob_node *chain = tree;
    int node;

    if (tree->id)
        get_props(tree);

    DPRINTF("visiting 0x%x\n", tree->id);
    node = obp_child(tree->id);
    if (node) {
        DPRINTF("visiting 0x%x's child 0x%x\n", tree->id, node);
        tree->child = malloc(sizeof(*tree->child));
        tree->child->id = node;
        chain->next = tree->child;
        chain = get_node(tree->child);
    } else {
        tree->child = NULL;
    }
    node = obp_nextnode(tree->id);
    if (node) {
        DPRINTF("visiting 0x%x's peer 0x%x\n", tree->id, node);
        tree->peer = malloc(sizeof(*tree->peer));
        tree->peer->id = node;
        chain->next = tree->peer;
        chain = get_node(tree->peer);
    } else {
        tree->peer = NULL;
    }
    return chain;
}

static struct ob_node *tree;

static void
get_tree(void)
{
    tree = malloc(sizeof(*tree));
    tree->id = 0;
    (void)get_node(tree);
    DPRINTF("Static tree finished.\n");
}

static struct ob_node *find_node(int node)
{
    struct ob_node *onode;

    for (onode = tree; onode != NULL && onode->id != node; onode = onode->next);

    return onode;
}

static struct ob_property *find_static_property(int node, char *name)
{
    struct ob_node *onode = find_node(node);
    struct ob_property *prop = onode->prop;

    while (prop && prop->name) {
        if (strcmp(prop->name, name) == 0)
            return prop;
        prop = prop->next;
    }

    return NULL;
}

static int obp_static_nextnode(int node)
{
    int nextnode = -1;
    struct ob_node *onode = find_node(node);

    if (onode && onode->peer)
        nextnode = onode->peer->id;

    DPRINTF("obp_static_nextnode(%d) = %d\n", node, nextnode);

    return nextnode;
}

static int obp_static_child(int node)
{
    int child = -1;
    struct ob_node *onode = find_node(node);

    if (onode && onode->child)
        child = onode->child->id;

    DPRINTF("obp_static_child(%d) = %d\n", node, child);

    return child;
}

static int obp_static_proplen(int node, char *name)
{
    struct ob_property *prop = find_static_property(node, name);

    if (prop) {
        DPRINTF("obp_static_proplen(%d, %s) = %d\n", node, name, prop->length);

        return prop->length;
    }

    DPRINTF("obp_static_proplen(%d, %s) (no prop)\n", node, name);

    return -1;
}

static int obp_static_getprop(int node, char *name, char *value)
{
    struct ob_node *currnode;
    struct ob_property *prop;

    if (!name || *name == '\0') {
        // NULL name means get first property
        currnode = find_node(node);
        if (!currnode->prop) {
            DPRINTF("obp_static_getprop(%d, (NULL)): not found\n", node);
            return -1;
        }
        name = "NULL";
        prop = currnode->prop;
    } else {
        prop = find_static_property(node, name);
    }
    if (prop && prop->length > 0) {
        memcpy(value, prop->value, prop->length);
        DPRINTF("obp_static_getprop(%d, %s) = %s\n", node, name, value);

        return prop->length;
    }

    DPRINTF("obp_static_getprop(%d, %s): not found\n", node, name);

    return -1;
}

static const char *obp_static_nextprop(int node, char *name)
{
    struct ob_property *prop;
    struct ob_node *currnode = find_node(node);

    if (!name || *name == '\0') {
        // NULL name means get first property
        if (!currnode->prop) {
            DPRINTF("obp_static_nextprop(%d, (NULL)): not found\n", node);
            return "";
        }
        prop = currnode->prop;
    } else {
	prop = find_static_property(node, name);
        prop = prop->next;
    }
    if (prop && prop->name) {
        DPRINTF("obp_static_nextprop(%d, %s) = %s\n", node, name, prop->name);
        return prop->name;
    }
    DPRINTF("obp_static_nextprop(%d, %s): not found\n", node, name);
    return "";
}
#define obp_nextnode obp_static_nextnode
#define obp_child obp_static_child
#define obp_proplen obp_static_proplen
#define obp_getprop obp_static_getprop
#define obp_nextprop obp_static_nextprop
#endif

static int obp_setprop(__attribute__((unused)) int node,
		       __attribute__((unused)) char *name,
		       __attribute__((unused)) char *value,
		       __attribute__((unused)) int len)
{
    DPRINTF("obp_setprop(0x%x, %s) = %s (%d)\n", node, name, value, len);

    return -1;
}

static const struct linux_nodeops nodeops0 = {
    obp_nextnode,	/* int (*no_nextnode)(int node); */
    obp_child,	        /* int (*no_child)(int node); */
    obp_proplen,	/* int (*no_proplen)(int node, char *name); */
    obp_getprop,	/* int (*no_getprop)(int node,char *name,char *val); */
    obp_setprop,	/* int (*no_setprop)(int node, char *name,
                           char *val, int len); */
    obp_nextprop	/* char * (*no_nextprop)(int node, char *name); */
};

static int obp_nbgetchar(void)
{
    return getchar();
}

static int obp_nbputchar(int ch)
{
    putchar(ch);

    return 0;
}

static void obp_reboot(char *str)
{
    printk("rebooting (%s)\n", str);
    outb(1, 0x71f00000);
    for (;;) {}
}

static void obp_abort(void)
{
    printk("abort, power off\n");
    outb(1, 0x71910000);
    for (;;) {}
}

static void obp_halt(void)
{
    printk("halt, power off\n");
    outb(1, 0x71910000);
    for (;;) {}
}

static int obp_devopen(char *str)
{
    int ret;

    push_str(str);
    fword("open-dev");
    ret = POP();
    DPRINTF("obp_devopen(%s) = 0x%x\n", str, ret);

    return ret;
}

static int obp_devclose(int dev_desc)
{
    int ret;

    PUSH(dev_desc);
    fword("close-dev");
    ret = POP();

    DPRINTF("obp_devclose(0x%x) = %d\n", dev_desc, ret);

    return ret;
}

static int obp_rdblkdev(int dev_desc, int num_blks, int offset, char *buf)
{
    int ret, hi, lo, bs;

    bs = 512; // XXX: real blocksize?
    hi = ((uint64_t)offset * bs) >> 32;
    lo = ((uint64_t)offset * bs) & 0xffffffff;

    ret = obp_devseek(dev_desc, hi, lo);

    ret = obp_devread(dev_desc, buf, num_blks * bs) / bs;

    DPRINTF("obp_rdblkdev(fd 0x%x, num_blks %d, offset %d (hi %d lo %d), buf 0x%x) = %d\n", dev_desc, num_blks, offset, hi, lo, (int)buf, ret);

    return ret;
}

static char *obp_dumb_mmap(char *va, __attribute__((unused)) int which_io,
			   unsigned int pa, unsigned int size)
{
    unsigned int npages;
    unsigned int off;
    unsigned int mva;

    DPRINTF("obp_dumb_mmap: virta 0x%x, which_io %d, paddr 0x%x, sz %d\n", (unsigned int)va, which_io, pa, size);
    off = pa & (PAGE_SIZE-1);
    npages = (off + size + (PAGE_SIZE-1)) / PAGE_SIZE;
    pa &= ~(PAGE_SIZE-1);

    mva = (unsigned int) va;
    while (npages-- != 0) {
        map_page(mva, pa, 1);
        mva += PAGE_SIZE;
        pa += PAGE_SIZE;
    }

    return va;
}

static void obp_dumb_munmap(__attribute__((unused)) char *va,
			    __attribute__((unused)) unsigned int size)
{
    DPRINTF("obp_dumb_munmap: virta 0x%x, sz %d\n", (unsigned int)va, size);
}

static int obp_devread(int dev_desc, char *buf, int nbytes)
{
    int ret;

    PUSH((int)buf);
    PUSH(nbytes);
    push_str("read");
    PUSH(dev_desc);
    fword("$call-method");
    ret = POP();

    DPRINTF("obp_devread(fd 0x%x, buf 0x%x, nbytes %d) = %d\n", dev_desc, (int)buf, nbytes, ret);

    return ret;
}

static int obp_devwrite(int dev_desc, char *buf, int nbytes)
{
    int ret;

    PUSH((int)buf);
    PUSH(nbytes);
    push_str("write");
    PUSH(dev_desc);
    fword("$call-method");
    ret = POP();

    DPRINTF("obp_devwrite(fd 0x%x, buf %s, nbytes %d) = %d\n", dev_desc, buf, nbytes, ret);

    return nbytes;
}

static int obp_devseek(int dev_desc, int hi, int lo)
{
    int ret;

    PUSH(lo);
    PUSH(hi);
    push_str("seek");
    PUSH(dev_desc);
    fword("$call-method");
    ret = POP();

    DPRINTF("obp_devseek(fd 0x%x, hi %d, lo %d) = %d\n", dev_desc, hi, lo, ret);

    return ret;
}

static int obp_inst2pkg(int dev_desc)
{
    int ret;

    PUSH(dev_desc);
    fword("ihandle>phandle");
    ret = POP();

    DPRINTF("obp_inst2pkg(fd 0x%x) = 0x%x\n", dev_desc, ret);

    return ret;
}

static int obp_cpustart(__attribute__((unused))unsigned int whichcpu,
                        __attribute__((unused))int ctxtbl_ptr,
                        __attribute__((unused))int thiscontext,
                        __attribute__((unused))char *prog_counter)
{
    //int cpu, found;
#ifdef CONFIG_DEBUG_OBP
    struct linux_prom_registers *smp_ctable = (void *)ctxtbl_ptr;
#endif

    DPRINTF("obp_cpustart: cpu %d, ctxptr 0x%x, ctx %d, pc 0x%x\n", whichcpu,
            smp_ctable->phys_addr, thiscontext, (unsigned int)prog_counter);
#if 0
    found = obp_getprop(whichcpu, "mid", (char *)&cpu);
    if (found == -1)
        return -1;
    st_bypass(PHYS_JJ_EEPROM + 0x38, (unsigned int)prog_counter);
    st_bypass(PHYS_JJ_EEPROM + 0x3C, ((unsigned int)smp_ctable->phys_addr) >> 4);
    st_bypass(PHYS_JJ_EEPROM + 0x40, thiscontext);
    DPRINTF("obp_cpustart: sending interrupt to CPU %d\n", cpu);
    st_bypass(PHYS_JJ_INTR0 + 0x1000 * cpu + 8, 0x40000000);
#endif

    return 0;
}

static int obp_cpustop(__attribute__((unused)) unsigned int whichcpu)
{
    DPRINTF("obp_cpustop: cpu %d\n", whichcpu);

    return 0;
}

static int obp_cpuidle(__attribute__((unused)) unsigned int whichcpu)
{
    DPRINTF("obp_cpuidle: cpu %d\n", whichcpu);

    return 0;
}

static int obp_cpuresume(__attribute__((unused)) unsigned int whichcpu)
{
    DPRINTF("obp_cpuresume: cpu %d\n", whichcpu);

    return 0;
}

void *
init_openprom(unsigned long memsize, const char *cmdline, char boot_device)
{
    ptphys = totphys;
    ptmap = totmap;
    ptavail = totavail;

    /*
     * Form memory descriptors.
     */
    totphys[0].theres_more = NULL;
    totphys[0].start_adr = (char *) 0;
    totphys[0].num_bytes = memsize;

    totavail[0].theres_more = NULL;
    totavail[0].start_adr = (char *) 0;
    totavail[0].num_bytes = memsize;

    totmap[0].theres_more = NULL;
    totmap[0].start_adr = &_start;
    totmap[0].num_bytes = (unsigned long) &_iomem - (unsigned long) &_start;

#ifdef PROLL_TREE
    prop_mem_reg[0] = 0;
    prop_mem_reg[1] = 0;
    prop_mem_reg[2] = memsize;
    prop_mem_avail[0] = 0;
    prop_mem_avail[1] = 0;
    prop_mem_avail[2] = va2pa((int)&_data) - 1;
    prop_vmem_avail[0] = 0;
    prop_vmem_avail[1] = 0;
    prop_vmem_avail[2] = (int)&_start - 1;
    prop_vmem_avail[3] = 0;
    prop_vmem_avail[4] = 0xffe00000;
    prop_vmem_avail[5] = 0x00200000;
#endif

    // Linux wants a R/W romvec table
    romvec0.pv_magic_cookie = LINUX_OPPROM_MAGIC;
    romvec0.pv_romvers = 0;
    romvec0.pv_plugin_revision = 77;
    romvec0.pv_printrev = 0x10203;
    romvec0.pv_v0mem.v0_totphys = &ptphys;
    romvec0.pv_v0mem.v0_prommap = &ptmap;
    romvec0.pv_v0mem.v0_available = &ptavail;
    romvec0.pv_nodeops = &nodeops0;
    romvec0.pv_bootstr = (void *)doublewalk;
    romvec0.pv_v0devops.v0_devopen = &obp_devopen;
    romvec0.pv_v0devops.v0_devclose = &obp_devclose;
    romvec0.pv_v0devops.v0_rdblkdev = &obp_rdblkdev;
    romvec0.pv_stdin = &obp_stdin;
    romvec0.pv_stdout = &obp_stdout;
    romvec0.pv_getchar = obp_nbgetchar;
    romvec0.pv_putchar = (void (*)(int))obp_nbputchar;
    romvec0.pv_nbgetchar = obp_nbgetchar;
    romvec0.pv_nbputchar = obp_nbputchar;
    romvec0.pv_reboot = obp_reboot;
    romvec0.pv_printf = (void (*)(const char *fmt, ...))printk;
    romvec0.pv_abort = obp_abort;
    romvec0.pv_halt = obp_halt;
    romvec0.pv_synchook = &synch_hook;
    romvec0.pv_v0bootargs = &obp_argp;
    romvec0.pv_v2devops.v2_inst2pkg = obp_inst2pkg;
    romvec0.pv_v2devops.v2_dumb_mmap = obp_dumb_mmap;
    romvec0.pv_v2devops.v2_dumb_munmap = obp_dumb_munmap;
    romvec0.pv_v2devops.v2_dev_open = obp_devopen;
    romvec0.pv_v2devops.v2_dev_close = (void (*)(int))obp_devclose;
    romvec0.pv_v2devops.v2_dev_read = obp_devread;
    romvec0.pv_v2devops.v2_dev_write = obp_devwrite;
    romvec0.pv_v2devops.v2_dev_seek = obp_devseek;
    obp_arg.boot_dev_ctrl = 0;
    obp_arg.boot_dev_unit = 0;
    obp_arg.dev_partition = 0;
    obp_arg.argv[0] = "sd(0,0,0):d";

    switch(boot_device) {
    default:
    case 'a':
        obp_arg.argv[0] = "fd()";
        obp_arg.boot_dev[0] = 'f';
        obp_arg.boot_dev[1] = 'd';
        break;
    case 'd':
        obp_arg.boot_dev_unit = 2;
        obp_arg.argv[0] = "sd(0,2,0):d";
        // Fall through
    case 'c':
        obp_arg.boot_dev[0] = 's';
        obp_arg.boot_dev[1] = 'd';
        break;
    case 'n':
        obp_arg.argv[0] = "le()";
        obp_arg.boot_dev[0] = 'l';
        obp_arg.boot_dev[1] = 'e';
        break;
    }
    obp_arg.argv[1] = cmdline;
    romvec0.pv_v2bootargs.bootpath = &obp_arg.argv[0];
    romvec0.pv_v2bootargs.bootargs = &obp_arg.argv[1];
    romvec0.pv_v2bootargs.fd_stdin = &obp_fd_stdin;
    romvec0.pv_v2bootargs.fd_stdout = &obp_fd_stdout;

    push_str("/builtin/console");
    fword("open-dev");
    obp_fd_stdin = POP();
    push_str("/builtin/console");
    fword("open-dev");
    obp_fd_stdout = POP();
    
    obp_stdin = PROMDEV_TTYA;
    obp_stdout = PROMDEV_TTYA;

    romvec0.v3_cpustart = obp_cpustart;
    romvec0.v3_cpustop = obp_cpustop;
    romvec0.v3_cpuidle = obp_cpuidle;
    romvec0.v3_cpuresume = obp_cpuresume;

#ifdef STATIC_TREE
    get_tree();
#endif
    return &romvec0;
}
