/*
 * Copyright (C) 2003, 2004 Stefan Reinauer
 *
 * See the file "COPYING" for further information about
 * the copyright and warranty status of this work.
 */

#include "openbios/config.h"
#include "openbios/bindings.h"
#include "openbios/kernel.h"
#include "openbios/drivers.h"
#include "openbios/fontdata.h"
#include "openbios.h"
#include "video_subr.h"
#include "libc/vsprintf.h"
#include "sys_info.h"
#include "boot.h"

/* ******************************************************************
 *                       serial console functions
 * ****************************************************************** */

#define SER_SIZE 8

#define RBR(x)  x==2?0x2f8:0x3f8
#define THR(x)  x==2?0x2f8:0x3f8
#define IER(x)  x==2?0x2f9:0x3f9
#define IIR(x)  x==2?0x2fa:0x3fa
#define LCR(x)  x==2?0x2fb:0x3fb
#define MCR(x)  x==2?0x2fc:0x3fc
#define LSR(x)  x==2?0x2fd:0x3fd
#define MSR(x)  x==2?0x2fe:0x3fe
#define SCR(x)  x==2?0x2ff:0x3ff
#define DLL(x)  x==2?0x2f8:0x3f8
#define DLM(x)  x==2?0x2f9:0x3f9

static int uart_charav(int port)
{
	return ((inb(LSR(port)) & 1) != 0);
}

static char uart_getchar(int port)
{
	while (!uart_charav(port));
	return ((char) inb(RBR(port)) & 0177);
}

static void uart_putchar(int port, unsigned char c)
{
	if (c == '\n')
		uart_putchar(port, '\r');
	while (!(inb(LSR(port)) & 0x20));
	outb(c, THR(port));
}

static void uart_init_line(int port, unsigned long baud)
{
	int i, baudconst;

	switch (baud) {
	case 115200:
		baudconst = 1;
		break;
	case 57600:
		baudconst = 2;
		break;
	case 38400:
		baudconst = 3;
		break;
	case 19200:
		baudconst = 6;
		break;
	case 9600:
	default:
		baudconst = 12;
		break;
	}

	outb(0x87, LCR(port));
	outb(0x00, DLM(port));
	outb(baudconst, DLL(port));
	outb(0x07, LCR(port));
	outb(0x0f, MCR(port));

	for (i = 10; i > 0; i--) {
		if (inb(LSR(port)) == (unsigned int) 0)
			break;
		inb(RBR(port));
	}
}

#ifdef CONFIG_DEBUG_CONSOLE
#ifdef CONFIG_DEBUG_CONSOLE_SERIAL

static void serial_cls(void);
static void serial_putchar(int c);

int uart_init(int port, unsigned long speed)
{
        uart_init_line(port, speed);
	return -1;
}

static void serial_putchar(int c)
{
	uart_putchar(CONFIG_SERIAL_PORT, (unsigned char) (c & 0xff));
}

static void serial_cls(void)
{
	serial_putchar(27);
	serial_putchar('[');
	serial_putchar('H');
	serial_putchar(27);
	serial_putchar('[');
	serial_putchar('J');
}

#endif

/* ******************************************************************
 *          simple polling video/keyboard console functions
 * ****************************************************************** */

/*
 *  keyboard driver
 */

static const char normal[] = {
	0x0, 0x1b, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-',
	'=', '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o',
	'p', '[', ']', 0xa, 0x0, 'a', 's', 'd', 'f', 'g', 'h', 'j',
	'k', 'l', ';', 0x27, 0x60, 0x0, 0x5c, 'z', 'x', 'c', 'v', 'b',
	'n', 'm', ',', '.', '/', 0x0, '*', 0x0, ' ', 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '0', 0x7f
};

static const char shifted[] = {
	0x0, 0x1b, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_',
	'+', '\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O',
	'P', '{', '}', 0xa, 0x0, 'A', 'S', 'D', 'F', 'G', 'H', 'J',
	'K', 'L', ':', 0x22, '~', 0x0, '|', 'Z', 'X', 'C', 'V', 'B',
	'N', 'M', '<', '>', '?', 0x0, '*', 0x0, ' ', 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '7', '8',
	'9', 0x0, '4', '5', '6', 0x0, '1', '2', '3', '0', 0x7f
};

static int key_ext;
static int key_lshift = 0, key_rshift = 0, key_caps = 0;

static char last_key;

static void keyboard_cmd(unsigned char cmd, unsigned char val)
{
	outb(cmd, 0x60);
	/* wait until keyboard controller accepts cmds: */
	while (inb(0x64) & 2);
	outb(val, 0x60);
	while (inb(0x64) & 2);
}

static void keyboard_controller_cmd(unsigned char cmd, unsigned char val)
{
	outb(cmd, 0x64);
	/* wait until keyboard controller accepts cmds: */
	while (inb(0x64) & 2);
	outb(val, 0x60);
	while (inb(0x64) & 2);
}

