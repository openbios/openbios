/*
 *   OpenBIOS Sparc OBIO driver
 *   
 *   (C) 2004 Stefan Reinauer <stepan@openbios.org>
 *   (C) 2005 Ed Schouten <ed@fxq.nl>
 * 
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   version 2
 *
 */

#include "openbios/config.h"
#include "openbios/bindings.h"
#include "openbios/kernel.h"
#include "libc/byteorder.h"
#include "libc/vsprintf.h"

#include "openbios/drivers.h"
#include "openbios/nvram.h"
#include "obio.h"

#define REGISTER_NAMED_NODE( name, path )   do { \
	     bind_new_node( name##_flags_, name##_size_, \
			path, name##_m, sizeof(name##_m)/sizeof(method_t)); \
	} while(0)

#define REGISTER_NODE_METHODS( name, path )   do {                      \
        const char *paths[1];                                                  \
                                                                        \
        paths[0] = path;                                                \
        bind_node( name##_flags_, name##_size_,                         \
                   paths, 1, name##_m, sizeof(name##_m)/sizeof(method_t)); \
    } while(0)

#define	PROMDEV_KBD	0		/* input from keyboard */
#define	PROMDEV_SCREEN	0		/* output to screen */
#define	PROMDEV_TTYA	1		/* in/out to ttya */

/* "NCPU" is an historical name that's now a bit of a misnomer.  The sun4m
 * architecture registers NCPU CPU-specific interrupts along with one
 * system-wide interrupt, regardless of the number of actual CPUs installed.
 * See the comment on "NCPU" at <http://stuff.mit.edu/afs/athena/astaff/
 * project/opssrc/sys.sunos/sun4m/devaddr.h>.
 */
#define SUN4M_NCPU      4

/* The kernel may want to examine the arguments, so hold a copy in OBP's
 * mapped memory.
 */
#define OBIO_CMDLINE_MAX 256
char obio_cmdline[OBIO_CMDLINE_MAX];

/* DECLARE data structures for the nodes.  */
DECLARE_UNNAMED_NODE( ob_obio, INSTALL_OPEN, sizeof(int) );

static void
ob_new_obio_device(const char *name, const char *type)
{
    push_str("/obio");
    fword("find-device");
    fword("new-device");

    push_str(name);
    fword("device-name");

    if (type) {
        push_str(type);
        fword("device-type");
    }
}

static unsigned long
ob_reg(unsigned long base, unsigned long offset, unsigned long size, int map)
{
    PUSH(0);
    fword("encode-int");
    PUSH(offset);
    fword("encode-int");
    fword("encode+");
    PUSH(size);
    fword("encode-int");
    fword("encode+");
    push_str("reg");
    fword("property");

    if (map) {
        unsigned long addr;

        addr = (unsigned long)map_io(base + offset, size);

        PUSH(addr);
        fword("encode-int");
        PUSH(4);
        fword("encode-int");
        fword("encode+");
        push_str("address");
        fword("property");
        return addr;
    }
    return 0;
}

static void
ob_intr(int intr)
{
    PUSH(intr);
    fword("encode-int");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    push_str("intr");
    fword("property");
}

// XXX move all arch/sparc32/console.c stuff here
#define CTRL(addr) (*(char *)(addr))
#define DATA(addr) (*(char *)(addr + 2))

/* Read Register 0 */
#define	Rx_CH_AV	0x1	/* Rx Character Available */
#define	Tx_BUF_EMP	0x4	/* Tx Buffer empty */

static int uart_charav(int port)
{
    return (CTRL(port) & Rx_CH_AV) != 0;
}

static char uart_getchar(int port)
{
    while (!uart_charav(port));

    return DATA(port) & 0177;
}

static void uart_putchar(int port, unsigned char c)
{
	if (c == '\n')
		uart_putchar(port, '\r');
	while (!(CTRL(port) & Tx_BUF_EMP));

        DATA(port) = c;
}

/* ( addr len -- actual ) */
static void
zs_read(unsigned long *address)
{
    char *addr;
    int len;

    len = POP();
    addr = (char *)POP();

    if (len != 1)
        printk("zs_read: bad len, addr %x len %x\n", (unsigned int)addr, len);

    if (uart_charav(*address)) {
        *addr = (char)uart_getchar(*address);
        PUSH(1);
    } else {
        PUSH(0);
    }
}

