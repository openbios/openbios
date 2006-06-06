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

#define KBD_BASE    0x71000004
#define SERIAL_BASE 0x71100004
#define CTRL(port) (SERIAL_BASE + (port) * 2 + 0)
#define DATA(port) (SERIAL_BASE + (port) * 2 + 2)

/* Conversion routines to/from brg time constants from/to bits
 * per second.
 */
#define BRG_TO_BPS(brg, freq) ((freq) / 2 / ((brg) + 2))
#define BPS_TO_BRG(bps, freq) ((((freq) + (bps)) / (2 * (bps))) - 2)

#define ZS_CLOCK		4915200 /* Zilog input clock rate. */
#define ZS_CLOCK_DIVISOR	16      /* Divisor this driver uses. */

/* Write Register 3 */
#define	RxENAB  	0x1	/* Rx Enable */

/* Write Register 5 */
#define	TxENAB		0x8	/* Tx Enable */

/* Write Register 14 (Misc control bits) */
#define	BRENAB 	1	/* Baud rate generator enable */
#define	BRSRC	2	/* Baud rate generator source */

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
        outb(RxENAB, CTRL(port)); // enable rx

        outb(5, CTRL(port)); // reg 5
        outb(TxENAB, CTRL(port)); // enable tx

        baud = BPS_TO_BRG(baud, ZS_CLOCK / ZS_CLOCK_DIVISOR);

        outb(12, CTRL(port)); // reg 12
        outb(baud & 0xff, CTRL(port));
        outb(13, CTRL(port)); // reg 13
        outb((baud >> 8) & 0xff, CTRL(port));
        outb(14, CTRL(port)); // reg 14
        outb(BRSRC | BRENAB, CTRL(port));
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
 *          simple polling video/keyboard console functions
 * ****************************************************************** */

#ifdef CONFIG_DEBUG_CONSOLE_VIDEO

#define VMEM_BASE 0x00800000
#define VMEM_SIZE (1024*768*1)
#define DAC_BASE  0x00200000
#define DAC_SIZE  16

static unsigned char *vmem;
static volatile uint32_t *dac;

typedef struct osi_fb_info {
	unsigned long   mphys;
	int             rb, w, h, depth;
} osi_fb_info_t;

int TCX_GetFBInfo( osi_fb_info_t *fb )
{
    fb->w = 1024;
    fb->h = 768;
    fb->depth = 8;
    fb->rb = 1024;
    fb->mphys = vmem;

    return 0;
}

#define openbios_GetFBInfo(x) TCX_GetFBInfo(x)

#include "../../modules/video.c"
#include "../../modules/console.c"

static void video_putchar(int c)
{
    char buf[2];

    buf[0] = c & 0xff;
    buf[1] = 0;

    console_draw_str(buf);
}

static void video_cls(void)
{
    memset((void *)vmem, 0, VMEM_SIZE);
}

void tcx_init(unsigned long base)
{
    vmem = map_io(base + VMEM_BASE, VMEM_SIZE);
    dac = map_io(base + DAC_BASE, DAC_SIZE);

    console_init();
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

static int
keyboard_dataready(void)
{
    return ((inb(KBD_BASE) & 1) == 1);
}

static unsigned char
keyboard_readdata(void)
{
    unsigned char ch;

    while (!keyboard_dataready()) { }

    do {
        ch = inb(KBD_BASE + 2) & 0xff;
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

#endif

/* ******************************************************************
 *      common functions, implementing simple concurrent console
 * ****************************************************************** */

int putchar(int c)
{
#ifdef CONFIG_DEBUG_CONSOLE_SERIAL
	serial_putchar(c);
#endif
#ifdef CONFIG_DEBUG_CONSOLE_VIDEO
	video_putchar(c);
#endif
	return c;
}

int availchar(void)
{
#ifdef CONFIG_DEBUG_CONSOLE_SERIAL
	if (uart_charav(CONFIG_SERIAL_PORT))
		return 1;
#endif
#ifdef CONFIG_DEBUG_CONSOLE_VIDEO
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
#ifdef CONFIG_DEBUG_CONSOLE_VIDEO
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
#ifdef CONFIG_DEBUG_CONSOLE_VIDEO
	video_cls();
#endif
}


#endif				// CONFIG_DEBUG_CONSOLE