static char keyboard_poll(void)
{
	unsigned int c;
	if (inb(0x64) & 1) {
		c = inb(0x60);
		switch (c) {
		case 0xe0:
			key_ext = 1;
			return 0;
		case 0x2a:
			key_lshift = 1;
			return 0;
		case 0x36:
			key_rshift = 1;
			return 0;
		case 0xaa:
			key_lshift = 0;
			return 0;
		case 0xb6:
			key_rshift = 0;
			return 0;
		case 0x3a:
			if (key_caps) {
				key_caps = 0;
				keyboard_cmd(0xed, 0);
			} else {
				key_caps = 1;
				keyboard_cmd(0xed, 4);	/* set caps led */
			}
			return 0;
		}

		if (key_ext) {
			// void printk(const char *format, ...);
			printk("extended keycode: %x\n", c);

			key_ext = 0;
			return 0;
		}

		if (c & 0x80)	/* unhandled key release */
			return 0;

		if (key_lshift || key_rshift)
			return key_caps ? normal[c] : shifted[c];
		else
			return key_caps ? shifted[c] : normal[c];
	}
	return 0;
}

static int keyboard_dataready(void)
{
	if (last_key)
		return 1;

	last_key = keyboard_poll();

	return (last_key != 0);
}

static unsigned char keyboard_readdata(void)
{
	char tmp;
	while (!keyboard_dataready());
	tmp = last_key;
	last_key = 0;
	return tmp;
}

/* ******************************************************************
 *      common functions, implementing simple concurrent console
 * ****************************************************************** */

int putchar(int c)
{
#ifdef CONFIG_DEBUG_CONSOLE_SERIAL
	serial_putchar(c);
#endif
	return c;
}

int availchar(void)
{
#ifdef CONFIG_DEBUG_CONSOLE_SERIAL
	if (uart_charav(CONFIG_SERIAL_PORT))
		return 1;
#endif
#ifdef CONFIG_DEBUG_CONSOLE_VGA
	if (keyboard_dataready())
		return 1;
#endif
	return 0;
}

int getchar(void)
{
#ifdef CONFIG_DEBUG_CONSOLE_SERIAL
	if (uart_charav(CONFIG_SERIAL_PORT))
		return (uart_getchar(CONFIG_SERIAL_PORT));
#endif
#ifdef CONFIG_DEBUG_CONSOLE_VGA
	if (keyboard_dataready())
		return (keyboard_readdata());
#endif
	return 0;
}

void cls(void)
{
#ifdef CONFIG_DEBUG_CONSOLE_SERIAL
	serial_cls();
#endif
}

#endif // CONFIG_DEBUG_CONSOLE

/* ( addr len -- actual ) */
static void
su_read(unsigned long *address)
{
    char *addr;
    int len;

    len = POP();
    addr = (char *)POP();

    if (len != 1)
        printk("su_read: bad len, addr %lx len %x\n", (unsigned long)addr, len);

    if (uart_charav(*address)) {
        *addr = (char)uart_getchar(*address);
        PUSH(1);
    } else {
        PUSH(0);
    }
}

/* ( addr len -- actual ) */
static void
su_read_keyboard(void)
{
    unsigned char *addr;
    int len;

    len = POP();
    addr = (unsigned char *)POP();

    if (len != 1)
        printk("su_read: bad len, addr %lx len %x\n", (unsigned long)addr, len);

    if (keyboard_dataready()) {
        *addr = keyboard_readdata();
        PUSH(1);
    } else {
        PUSH(0);
    }
}

/* ( addr len -- actual ) */
static void
su_write(unsigned long *address)
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
su_close(void)
{
}

static void
su_open(unsigned long *address)
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

    RET ( -1 );
}

DECLARE_UNNAMED_NODE(su, INSTALL_OPEN, sizeof(unsigned long));

NODE_METHODS(su) = {
    { "open",               su_open              },
    { "close",              su_close             },
    { "read",               su_read              },
    { "write",              su_write             },
};

DECLARE_UNNAMED_NODE(su_keyboard, INSTALL_OPEN, sizeof(unsigned long));

NODE_METHODS(su_keyboard) = {
    { "open",               su_open              },
    { "close",              su_close             },
    { "read",               su_read_keyboard     },
};

void
ob_su_init(uint64_t base, uint64_t offset, int intr)
{
    push_str("/pci/isa");
    fword("find-device");
    fword("new-device");

    push_str("su");
    fword("device-name");

    push_str("serial");
    fword("device-type");

    PUSH((base + offset) >> 32);
    fword("encode-int");
    PUSH((base + offset) & 0xffffffff);
    fword("encode-int");
    fword("encode+");
    PUSH(SER_SIZE);
    fword("encode-int");
    fword("encode+");
    push_str("reg");
    fword("property");

    fword("finish-device");

    REGISTER_NODE_METHODS(su, "/pci/isa/su");

    push_str("/chosen");
    fword("find-device");

    push_str("/pci/isa/su");
    fword("open-dev");
    fword("encode-int");
    push_str("stdin");
    fword("property");

    push_str("/pci/isa/su");
    fword("open-dev");
    fword("encode-int");
    push_str("stdout");
    fword("property");

    keyboard_controller_cmd(0x60, 0x40); // Write mode command, translated mode
}
