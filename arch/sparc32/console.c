/*
 * Copyright (C) 2003, 2004 Stefan Reinauer
 *
 * See the file "COPYING" for further information about
 * the copyright and warranty status of this work.
 */

#include "config.h"
#include "kernel/kernel.h"
#include "drivers/drivers.h"
#include "openbios.h"
#include "libopenbios/ofmem.h"
#include "libopenbios/video.h"

#ifdef CONFIG_DEBUG_CONSOLE

/* ******************************************************************
 *          simple polling video/keyboard console functions
 * ****************************************************************** */

#ifdef CONFIG_DEBUG_CONSOLE_VIDEO

#define VMEM_BASE 0x00800000ULL
#define VMEM_SIZE (1024*768*1)
#define DAC_BASE  0x00200000ULL
#define DAC_SIZE  16

unsigned char *vmem;
volatile uint32_t *dac;

void tcx_init(uint64_t base)
{
    vmem = (unsigned char *)ofmem_map_io(base + VMEM_BASE, VMEM_SIZE);
    dac = (uint32_t *)ofmem_map_io(base + DAC_BASE, DAC_SIZE);
}

/* ( r g b index -- ) */
void tcx_hw_set_color(void)
{
    int index = POP();
    int b = POP();
    int g = POP();
    int r = POP();

    if( VIDEO_DICT_VALUE(video.depth) == 8 ) {
        dac[0] = index << 24;
        dac[1] = r << 24; // Red
        dac[1] = g << 24; // Green
        dac[1] = b << 24; // Blue
    }
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

#endif				// CONFIG_DEBUG_CONSOLE