int keyboard_dataready(void);
unsigned char keyboard_readdata(void);

/* ( addr len -- actual ) */
static void
zs_read_keyboard(unsigned long *address)
{
    unsigned char *addr;
    int len;

    len = POP();
    addr = (unsigned char *)POP();

    if (len != 1)
        printk("zs_read: bad len, addr %x len %x\n", (unsigned int)addr, len);

    if (keyboard_dataready()) {
        *addr = keyboard_readdata();
        PUSH(1);
    } else {
        PUSH(0);
    }
}

/* ( addr len -- actual ) */
static void
zs_write(unsigned long *address)
{
    unsigned char *addr;
    int i, len;

    len = POP();
    addr = (unsigned char *)POP();

     for (i = 0; i < len; i++) {
        uart_putchar(*address, addr[i]);
    }
    PUSH(len);
}

static void
zs_close(void)
{
}

static void
zs_open(unsigned long *address)
{
    int len;
    phandle_t ph;
    unsigned long *prop;
    char *args;

    fword("my-self");
    fword("ihandle>phandle");
    ph = (phandle_t)POP();
    prop = (unsigned long *)get_property(ph, "address", &len);
    *address = *prop;
    fword("my-args");
    args = pop_fstr_copy();
    if (args && args[0] == 'a')
        *address += 4;

    //printk("zs_open: address %lx, args %s\n", *address, args);
    RET ( -1 );
}

DECLARE_UNNAMED_NODE(zs, INSTALL_OPEN, sizeof(unsigned long));

NODE_METHODS(zs) = {
	{ "open",               zs_open              },
	{ "close",              zs_close             },
	{ "read",               zs_read              },
	{ "write",              zs_write             },
};

DECLARE_UNNAMED_NODE(zs_keyboard, INSTALL_OPEN, sizeof(unsigned long));

NODE_METHODS(zs_keyboard) = {
	{ "open",               zs_open              },
	{ "close",              zs_close             },
	{ "read",               zs_read_keyboard     },
};

static void
ob_zs_init(unsigned long base, unsigned long offset, int intr, int slave, int keyboard)
{
    char nodebuff[256];

    ob_new_obio_device("zs", "serial");

    ob_reg(base, offset, ZS_REGS, 1);

    PUSH(slave);
    fword("encode-int");
    push_str("slave");
    fword("property");

    if (keyboard) {
        PUSH(-1);
        fword("encode-int");
        push_str("keyboard");
        fword("property");

        PUSH(-1);
        fword("encode-int");
        push_str("mouse");
        fword("property");
    }

    ob_intr(intr);

    fword("finish-device");

    sprintf(nodebuff, "/obio/zs@0,%x", offset);
    if (keyboard) {
        REGISTER_NODE_METHODS(zs_keyboard, nodebuff);
    } else {
        REGISTER_NODE_METHODS(zs, nodebuff);
    }
}

static unsigned char *nvram;
struct qemu_nvram_v1 nv_info;

void
arch_nvram_get(char *data)
{
    memcpy(data, nvram, NVRAM_SIZE);
}

void
arch_nvram_put(char *data)
{
    memcpy(nvram, data, NVRAM_SIZE);
}

int
arch_nvram_size(void)
{
    return NVRAM_SIZE;
}

struct cpudef {
    const unsigned char *name;
    unsigned long iu_version;
};

static const struct cpudef sparc_defs[] = {
    {
        .name = "FMI,MB86904",
        .iu_version = 0x04 << 24, /* Impl 0, ver 4 */
    },
    {
        .name = "TI,TMS390Z55",
        .iu_version = 0x40000000,
    },
};

static const char *
id_cpu(void)
{
    unsigned long iu_version;
    unsigned int i;

    asm("rd %%psr, %0\n"
        : "=r"(iu_version) :);
    iu_version &= 0xff000000;

    for (i = 0; i < sizeof(sparc_defs)/sizeof(struct cpudef); i++) {
        if (iu_version == sparc_defs[i].iu_version)
            return sparc_defs[i].name;
    }
    return "CPU,Unknown";
}

