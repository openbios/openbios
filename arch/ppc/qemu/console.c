/*
 *      <console.c>
 *
 *      Simple text console
 *
 *   Copyright (C) 2005 Stefan Reinauer <stepan@openbios.org>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation
 *
 */

#include "openbios/config.h"
#include "openbios/kernel.h"
#include "openbios/bindings.h"
#include "libc/diskio.h"
#include "ofmem.h"
#include "qemu/qemu.h"

#ifdef CONFIG_DEBUG_CONSOLE

/* ******************************************************************
 *                       serial console functions
 * ****************************************************************** */

#ifdef CONFIG_DEBUG_CONSOLE_SERIAL

#define UART(port)	(port==2?0x2f8:0x3f8)

#define RBR(port)	(UART(port) + 0)
#define THR(port)	(UART(port) + 0)
#define IER(port)	(UART(port) + 1)
#define IIR(port)	(UART(port) + 2)
#define LCR(port)	(UART(port) + 3)
#define MCR(port)	(UART(port) + 4)
#define LSR(port)	(UART(port) + 5)
#define MSR(port)	(UART(port) + 6)
#define SCR(port)	(UART(port) + 7)
#define DLL(port)	(UART(port) + 0)
#define DLM(port)	(UART(port) + 1)

static int uart_charav(int port)
{
	if (!port)
		return -1;
	return ((inb(LSR(port)) & 1) != 0);
}

static char uart_getchar(int port)
{
	if (!port)
		return -1;
	while (!uart_charav(port));
	return ((char) inb(RBR(port)) & 0177);
}

static void uart_putchar(int port, unsigned char c)
{
	if (!port)
		return -1;
        while (!(inb(LSR(port)) & 0x20));
        outb(c, THR(port));
}

static void serial_putchar(int c)
{
	if (c == '\n')
		uart_putchar(CONFIG_SERIAL_PORT, '\r');
	uart_putchar(CONFIG_SERIAL_PORT, c);
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

int
serial_init(void)
{
	if (CONFIG_SERIAL_PORT)
		uart_init_line(CONFIG_SERIAL_PORT, CONFIG_SERIAL_SPEED);
	return -1;
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
#endif	// CONFIG_DEBUG_CONSOLE_SERIAL

typedef struct osi_fb_info {
	unsigned long   mphys;
	int             rb, w, h, depth;
} osi_fb_info_t;


#define openbios_GetFBInfo(x) Qemu_GetFBInfo(x)

#include "../../../modules/font_8x16.c"
#include "../../../modules/video.c"
#include "../../../modules/console.c"

static uint32_t vga_phys_mem;
static int vga_width, vga_height, vga_depth;

int Qemu_GetFBInfo( osi_fb_info_t *fb )
{
	fb->mphys = vga_phys_mem;
	fb->w = vga_width;
	fb->h = vga_height;
	fb->depth = vga_depth;
	fb->rb = fb->w * ((fb->depth + 7) / 8);

	return 0;
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
        return 0;
}

int getchar(void)
{
#ifdef CONFIG_DEBUG_CONSOLE_SERIAL
	if (uart_charav(CONFIG_SERIAL_PORT))
		return (uart_getchar(CONFIG_SERIAL_PORT));
#endif
        return 0;
}

void cls(void)
{
#ifdef CONFIG_DEBUG_CONSOLE_SERIAL
	serial_cls();
#endif
}
#endif	// CONFIG_DEBUG_CONSOLE
