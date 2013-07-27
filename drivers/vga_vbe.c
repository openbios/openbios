/*
 *  Copyright (c) 2004-2005 Fabrice Bellard
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License V2
 *   as published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 *   MA 02110-1301, USA.
 */

#include "config.h"
#include "kernel/kernel.h"
#include "libopenbios/bindings.h"
#include "drivers/pci.h"
#include "drivers/drivers.h"
#include "libopenbios/fontdata.h"
#include "asm/io.h"
#include "libc/vsprintf.h"
#include "drivers/vga.h"
#include "libopenbios/video.h"
#include "libopenbios/ofmem.h"
#include "packages/video.h"

/* VGA init. We use the Bochs VESA VBE extensions  */
#define VBE_DISPI_INDEX_ID              0x0
#define VBE_DISPI_INDEX_XRES            0x1
#define VBE_DISPI_INDEX_YRES            0x2
#define VBE_DISPI_INDEX_BPP             0x3
#define VBE_DISPI_INDEX_ENABLE          0x4
#define VBE_DISPI_INDEX_BANK            0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH      0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT     0x7
#define VBE_DISPI_INDEX_X_OFFSET        0x8
#define VBE_DISPI_INDEX_Y_OFFSET        0x9
#define VBE_DISPI_INDEX_NB              0xa

#define VBE_DISPI_ID0                   0xB0C0
#define VBE_DISPI_ID1                   0xB0C1
#define VBE_DISPI_ID2                   0xB0C2

#define VBE_DISPI_DISABLED              0x00
#define VBE_DISPI_ENABLED               0x01
#define VBE_DISPI_LFB_ENABLED           0x40
#define VBE_DISPI_NOCLEARMEM            0x80

static void vbe_outw(int index, int val)
{
    outw(index, 0x1ce);
    outw(val, 0x1d0);
}

/* for depth = 8 mode, set a hardware palette entry */
void vga_set_color(int i, unsigned int r, unsigned int g, unsigned int b)
{
    r &= 0xff;
    g &= 0xff;
    b &= 0xff;
    outb(i, 0x3c8);
    outb(r >> 2, 0x3c9);
    outb(g >> 2, 0x3c9);
    outb(b >> 2, 0x3c9);
}

/* build standard RGB palette */
static void vga_build_rgb_palette(void)
{
    static const uint8_t pal_value[6] = { 0x00, 0x33, 0x66, 0x99, 0xcc, 0xff };
    int i, r, g, b;

    i = 0;
    for(r = 0; r < 6; r++) {
        for(g = 0; g < 6; g++) {
            for(b = 0; b < 6; b++) {
                vga_set_color(i, pal_value[r], pal_value[g], pal_value[b]);
                i++;
            }
        }
    }
}

/* depth = 8, 15, 16 or 32 */
void vga_vbe_set_mode(int width, int height, int depth)
{
    outb(0x00, 0x3c0); /* enable blanking */
    vbe_outw(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    vbe_outw(VBE_DISPI_INDEX_X_OFFSET, 0);
    vbe_outw(VBE_DISPI_INDEX_Y_OFFSET, 0);
    vbe_outw(VBE_DISPI_INDEX_XRES, width);
    vbe_outw(VBE_DISPI_INDEX_YRES, height);
    vbe_outw(VBE_DISPI_INDEX_BPP, depth);
    vbe_outw(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED);
    outb(0x00, 0x3c0);
    outb(0x20, 0x3c0); /* disable blanking */

    if (depth == 8)
        vga_build_rgb_palette();
}

/* Low-level Forth accessor to update VGA color registers */

/* ( r g b index -- ) */
static
void vga_hw_set_color(void)
{
    int index = POP();
    int b = POP();
    int g = POP();
    int r = POP();

    vga_set_color(index, (r & 0xff),
        (g & 0xff),
        (b & 0xff));
}

void vga_vbe_init(const char *path, unsigned long fb, uint32_t fb_size,
                  unsigned long rom, uint32_t rom_size)
{
	phys_addr_t phys;
	phandle_t ph, chosen, aliases, options;
	char buf[6];
	int size;

	phys = fb;

#if defined(CONFIG_SPARC64)
	/* Fix virtual address on SPARC64 somewhere else */
	fb = 0xfe000000ULL;
#endif

	setup_video(phys, fb);
	vga_vbe_set_mode(VIDEO_DICT_VALUE(video.w),
			 VIDEO_DICT_VALUE(video.h),
			 VIDEO_DICT_VALUE(video.depth));

#if 0
    ph = find_dev(path);
#else
    ph = get_cur_dev();
#endif

	bind_func("hw-set-color", vga_hw_set_color);

	set_int_property(ph, "width", VIDEO_DICT_VALUE(video.w));
	set_int_property(ph, "height", VIDEO_DICT_VALUE(video.h));
	set_int_property(ph, "depth", VIDEO_DICT_VALUE(video.depth));
	set_int_property(ph, "linebytes", VIDEO_DICT_VALUE(video.rb));
	set_int_property(ph, "address", (u32)(fb & ~0x0000000F));

	molvideo_init();

	chosen = find_dev("/chosen");
	push_str(path);
	fword("open-dev");
	set_int_property(chosen, "display", POP());

	aliases = find_dev("/aliases");
	set_property(aliases, "screen", path, strlen(path) + 1);

	options = find_dev("/options");
	snprintf(buf, sizeof(buf), FMT_ucell, VIDEO_DICT_VALUE(video.w) / FONT_WIDTH);
	set_property(options, "screen-#columns", buf, strlen(buf) + 1);
	snprintf(buf, sizeof(buf), FMT_ucell, VIDEO_DICT_VALUE(video.h) / FONT_HEIGHT);
	set_property(options, "screen-#rows", buf, strlen(buf) + 1);

	if (rom_size >= 8) {
                const char *p;

                p = (const char *)rom;
		if (p[0] == 'N' && p[1] == 'D' && p[2] == 'R' && p[3] == 'V') {
			size = *(uint32_t*)(p + 4);
			set_property(ph, "driver,AAPL,MacOS,PowerPC",
				     p + 8, size);
		}
	}

#if defined(CONFIG_OFMEM) && defined(CONFIG_DRIVER_PCI)
        size = ((VIDEO_DICT_VALUE(video.h) * VIDEO_DICT_VALUE(video.rb))  + 0xfff) & ~0xfff;

	ofmem_claim_phys( video.mphys, size, 0 );
	ofmem_claim_virt( VIDEO_DICT_VALUE(video.mvirt), size, 0 );
	ofmem_map( video.mphys, VIDEO_DICT_VALUE(video.mvirt), size, ofmem_arch_io_translation_mode(video.mphys) );
#endif
}