static void
ob_nvram_init(unsigned long base, unsigned long offset)
{
    extern uint32_t kernel_image;
    extern uint32_t kernel_size;
    extern uint32_t cmdline;
    extern uint32_t cmdline_size;
    extern char boot_device;
    extern char obp_stdin, obp_stdout;
    extern const char *obp_stdin_path, *obp_stdout_path;

    const char *stdin, *stdout, *cpuname;
    unsigned int i;
    char nographic;
    uint32_t size;
    unsigned int machine_id;

    ob_new_obio_device("eeprom", NULL);

    nvram = (char *)ob_reg(base, offset, NVRAM_SIZE, 1);

    PUSH((unsigned long)nvram);
    fword("encode-int");
    push_str("address");
    fword("property");
    
    memcpy(&nv_info, nvram, sizeof(nv_info));
    machine_id = (unsigned int)nvram[0x1fd9] & 0xff;
    printk("Nvram id %s, version %d, machine id 0x%2.2x\n", nv_info.id_string,
           nv_info.version, machine_id);
    if (strcmp(nv_info.id_string, "QEMU_BIOS") || nv_info.version != 1) {
        printk("Unknown nvram, freezing!\n");
        for (;;);
    }

    kernel_image = nv_info.kernel_image;
    kernel_size = nv_info.kernel_size;
    size = nv_info.cmdline_size;
    if (size > OBIO_CMDLINE_MAX - 1)
        size = OBIO_CMDLINE_MAX - 1;
    memcpy(obio_cmdline, nv_info.cmdline, size);
    obio_cmdline[size] = '\0';
    cmdline = obio_cmdline;
    cmdline_size = size;

    boot_device = nv_info.boot_device;
    nographic = nv_info.nographic;

    push_str("mk48t08");
    fword("model");

    fword("finish-device");

    // Add /idprom
    push_str("/");
    fword("find-device");
    
    PUSH((long)&nvram[NVRAM_IDPROM]);
    PUSH(32);
    fword("encode-bytes");
    push_str("idprom");
    fword("property");

    switch (machine_id) {
    case 0x72:
        push_str("SPARCstation 10 (1 X 390Z55)");
        push_str("banner-name");
        fword("property");
        push_str("SUNW,S10,501-2365");
        push_str("model");
        fword("property");
        push_str("SUNW,SPARCstation-10");
        push_str("name");
        fword("property");
        break;
    case 0x80:
        push_str("SPARCstation 5");
        push_str("name");
        fword("property");
        push_str("SUNW,501-3059");
        push_str("model");
        fword("property");
        push_str("SUNW,SPARCstation-5");
        push_str("name");
        fword("property");
        break;
    default:
        printk("Unknown machine, freezing!\n");
        for (;;);
    }
    // Add cpus
    printk("CPUs: %x\n", nv_info.smp_cpus);
    for (i = 0; i < (unsigned int)nv_info.smp_cpus; i++) {
        push_str("/");
        fword("find-device");

        fword("new-device");

        cpuname = id_cpu();
        push_str(cpuname);
        fword("device-name");

        push_str("cpu");
        fword("device-type");

        PUSH(0);
        fword("encode-int");
        push_str("implementation");
        fword("property");

        PUSH(4);
        fword("encode-int");
        push_str("version");
        fword("property");

        push_str("");
        push_str("cache-physical?");
        fword("property");

        PUSH(32);
        fword("encode-int");
        push_str("cache-line-size");
        fword("property");

        PUSH(512);
        fword("encode-int");
        push_str("cache-nlines");
        fword("property");

        PUSH(4096);
        fword("encode-int");
        push_str("page-size");
        fword("property");

        PUSH(16);
        fword("encode-int");
        push_str("dcache-line-size");
        fword("property");

        PUSH(512);
        fword("encode-int");
        push_str("dcache-nlines");
        fword("property");

        PUSH(1);
        fword("encode-int");
        push_str("dcache-associativity");
        fword("property");

        PUSH(16);
        fword("encode-int");
        push_str("icache-line-size");
        fword("property");

        PUSH(512);
        fword("encode-int");
        push_str("icache-nlines");
        fword("property");

        PUSH(1);
        fword("encode-int");
        push_str("icache-associativity");
        fword("property");


        PUSH(0x20);
        fword("encode-int");
        push_str("ecache-line-size");
        fword("property");

        PUSH(0x4000);
        fword("encode-int");
        push_str("ecache-nlines");
        fword("property");

        PUSH(1);
        fword("encode-int");
        push_str("ecache-associativity");
        fword("property");
	
        PUSH(2);
        fword("encode-int");
        push_str("ncaches");
        fword("property");

        PUSH(4096);
        fword("encode-int");
        push_str("mmu-nctx");
        fword("property");

        PUSH(8);
        fword("encode-int");
        push_str("sparc-version");
        fword("property");

        PUSH(37);
        fword("encode-int");
        push_str("mask_rev");
        fword("property");

        PUSH(i);
        fword("encode-int");
        push_str("mid");
        fword("property");

	
	
        fword("finish-device");
    }

    if (nographic) {
        obp_stdin = PROMDEV_TTYA;
        obp_stdout = PROMDEV_TTYA;
        stdin = "/obio/zs@0,100000:a";
        stdout = "/obio/zs@0,100000:a";
    } else {
        obp_stdin = PROMDEV_KBD;
        obp_stdout = PROMDEV_SCREEN;
        stdin = "/obio/zs@0,0:a";
        stdout = "/iommu/sbus/SUNW,tcx";
    }

    push_str("/");
    fword("find-device");
    push_str(stdin);
    fword("encode-string");
    push_str("stdin-path");
    fword("encode-string");
    fword("property");

    push_str(stdout);
    fword("encode-string");
    push_str("stdout-path");
    fword("encode-string");
    fword("property");

    obp_stdin_path = stdin;
    obp_stdout_path = stdout;
}

