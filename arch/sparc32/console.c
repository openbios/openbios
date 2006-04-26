/*
 * Copyright (C) 2003, 2004 Stefan Reinauer
 *
 * See the file "COPYING" for further information about
 * the copyright and warranty status of this work.
 */

#include "openbios/config.h"
#include "openbios/kernel.h"
#include "openbios.h"

#ifdef CONFIG_DEBUG_CONSOLE

/* ******************************************************************
 *                       serial console functions
 * ****************************************************************** */

#ifdef CONFIG_DEBUG_CONSOLE_SERIAL

#define SERIAL_BASE 0x71100004
#define CTRL(port) (SERIAL_BASE + (port) * 2 + 0)
#define DATA(port) (SERIAL_BASE + (port) * 2 + 2)

static int uart_charav(int port)
{
	return ((inb(CTRL(port)) & 1) != 0);
}

static char uart_getchar(int port)
{
	while (!uart_charav(port));
	return ((char) (inb(DATA(port))) & 0177);
}

static void uart_putchar(int port, unsigned char c)
{
	if (c == '\n')
		uart_putchar(port, '\r');
	while (!inb(CTRL(port)) & 4);
	outb(c, DATA(port));
}

static void uart_init_line(int port, unsigned long baud)
{
	outb(3, CTRL(port)); // reg 3
	outb(1, CTRL(port)); // enable rx

	outb(5, CTRL(port)); // reg 5
	outb(8, CTRL(port)); // enable tx

	// XXX: baud rate
}

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

#endif				// CONFIG_DEBUG_CONSOLE
