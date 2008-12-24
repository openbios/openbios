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
#define cpu_to_be16(x) __cpu_to_be16(x)
#include "openbios/firmware_abi.h"
#define NO_QEMU_PROTOS
#include "openbios/fw_cfg.h"

#define UUID_FMT "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x"

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
static char obio_cmdline[OBIO_CMDLINE_MAX];

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
map_reg(uint64_t base, uint64_t offset, unsigned long size, int map,
        int phys_hi)
{
    PUSH(phys_hi);
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

static unsigned long
ob_reg(uint64_t base, uint64_t offset, unsigned long size, int map)
{
    return map_reg(base, offset, size, map, 0);
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

/* ******************************************************************
 *                       serial console functions
 * ****************************************************************** */

static volatile unsigned char *kbd_dev, *serial_dev;

#define CTRL(addr) (*(volatile unsigned char *)(addr))
#define DATA(addr) (*(volatile unsigned char *)(addr + 2))

/* Conversion routines to/from brg time constants from/to bits
 * per second.
 */
#define BRG_TO_BPS(brg, freq) ((freq) / 2 / ((brg) + 2))
#define BPS_TO_BRG(bps, freq) ((((freq) + (bps)) / (2 * (bps))) - 2)

#define ZS_CLOCK		4915200 /* Zilog input clock rate. */
#define ZS_CLOCK_DIVISOR	16      /* Divisor this driver uses. */

/* Write Register 3 */
#define	RxENAB  	0x1	/* Rx Enable */
#define	Rx8		0xc0	/* Rx 8 Bits/Character */

/* Write Register 4 */
#define	SB1		0x4	/* 1 stop bit/char */
#define	X16CLK		0x40	/* x16 clock mode */

/* Write Register 5 */
#define	RTS		0x2	/* RTS */
#define	TxENAB		0x8	/* Tx Enable */
#define	Tx8		0x60	/* Tx 8 bits/character */
#define	DTR		0x80	/* DTR */

/* Write Register 14 (Misc control bits) */
#define	BRENAB 	1	/* Baud rate generator enable */
#define	BRSRC	2	/* Baud rate generator source */

/* Read Register 0 */
#define	Rx_CH_AV	0x1	/* Rx Character Available */
#define	Tx_BUF_EMP	0x4	/* Tx Buffer empty */

int uart_charav(int port)
{
    return (CTRL(port) & Rx_CH_AV) != 0;
}

char uart_getchar(int port)
{
    while (!uart_charav(port))
        ;
    return DATA(port) & 0177;
}

static void uart_putchar(int port, unsigned char c)
{
    if (!serial_dev)
        return;

    if (c == '\n')
        uart_putchar(port, '\r');
    while (!(CTRL(port) & Tx_BUF_EMP))
        ;
    DATA(port) = c;
}

static void uart_init_line(volatile unsigned char *port, unsigned long baud)
{
    CTRL(port) = 4; // reg 4
    CTRL(port) = SB1 | X16CLK; // no parity, async, 1 stop bit, 16x
                               // clock

    baud = BPS_TO_BRG(baud, ZS_CLOCK / ZS_CLOCK_DIVISOR);

    CTRL(port) = 12; // reg 12
    CTRL(port) = baud & 0xff;
    CTRL(port) = 13; // reg 13
    CTRL(port) = (baud >> 8) & 0xff;
    CTRL(port) = 14; // reg 14
    CTRL(port) = BRSRC | BRENAB;

    CTRL(port) = 3; // reg 3
    CTRL(port) = RxENAB | Rx8; // enable rx, 8 bits/char

    CTRL(port) = 5; // reg 5
    CTRL(port) = RTS | TxENAB | Tx8 | DTR; // enable tx, 8 bits/char,
                                           // set RTS & DTR

}

int uart_init(uint64_t port, unsigned long speed)
{
    serial_dev = map_io(port & ~7ULL, 2 * 4);
    serial_dev += port & 7ULL;
    uart_init_line(serial_dev, speed);

    return -1;
}

void serial_putchar(int c)
{
    uart_putchar((int)(serial_dev + CONFIG_SERIAL_PORT),
                 (unsigned char) (c & 0xff));
}

void serial_cls(void)
{
    serial_putchar(27);
    serial_putchar('[');
    serial_putchar('H');
    serial_putchar(27);
    serial_putchar('[');
    serial_putchar('J');
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

void kbd_init(uint64_t base)
{
    kbd_dev = map_io(base, 2 * 4);
    kbd_dev += 4;
}

static const unsigned char sunkbd_keycode[128] = {
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0,
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0, 8,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 9,
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '\\', 13,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    ' ',
};

static const unsigned char sunkbd_keycode_shifted[128] = {
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0,
    '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0, 8,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 9,
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '|', 13,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    ' ',
};

static int shiftstate;

int
keyboard_dataready(void)
{
    return ((kbd_dev[0] & 1) == 1);
}

unsigned char
keyboard_readdata(void)
{
    unsigned char ch;

    while (!keyboard_dataready()) { }

    do {
        ch = kbd_dev[2] & 0xff;
        if (ch == 99)
            shiftstate |= 1;
        else if (ch == 110)
            shiftstate |= 2;
        else if (ch == 227)
            shiftstate &= ~1;
        else if (ch == 238)
            shiftstate &= ~2;
        //printk("getch: %d\n", ch);
    }
    while ((ch & 0x80) == 0 || ch == 238 || ch == 227); // Wait for key release
    //printk("getch rel: %d\n", ch);
    ch &= 0x7f;
    if (shiftstate)
        ch = sunkbd_keycode_shifted[ch];
    else
        ch = sunkbd_keycode[ch];
    //printk("getch xlate: %d\n", ch);

    return ch;
}

/* ( addr len -- actual ) */
static void
zs_read_keyboard(void)
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
    if (args) {
        if (args[0] == 'a')
            *address += 4;
        //printk("zs_open: address %lx, args %s\n", *address, args);
        free(args);
    }

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
ob_zs_init(uint64_t base, uint64_t offset, int intr, int slave, int keyboard)
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

    snprintf(nodebuff, sizeof(nodebuff), "/obio/zs@0,%x",
             (int)offset & 0xffffffff);
    if (keyboard) {
        REGISTER_NODE_METHODS(zs_keyboard, nodebuff);
    } else {
        REGISTER_NODE_METHODS(zs, nodebuff);
    }
}

static void
ob_eccmemctl_init(void)
{
    uint32_t version, *regs;
    const char *mc_type;

    push_str("/");
    fword("find-device");
    fword("new-device");

    push_str("eccmemctl");
    fword("device-name");

    PUSH(0x20);
    fword("encode-int");
    push_str("width");
    fword("property");

    regs = (uint32_t *)map_reg(ECC_BASE, 0, ECC_SIZE, 1, ECC_BASE >> 32);

    version = regs[0];
    switch (version) {
    case 0x00000000:
        mc_type = "MCC";
        break;
    case 0x10000000:
        mc_type = "EMC";
        break;
    default:
    case 0x20000000:
        mc_type = "SMC";
        break;
    }
    push_str(mc_type);
    fword("encode-string");
    push_str("mc-type");
    fword("property");

    fword("finish-device");
}

static unsigned char *nvram;
static ohwcfg_v3_t nv_info;

#define NVRAM_OB_START   (sizeof(ohwcfg_v3_t) + sizeof(struct sparc_arch_cfg))
#define NVRAM_OB_SIZE    ((NVRAM_IDPROM - NVRAM_OB_START) & ~15)

void
arch_nvram_get(char *data)
{
    memcpy(data, &nvram[NVRAM_OB_START], NVRAM_OB_SIZE);
}

void
arch_nvram_put(char *data)
{
    memcpy(&nvram[NVRAM_OB_START], data, NVRAM_OB_SIZE);
}

int
arch_nvram_size(void)
{
    return NVRAM_OB_SIZE;
}

static void mb86904_init(void)
{
    PUSH(32);
    fword("encode-int");
    push_str("cache-line-size");
    fword("property");

    PUSH(512);
    fword("encode-int");
    push_str("cache-nlines");
    fword("property");

    PUSH(0x23);
    fword("encode-int");
    push_str("mask_rev");
    fword("property");
}

static void tms390z55_init(void)
{
    push_str("");
    fword("encode-string");
    push_str("ecache-parity?");
    fword("property");

    push_str("");
    fword("encode-string");
    push_str("bfill?");
    fword("property");

    push_str("");
    fword("encode-string");
    push_str("bcopy?");
    fword("property");

    push_str("");
    fword("encode-string");
    push_str("cache-physical?");
    fword("property");

    PUSH(0xf);
    fword("encode-int");
    PUSH(0xf8fffffc);
    fword("encode-int");
    fword("encode+");
    PUSH(4);
    fword("encode-int");
    fword("encode+");

    PUSH(0xf);
    fword("encode-int");
    fword("encode+");
    PUSH(0xf8c00000);
    fword("encode-int");
    fword("encode+");
    PUSH(0x1000);
    fword("encode-int");
    fword("encode+");

    PUSH(0xf);
    fword("encode-int");
    fword("encode+");
    PUSH(0xf8000000);
    fword("encode-int");
    fword("encode+");
    PUSH(0x1000);
    fword("encode-int");
    fword("encode+");

    PUSH(0xf);
    fword("encode-int");
    fword("encode+");
    PUSH(0xf8800000);
    fword("encode-int");
    fword("encode+");
    PUSH(0x1000);
    fword("encode-int");
    fword("encode+");
    push_str("reg");
    fword("property");
}

static void rt625_init(void)
{
    PUSH(32);
    fword("encode-int");
    push_str("cache-line-size");
    fword("property");

    PUSH(512);
    fword("encode-int");
    push_str("cache-nlines");
    fword("property");

}

static void bad_cpu_init(void)
{
    printk("This CPU is not supported yet, freezing.\n");
    for(;;);
}

struct cpudef {
    unsigned long iu_version;
    const char *name;
    int psr_impl, psr_vers, impl, vers;
    int dcache_line_size, dcache_lines, dcache_assoc;
    int icache_line_size, icache_lines, icache_assoc;
    int ecache_line_size, ecache_lines, ecache_assoc;
    int mmu_nctx;
    void (*initfn)(void);
};

static const struct cpudef sparc_defs[] = {
    {
        .iu_version = 0x00 << 24, /* Impl 0, ver 0 */
        .name = "FMI,MB86900",
        .initfn = bad_cpu_init,
    },
    {
        .iu_version = 0x04 << 24, /* Impl 0, ver 4 */
        .name = "FMI,MB86904",
        .psr_impl = 0,
        .psr_vers = 5,
        .impl = 0,
        .vers = 5,
        .dcache_line_size = 0x10,
        .dcache_lines = 0x200,
        .dcache_assoc = 1,
        .icache_line_size = 0x20,
        .icache_lines = 0x200,
        .icache_assoc = 1,
        .ecache_line_size = 0x20,
        .ecache_lines = 0x4000,
        .ecache_assoc = 1,
        .mmu_nctx = 0x100,
        .initfn = mb86904_init,
    },
    {
        .iu_version = 0x05 << 24, /* Impl 0, ver 5 */
        .name = "FMI,MB86907",
        .psr_impl = 0,
        .psr_vers = 5,
        .impl = 0,
        .vers = 5,
        .dcache_line_size = 0x20,
        .dcache_lines = 0x200,
        .dcache_assoc = 1,
        .icache_line_size = 0x20,
        .icache_lines = 0x200,
        .icache_assoc = 1,
        .ecache_line_size = 0x20,
        .ecache_lines = 0x4000,
        .ecache_assoc = 1,
        .mmu_nctx = 0x100,
        .initfn = mb86904_init,
    },
    {
        .iu_version = 0x10 << 24, /* Impl 1, ver 0 */
        .name = "LSI,L64811",
        .initfn = bad_cpu_init,
    },
    {
        .iu_version = 0x11 << 24, /* Impl 1, ver 1 */
        .name = "CY,CY7C601",
        .psr_impl = 1,
        .psr_vers = 1,
        .impl = 1,
        .vers = 1,
        .mmu_nctx = 0x10,
        .initfn = bad_cpu_init,
    },
    {
        .iu_version = 0x13 << 24, /* Impl 1, ver 3 */
        .name = "CY,CY7C611",
        .initfn = bad_cpu_init,
    },
    {
        .iu_version = 0x40000000,
        .name = "TI,TMS390Z55",
        .psr_impl = 4,
        .psr_vers = 0,
        .impl = 0,
        .vers = 4,
        .dcache_line_size = 0x20,
        .dcache_lines = 0x80,
        .dcache_assoc = 4,
        .icache_line_size = 0x40,
        .icache_lines = 0x40,
        .icache_assoc = 5,
        .ecache_line_size = 0x20,
        .ecache_lines = 0x8000,
        .ecache_assoc = 1,
        .mmu_nctx = 0x10000,
        .initfn = tms390z55_init,
    },
    {
        .iu_version = 0x41000000,
        .name = "TI,TMS390S10",
        .psr_impl = 4,
        .psr_vers = 1,
        .impl = 4,
        .vers = 1,
        .dcache_line_size = 0x10,
        .dcache_lines = 0x80,
        .dcache_assoc = 4,
        .icache_line_size = 0x20,
        .icache_lines = 0x80,
        .icache_assoc = 5,
        .ecache_line_size = 0x20,
        .ecache_lines = 0x8000,
        .ecache_assoc = 1,
        .mmu_nctx = 0x10000,
        .initfn = tms390z55_init,
    },
    {
        .iu_version = 0x42000000,
        .name = "TI,TMS390S10",
        .psr_impl = 4,
        .psr_vers = 2,
        .impl = 4,
        .vers = 2,
        .dcache_line_size = 0x10,
        .dcache_lines = 0x80,
        .dcache_assoc = 4,
        .icache_line_size = 0x20,
        .icache_lines = 0x80,
        .icache_assoc = 5,
        .ecache_line_size = 0x20,
        .ecache_lines = 0x8000,
        .ecache_assoc = 1,
        .mmu_nctx = 0x10000,
        .initfn = tms390z55_init,
    },
    {
        .iu_version = 0x43000000,
        .name = "TI,TMS390S10",
        .psr_impl = 4,
        .psr_vers = 3,
        .impl = 4,
        .vers = 3,
        .dcache_line_size = 0x10,
        .dcache_lines = 0x80,
        .dcache_assoc = 4,
        .icache_line_size = 0x20,
        .icache_lines = 0x80,
        .icache_assoc = 5,
        .ecache_line_size = 0x20,
        .ecache_lines = 0x8000,
        .ecache_assoc = 1,
        .mmu_nctx = 0x10000,
        .initfn = tms390z55_init,
    },
    {
        .iu_version = 0x44000000,
        .name = "TI,TMS390S10",
        .psr_impl = 4,
        .psr_vers = 4,
        .impl = 4,
        .vers = 4,
        .dcache_line_size = 0x10,
        .dcache_lines = 0x80,
        .dcache_assoc = 4,
        .icache_line_size = 0x20,
        .icache_lines = 0x80,
        .icache_assoc = 5,
        .ecache_line_size = 0x20,
        .ecache_lines = 0x8000,
        .ecache_assoc = 1,
        .mmu_nctx = 0x10000,
        .initfn = tms390z55_init,
    },
    {
        .iu_version = 0x1e000000,
        .name = "Ross,RT625",
        .psr_impl = 1,
        .psr_vers = 14,
        .impl = 1,
        .vers = 7,
        .dcache_line_size = 0x20,
        .dcache_lines = 0x80,
        .dcache_assoc = 4,
        .icache_line_size = 0x40,
        .icache_lines = 0x40,
        .icache_assoc = 5,
        .ecache_line_size = 0x20,
        .ecache_lines = 0x8000,
        .ecache_assoc = 1,
        .mmu_nctx = 0x10000,
        .initfn = rt625_init,
    },
    {
        .iu_version = 0x1f000000,
        .name = "Ross,RT620",
        .psr_impl = 1,
        .psr_vers = 15,
        .impl = 1,
        .vers = 7,
        .dcache_line_size = 0x20,
        .dcache_lines = 0x80,
        .dcache_assoc = 4,
        .icache_line_size = 0x40,
        .icache_lines = 0x40,
        .icache_assoc = 5,
        .ecache_line_size = 0x20,
        .ecache_lines = 0x8000,
        .ecache_assoc = 1,
        .mmu_nctx = 0x10000,
        .initfn = rt625_init,
    },
    {
        .iu_version = 0x20000000,
        .name = "BIT,B5010",
        .initfn = bad_cpu_init,
    },
    {
        .iu_version = 0x50000000,
        .name = "MC,MN10501",
        .initfn = bad_cpu_init,
    },
    {
        .iu_version = 0x90 << 24, /* Impl 9, ver 0 */
        .name = "Weitek,W8601",
        .initfn = bad_cpu_init,
    },
    {
        .iu_version = 0xf2000000,
        .name = "GR,LEON2",
        .initfn = bad_cpu_init,
    },
    {
        .iu_version = 0xf3000000,
        .name = "GR,LEON3",
        .initfn = bad_cpu_init,
    },
};

static const struct cpudef *
id_cpu(void)
{
    unsigned long iu_version;
    unsigned int i;

    asm("rd %%psr, %0\n"
        : "=r"(iu_version) :);
    iu_version &= 0xff000000;

    for (i = 0; i < sizeof(sparc_defs)/sizeof(struct cpudef); i++) {
        if (iu_version == sparc_defs[i].iu_version)
            return &sparc_defs[i];
    }
    printk("Unknown cpu (psr %lx), freezing!\n", iu_version);
    for (;;);
}

static void dummy_mach_init(void)
{
}

struct machdef {
    uint16_t machine_id;
    const char *banner_name;
    const char *model;
    const char *name;
    int mid_offset;
    void (*initfn)(void);
};

static const struct machdef sun4m_defs[] = {
    {
        .machine_id = 32,
        .banner_name = "SPARCstation 5",
        .model = "SUNW,501-3059",
        .name = "SUNW,SPARCstation-5",
        .mid_offset = 0,
        .initfn = dummy_mach_init,
    },
    {
        .machine_id = 33,
        .banner_name = "SPARCstation Voyager",
        .model = "SUNW,501-2581",
        .name = "SUNW,SPARCstation-Voyager",
        .mid_offset = 0,
        .initfn = dummy_mach_init,
    },
    {
        .machine_id = 34,
        .banner_name = "SPARCstation LX",
        .model = "SUNW,501-2031",
        .name = "SUNW,SPARCstation-LX",
        .mid_offset = 0,
        .initfn = dummy_mach_init,
    },
    {
        .machine_id = 35,
        .banner_name = "SPARCstation 4",
        .model = "SUNW,501-2572",
        .name = "SUNW,SPARCstation-4",
        .mid_offset = 0,
        .initfn = dummy_mach_init,
    },
    {
        .machine_id = 36,
        .banner_name = "SPARCstation Classic",
        .model = "SUNW,501-2326",
        .name = "SUNW,SPARCstation-Classic",
        .mid_offset = 0,
        .initfn = dummy_mach_init,
    },
    {
        .machine_id = 37,
        .banner_name = "Tadpole S3 GX",
        .model = "S3",
        .name = "Tadpole_S3GX",
        .mid_offset = 0,
        .initfn = dummy_mach_init,
    },
    {
        .machine_id = 64,
        .banner_name = "SPARCstation 10 (1 X 390Z55)",
        .model = "SUNW,S10,501-2365",
        .name = "SUNW,SPARCstation-10",
        .mid_offset = 8,
        .initfn = ob_eccmemctl_init,
    },
    {
        .machine_id = 65,
        .banner_name = "SPARCstation 20 (1 X 390Z55)",
        .model = "SUNW,S20,501-2324",
        .name = "SUNW,SPARCstation-20",
        .mid_offset = 8,
        .initfn = ob_eccmemctl_init,
    },
    {
        .machine_id = 66,
        .banner_name = "SPARCsystem 600(1 X 390Z55)",
        .model = NULL,
        .name = "SUNW,SPARCsystem-600",
        .mid_offset = 8,
        .initfn = ob_eccmemctl_init,
    },
};

static const struct machdef *
id_machine(uint16_t machine_id)
{
    unsigned int i;

    for (i = 0; i < sizeof(sun4m_defs)/sizeof(struct machdef); i++) {
        if (machine_id == sun4m_defs[i].machine_id)
            return &sun4m_defs[i];
    }
    printk("Unknown machine (ID %d), freezing!\n", machine_id);
    for (;;);
}

static volatile uint16_t *fw_cfg_cmd;
static volatile uint8_t *fw_cfg_data;

static void
fw_cfg_read(uint16_t cmd, char *buf, unsigned int nbytes)
{
    unsigned int i;

    *fw_cfg_cmd = cmd;
    for (i = 0; i < nbytes; i++)
        buf[i] = *fw_cfg_data;
}

static uint32_t
fw_cfg_read_i32(uint16_t cmd)
{
    char buf[sizeof(uint32_t)];

    fw_cfg_read(cmd, buf, sizeof(uint32_t));

    return __le32_to_cpu(*(uint32_t *)buf);
}

static uint16_t
fw_cfg_read_i16(uint16_t cmd)
{
    char buf[sizeof(uint16_t)];

    fw_cfg_read(cmd, buf, sizeof(uint16_t));

    return __le16_to_cpu(*(uint16_t *)buf);
}

static uint8_t qemu_uuid[16];

static void
ob_nvram_init(uint64_t base, uint64_t offset)
{
    const char *stdin, *stdout;
    unsigned int i;
    char nographic;
    uint32_t size;
    uint16_t machine_id;
    const struct cpudef *cpu;
    const struct machdef *mach;
    ohwcfg_v3_t *header;
    char buf[256];
    uint32_t temp;

    ob_new_obio_device("eeprom", NULL);

    nvram = (unsigned char *)ob_reg(base, offset, NVRAM_SIZE, 1);

    PUSH((unsigned long)nvram);
    fword("encode-int");
    push_str("address");
    fword("property");

    push_str("mk48t08");
    fword("model");

    fword("finish-device");

    fw_cfg_cmd = map_io(CFG_ADDR, CFG_SIZE);
    fw_cfg_data = (uint8_t *)fw_cfg_cmd + 2;

    fw_cfg_read(FW_CFG_SIGNATURE, buf, 4);
    buf[4] = '\0';

    printk("Configuration device id %s", buf);

    temp = fw_cfg_read_i32(FW_CFG_ID);
    machine_id = fw_cfg_read_i16(FW_CFG_MACHINE_ID);

    printk(" version %d machine id %d\n", temp, machine_id);

    if (temp != 1) {
        printk("Incompatible configuration device version, freezing\n");
        for(;;);
    }

    memcpy(&nv_info, nvram, sizeof(nv_info));
    kernel_image = nv_info.kernel_image;
    kernel_size = nv_info.kernel_size;
    size = nv_info.cmdline_size;
    if (size > OBIO_CMDLINE_MAX - 1)
        size = OBIO_CMDLINE_MAX - 1;
    memcpy(&obio_cmdline, (void *)(long)nv_info.cmdline, size);
    obio_cmdline[size] = '\0';
    qemu_cmdline = (uint32_t) &obio_cmdline;
    cmdline_size = size;
    header = (ohwcfg_v3_t *)nvram;
    header->kernel_image = 0;
    header->kernel_size = 0;
    header->cmdline_size = 0;
    header->crc = OHW_compute_crc(header, 0x00, 0xF8);

    boot_device = nv_info.boot_devices[0];
    fw_cfg_read(FW_CFG_NOGRAPHIC, &nographic, 1);
    graphic_depth = fw_cfg_read_i16(FW_CFG_SUN4M_DEPTH);

    // Add /uuid
    fw_cfg_read(FW_CFG_UUID, (char *)qemu_uuid, 16);

    printk("UUID: " UUID_FMT "\n", qemu_uuid[0], qemu_uuid[1], qemu_uuid[2],
           qemu_uuid[3], qemu_uuid[4], qemu_uuid[5], qemu_uuid[6],
           qemu_uuid[7], qemu_uuid[8], qemu_uuid[9], qemu_uuid[10],
           qemu_uuid[11], qemu_uuid[12], qemu_uuid[13], qemu_uuid[14],
           qemu_uuid[15]);

    push_str("/");
    fword("find-device");

    PUSH((long)&qemu_uuid);
    PUSH(16);
    fword("encode-bytes");
    push_str("uuid");
    fword("property");

    // Add /idprom
    push_str("/");
    fword("find-device");

    PUSH((long)&nvram[NVRAM_IDPROM]);
    PUSH(32);
    fword("encode-bytes");
    push_str("idprom");
    fword("property");

    mach = id_machine(machine_id);

    mach->initfn();

    push_str(mach->banner_name);
    fword("encode-string");
    push_str("banner-name");
    fword("property");

    if (mach->model) {
        push_str(mach->model);
        fword("encode-string");
        push_str("model");
        fword("property");
    }
    push_str(mach->name);
    fword("encode-string");
    push_str("name");
    fword("property");

    // Add cpus
    temp = fw_cfg_read_i32(FW_CFG_NB_CPUS);

    printk("CPUs: %x", temp);
    cpu = id_cpu();
    printk(" x %s\n", cpu->name);
    for (i = 0; i < temp; i++) {
        push_str("/");
        fword("find-device");

        fword("new-device");

        push_str(cpu->name);
        fword("device-name");

        push_str("cpu");
        fword("device-type");

        PUSH(cpu->psr_impl);
        fword("encode-int");
        push_str("psr-implementation");
        fword("property");

        PUSH(cpu->psr_vers);
        fword("encode-int");
        push_str("psr-version");
        fword("property");

        PUSH(cpu->impl);
        fword("encode-int");
        push_str("implementation");
        fword("property");

        PUSH(cpu->vers);
        fword("encode-int");
        push_str("version");
        fword("property");

        PUSH(4096);
        fword("encode-int");
        push_str("page-size");
        fword("property");

        PUSH(cpu->dcache_line_size);
        fword("encode-int");
        push_str("dcache-line-size");
        fword("property");

        PUSH(cpu->dcache_lines);
        fword("encode-int");
        push_str("dcache-nlines");
        fword("property");

        PUSH(cpu->dcache_assoc);
        fword("encode-int");
        push_str("dcache-associativity");
        fword("property");

        PUSH(cpu->icache_line_size);
        fword("encode-int");
        push_str("icache-line-size");
        fword("property");

        PUSH(cpu->icache_lines);
        fword("encode-int");
        push_str("icache-nlines");
        fword("property");

        PUSH(cpu->icache_assoc);
        fword("encode-int");
        push_str("icache-associativity");
        fword("property");

        PUSH(cpu->ecache_line_size);
        fword("encode-int");
        push_str("ecache-line-size");
        fword("property");

        PUSH(cpu->ecache_lines);
        fword("encode-int");
        push_str("ecache-nlines");
        fword("property");

        PUSH(cpu->ecache_assoc);
        fword("encode-int");
        push_str("ecache-associativity");
        fword("property");

        PUSH(2);
        fword("encode-int");
        push_str("ncaches");
        fword("property");

        PUSH(cpu->mmu_nctx);
        fword("encode-int");
        push_str("mmu-nctx");
        fword("property");

        PUSH(8);
        fword("encode-int");
        push_str("sparc-version");
        fword("property");

        push_str("");
        fword("encode-string");
        push_str("cache-coherence?");
        fword("property");

        PUSH(i + mach->mid_offset);
        fword("encode-int");
        push_str("mid");
        fword("property");

        cpu->initfn();

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
    fword("property");

    push_str(stdout);
    fword("encode-string");
    push_str("stdout-path");
    fword("property");

    obp_stdin_path = stdin;
    obp_stdout_path = stdout;
}

static void
ob_fd_init(uint64_t base, uint64_t offset, int intr)
{
    ob_new_obio_device("SUNW,fdtwo", "block");

    ob_reg(base, offset, FD_REGS, 0);

    ob_intr(intr);

    fword("finish-device");
}


static void
ob_sconfig_init(uint64_t base, uint64_t offset)
{
    ob_new_obio_device("slavioconfig", NULL);

    ob_reg(base, offset, SCONFIG_REGS, 0);

    fword("finish-device");
}

static void
ob_auxio_init(uint64_t base, uint64_t offset)
{
    ob_new_obio_device("auxio", NULL);

    ob_reg(base, offset, AUXIO_REGS, 0);

    fword("finish-device");
}

volatile unsigned char *power_reg;
volatile unsigned int *reset_reg;

static void
sparc32_reset_all(void)
{
    *reset_reg = 1;
}

// AUX 2 (Software Powerdown Control) and reset
static void
ob_aux2_reset_init(uint64_t base, uint64_t offset, int intr)
{
    ob_new_obio_device("power", NULL);

    power_reg = (void *)ob_reg(base, offset, AUXIO2_REGS, 1);

    // Not in device tree
    reset_reg = map_io(base + (uint64_t)SLAVIO_RESET, RESET_REGS);

    bind_func("sparc32-reset-all", sparc32_reset_all);
    push_str("' sparc32-reset-all to reset-all");
    fword("eval");

    ob_intr(intr);

    fword("finish-device");
}

volatile struct sun4m_timer_regs *counter_regs;

static void
ob_counter_init(uint64_t base, unsigned long offset)
{
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


    counter_regs = map_io(base + (uint64_t)offset, sizeof(*counter_regs));
    counter_regs->cfg = 0xffffffff;
    counter_regs->l10_timer_limit = (((1000000/100) + 1) << 10);
    counter_regs->cpu_timers[0].l14_timer_limit = 0;
    counter_regs->cpu_timers[0].cntrl = 1;

    for (i = 0; i < SUN4M_NCPU; i++) {
        PUSH((unsigned long)&counter_regs->cpu_timers[i]);
        fword("encode-int");
        if (i != 0)
            fword("encode+");
    }
    PUSH((unsigned long)&counter_regs->l10_timer_limit);
    fword("encode-int");
    fword("encode+");
    push_str("address");
    fword("property");

    fword("finish-device");
}

static volatile struct sun4m_intregs *intregs;

static void
ob_interrupt_init(uint64_t base, unsigned long offset)
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

    intregs = map_io(base | (uint64_t)offset, sizeof(*intregs));
    intregs->clear = ~SUN4M_INT_MASKALL;
    intregs->cpu_intregs[0].clear = ~0x17fff;

    for (i = 0; i < SUN4M_NCPU; i++) {
        PUSH((unsigned long)&intregs->cpu_intregs[i]);
        fword("encode-int");
        if (i != 0)
            fword("encode+");
    }
    PUSH((unsigned long)&intregs->tbt);
    fword("encode-int");
    fword("encode+");
    push_str("address");
    fword("property");

    fword("finish-device");
}

int
start_cpu(unsigned int pc, unsigned int context_ptr, unsigned int context, int cpu)
{
    ohwcfg_v3_t *header = (ohwcfg_v3_t *)nvram;
    struct sparc_arch_cfg *sparc_header;

    if (!cpu)
        return -1;

    sparc_header = (struct sparc_arch_cfg *)&nvram[header->nvram_arch_ptr];
    sparc_header->smp_entry = pc;
    sparc_header->smp_ctxtbl = context_ptr;
    sparc_header->smp_ctx = context;
    sparc_header->valid = 1;

    cpu &= 7;
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
ob_set_obio_ranges(uint64_t base)
{
    push_str("/obio");
    fword("find-device");
    PUSH(0);
    fword("encode-int");
    PUSH(0);
    fword("encode-int");
    fword("encode+");
    PUSH(base >> 32);
    fword("encode-int");
    fword("encode+");
    PUSH(base & 0xffffffff);
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
ob_obio_init(uint64_t slavio_base, unsigned long fd_offset,
             unsigned long counter_offset, unsigned long intr_offset,
             unsigned long aux1_offset, unsigned long aux2_offset)
{

    // All devices were integrated to NCR89C105, see
    // http://www.ibiblio.org/pub/historic-linux/early-ports/Sparc/NCR/NCR89C105.txt

    //printk("Initializing OBIO devices...\n");
#if 0 // XXX
    REGISTER_NAMED_NODE(ob_obio, "/obio");
    device_end();
#endif
    ob_set_obio_ranges(slavio_base);

    // Zilog Z8530 serial ports, see http://www.zilog.com
    // Must be before zs@0,0 or Linux won't boot
    ob_zs_init(slavio_base, SLAVIO_ZS1, ZS_INTR, 0, 0);

    ob_zs_init(slavio_base, SLAVIO_ZS, ZS_INTR, 1, 1);

    // M48T08 NVRAM, see http://www.st.com
    ob_nvram_init(slavio_base, SLAVIO_NVRAM);

    // 82078 FDC
    if (fd_offset != (unsigned long) -1)
        ob_fd_init(slavio_base, fd_offset, FD_INTR);

    ob_sconfig_init(slavio_base, SLAVIO_SCONFIG);

    ob_auxio_init(slavio_base, aux1_offset);

    if (aux2_offset != (unsigned long) -1)
        ob_aux2_reset_init(slavio_base, aux2_offset, AUXIO2_INTR);

    ob_counter_init(slavio_base, counter_offset);

    ob_interrupt_init(slavio_base, intr_offset);

    return 0;
}