static void
ob_fd_init(unsigned long base, unsigned long offset, int intr)
{
    ob_new_obio_device("SUNW,fdtwo", "block");

    ob_reg(base, offset, FD_REGS, 0);

    ob_intr(intr);

    fword("finish-device");
}


static void
ob_sconfig_init(unsigned long base, unsigned long offset)
{
    ob_new_obio_device("slavioconfig", NULL);

    ob_reg(base, offset, SCONFIG_REGS, 0);

    fword("finish-device");
}

static void
ob_auxio_init(unsigned long base, unsigned long offset)
{
    ob_new_obio_device("auxio", NULL);

    ob_reg(base, offset, AUXIO_REGS, 0);

    fword("finish-device");
}

static void
ob_power_init(unsigned long base, unsigned long offset, int intr)
{
    ob_new_obio_device("power", NULL);

    ob_reg(base, offset, POWER_REGS, 0);

    ob_intr(intr);

    fword("finish-device");
}

static void
ob_counter_init(unsigned long base, unsigned long offset)
{
    volatile struct sun4m_timer_regs *regs;
    int i;

    ob_new_obio_device("counter", NULL);

    for (i = 0; i < SUN4M_NCPU; i++) {
        PUSH(0);
        fword("encode-int");
        if (i != 0) fword("encode+");
        PUSH(offset + (i * PAGE_SIZE));
        fword("encode-int");
        fword("encode+");
        PUSH(COUNTER_REGS);
        fword("encode-int");
        fword("encode+");
    }

    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH(offset + 0x10000);
    fword("encode-int");
    fword("encode+");
    PUSH(COUNTER_REGS);
    fword("encode-int");
    fword("encode+");

    push_str("reg");
    fword("property");


    regs = map_io(base + offset, sizeof(*regs));
    regs->l10_timer_limit = (((1000000/100) + 1) << 10);
    regs->cpu_timers[0].l14_timer_limit = 0;

    PUSH((unsigned long)regs);
    fword("encode-int");
    push_str("address");
    fword("property");

    fword("finish-device");
}

static volatile struct sun4m_intregs *intregs;

static void
ob_interrupt_init(unsigned long base, unsigned long offset)
{
    int i;

    ob_new_obio_device("interrupt", NULL);

    for (i = 0; i < SUN4M_NCPU; i++) {
        PUSH(0);
        fword("encode-int");
        if (i != 0) fword("encode+");
        PUSH(offset + (i * PAGE_SIZE));
        fword("encode-int");
        fword("encode+");
        PUSH(INTERRUPT_REGS);
        fword("encode-int");
        fword("encode+");
    }

    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH(offset + 0x10000);
    fword("encode-int");
    fword("encode+");
    PUSH(INTERRUPT_REGS);
    fword("encode-int");
    fword("encode+");

    push_str("reg");
    fword("property");

    intregs = map_io(base + offset, sizeof(*intregs));
    intregs->set = ~SUN4M_INT_MASKALL;
    intregs->cpu_intregs[0].clear = ~0x17fff;

    // Is this correct? It works for NetBSD at least
    PUSH((int)intregs);
    fword("encode-int");
    PUSH((int)intregs);
    fword("encode-int");
    fword("encode+");
    push_str("address");
    fword("property");

    fword("finish-device");
}

int
start_cpu(unsigned int pc, unsigned int context_ptr, unsigned int context, int cpu)
{
    if (!cpu)
        return -1;

    *(uint32_t *)&nvram[0x38] = pc;
    *(uint32_t *)&nvram[0x3c] = context_ptr;
    *(uint32_t *)&nvram[0x40] = context;
    nvram[0x2e] = cpu & 0xff;

    intregs->cpu_intregs[cpu].set = SUN4M_SOFT_INT(14);

    return 0;
}


static void
ob_obio_open(__attribute__((unused))int *idx)
{
	int ret=1;
	RET ( -ret );
}

static void
ob_obio_close(__attribute__((unused))int *idx)
{
	selfword("close-deblocker");
}

static void
ob_obio_initialize(__attribute__((unused))int *idx)
{
    push_str("/");
    fword("find-device");
    fword("new-device");

    push_str("obio");
    fword("device-name");

    push_str("hierarchical");
    fword("device-type");

    PUSH(2);
    fword("encode-int");
    push_str("#address-cells");
    fword("property");

    PUSH(1);
    fword("encode-int");
    push_str("#size-cells");
    fword("property");

    fword("finish-device");
}

static void
ob_set_obio_ranges(unsigned long high, unsigned long base)
{
    push_str("/obio");
    fword("find-device");
    PUSH(0);
    fword("encode-int");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH(high);
    fword("encode-int");
    fword("encode+");
    PUSH(base);
    fword("encode-int");
    fword("encode+");
    PUSH(SLAVIO_SIZE);
    fword("encode-int");
    fword("encode+");
    push_str("ranges");
    fword("property");
}

static void
ob_obio_decodeunit(__attribute__((unused)) int *idx)
{
    fword("decode-unit-sbus");
}


static void
ob_obio_encodeunit(__attribute__((unused)) int *idx)
{
    fword("encode-unit-sbus");
}

NODE_METHODS(ob_obio) = {
	{ NULL,			ob_obio_initialize	},
	{ "open",		ob_obio_open		},
	{ "close",		ob_obio_close		},
	{ "encode-unit",	ob_obio_encodeunit	},
	{ "decode-unit",	ob_obio_decodeunit	},
};


int
ob_obio_init(unsigned long slavio_high, unsigned long slavio_base,
             unsigned long fd_offset, unsigned long counter_offset,
             unsigned long intr_offset)
{

    // All devices were integrated to NCR89C105, see
    // http://www.ibiblio.org/pub/historic-linux/early-ports/Sparc/NCR/NCR89C105.txt

    //printk("Initializing OBIO devices...\n");
#if 0 // XXX
    REGISTER_NAMED_NODE(ob_obio, "/obio");
    device_end();
#endif
    ob_set_obio_ranges(slavio_high, slavio_base);

    // Zilog Z8530 serial ports, see http://www.zilog.com
    // Must be before zs@0,0 or Linux won't boot
    ob_zs_init(slavio_base, SLAVIO_ZS1, ZS_INTR, 0, 0);

    ob_zs_init(slavio_base, SLAVIO_ZS, ZS_INTR, 1, 1);

    // M48T08 NVRAM, see http://www.st.com
    ob_nvram_init(slavio_base, SLAVIO_NVRAM);

    // 82078 FDC
    ob_fd_init(slavio_base, fd_offset, FD_INTR);

    ob_sconfig_init(slavio_base, SLAVIO_SCONFIG);

    ob_auxio_init(slavio_base, SLAVIO_AUXIO);

    ob_power_init(slavio_base, SLAVIO_POWER, POWER_INTR);

    ob_counter_init(slavio_base, counter_offset);

    ob_interrupt_init(slavio_base, intr_offset);

    return 0;
}
