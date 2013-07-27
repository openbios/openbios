/*
 *   Creation Date: <2002/10/23 20:26:40 samuel>
 *   Time-stamp: <2004/01/07 19:39:15 samuel>
 *
 *     <video_common.c>
 *
 *     Shared video routines
 *
 *   Copyright (C) 2002, 2003, 2004 Samuel Rydh (samuel@ibrium.se)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation
 *
 */

#include "config.h"
#include "libopenbios/bindings.h"
#include "libopenbios/console.h"
#include "libopenbios/fontdata.h"
#include "libopenbios/ofmem.h"
#include "libopenbios/video.h"
#include "packages/video.h"
#include "drivers/vga.h"

struct video_info video;

unsigned long
video_get_color( int col_ind )
{
	unsigned long col;
	if( !video.has_video || col_ind < 0 || col_ind > 255 )
		return 0;
	if( video.depth == 8 )
		return col_ind;
	col = video.pal[col_ind];
	if( video.depth == 24 || video.depth == 32 )
		return col;
	if( video.depth == 15 )
		return ((col>>9) & 0x7c00) | ((col>>6) & 0x03e0) | ((col>>3) & 0x1f);
	return 0;
}

void
video_set_color( int ind, unsigned long color )
{
	if( !video.has_video || ind < 0 || ind > 255 )
		return;
	video.pal[ind] = color;

#ifdef CONFIG_MOL
	if( video.depth == 8 )
		OSI_SetColor( ind, color );
#elif defined(CONFIG_SPARC32)
#if defined(CONFIG_DEBUG_CONSOLE_VIDEO)
	if( video.depth == 8 ) {
            dac[0] = ind << 24;
            dac[1] = ((color >> 16) & 0xff) << 24; // Red
            dac[1] = ((color >> 8) & 0xff) << 24; // Green
            dac[1] = (color & 0xff) << 24; // Blue
        }
#endif
#else
	vga_set_color(ind, ((color >> 16) & 0xff),
			   ((color >> 8) & 0xff),
			   (color & 0xff));
#endif
}

int
video_get_res( int *w, int *h )
{
	if( !video.has_video ) {
		*w = *h = 0;
		return -1;
	}
	*w = video.w;
	*h = video.h;
	return 0;
}

/* ( fbaddr maskaddr width height fgcolor bgcolor -- ) */

void
video_mask_blit(void)
{
	ucell bgcolor = POP();
	ucell fgcolor = POP();
	ucell height = POP();
	ucell width = POP();
	unsigned char *mask = (unsigned char *)POP();
	unsigned char *fbaddr = (unsigned char *)POP();

	ucell color;
	unsigned char *dst, *rowdst;
	int x, y, m, b, d, depthbytes;

	fgcolor = video_get_color(fgcolor);
	bgcolor = video_get_color(bgcolor);
	d = video.depth;
	depthbytes = (video.depth + 1) >> 3;

	dst = fbaddr;
	for( y = 0; y < height; y++) {
		rowdst = dst;
		for( x = 0; x < (width + 1) >> 3; x++ ) {
			for (b = 0; b < 8; b++) {
				m = (1 << (7 - b));

				if (*mask & m) {
					color = fgcolor;
				} else {
					color = bgcolor;
				}

				if( d >= 24 )
					*((unsigned long*)dst) = color;
				else if( d >= 15 )
					*((short*)dst) = color;
				else
					*dst = color;

				dst += depthbytes;
			}
			mask++;
		}
		dst = rowdst;
		dst += video.rb;
	}
}

/* ( x y w h fgcolor bgcolor -- ) */

void
video_invert_rect( void )
{
	ucell bgcolor = POP();
	ucell fgcolor = POP();
	int h = POP();
	int w = POP();
	int y = POP();
	int x = POP();
	char *pp;

	bgcolor = video_get_color(bgcolor);
	fgcolor = video_get_color(fgcolor);

	if (!video.has_video || x < 0 || y < 0 || w <= 0 || h <= 0 ||
		x + w > video.w || y + h > video.h)
		return;

	pp = (char*)video.mvirt + video.rb * y;
	for( ; h--; pp += video.rb ) {
		int ww = w;
		if( video.depth == 24 || video.depth == 32 ) {
			unsigned long *p = (unsigned long*)pp + x;
			while( ww-- ) {
				if (*p == fgcolor) {
					*p++ = bgcolor;
				} else if (*p == bgcolor) {
					*p++ = fgcolor;
				}
			}
		} else if( video.depth == 16 || video.depth == 15 ) {
			unsigned short *p = (unsigned short*)pp + x;
			while( ww-- ) {
				if (*p == (unsigned short)fgcolor) {
					*p++ = bgcolor;
				} else if (*p == (unsigned short)bgcolor) {
					*p++ = fgcolor;
				}
			}
		} else {
			char *p = (char *)(pp + x);

			while( ww-- ) {
				if (*p == (char)fgcolor) {
					*p++ = bgcolor;
				} else if (*p == (char)bgcolor) {
					*p++ = fgcolor;
				}
			}
		}
	}
}

/* ( color_ind x y width height -- ) (?) */
void
video_fill_rect(void)
{
	int h = POP();
	int w = POP();
	int y = POP();
	int x = POP();
	int col_ind = POP();

	char *pp;
	unsigned long col = video_get_color(col_ind);

        if (!video.has_video || x < 0 || y < 0 || w <= 0 || h <= 0 ||
            x + w > video.w || y + h > video.h)
		return;

	pp = (char*)video.mvirt + video.rb * y;
	for( ; h--; pp += video.rb ) {
		int ww = w;
		if( video.depth == 24 || video.depth == 32 ) {
			unsigned long *p = (unsigned long*)pp + x;
			while( ww-- )
				*p++ = col;
		} else if( video.depth == 16 || video.depth == 15 ) {
			unsigned short *p = (unsigned short*)pp + x;
			while( ww-- )
				*p++ = col;
		} else {
                        char *p = (char *)(pp + x);

			while( ww-- )
				*p++ = col;
		}
	}
}

void
init_video( unsigned long fb, int width, int height, int depth, int rb )
{
        int i;
#if defined(CONFIG_OFMEM) && defined(CONFIG_DRIVER_PCI)
        int size;
#endif
	phandle_t ph=0, saved_ph=0;

	video.mphys = video.mvirt = fb;

#if defined(CONFIG_SPARC64)
	/* Fix virtual address on SPARC64 somewhere else */
	video.mvirt = 0xfe000000;
#endif

	video.w = width;
	video.h = height;
	video.depth = depth;
	video.rb = rb;

	saved_ph = get_cur_dev();
	while( (ph=dt_iterate_type(ph, "display")) ) {
		set_int_property( ph, "width", video.w );
		set_int_property( ph, "height", video.h );
		set_int_property( ph, "depth", video.depth );
		set_int_property( ph, "linebytes", video.rb );
		set_int_property( ph, "address", video.mvirt );

		activate_dev(ph);

		molvideo_init();
	}
	video.has_video = 1;
	video.pal = malloc( 256 * sizeof(unsigned long) );
	activate_dev(saved_ph);

	PUSH(video.mvirt);
	feval("to frame-buffer-adr");

	/* Set global variables ready for fb8-install */
	PUSH( pointer2cell(video_mask_blit) );
	fword("is-noname-cfunc");
	feval("to fb8-blitmask");
	PUSH( pointer2cell(video_fill_rect) );
	fword("is-noname-cfunc");
	feval("to fb8-fillrect");
	PUSH( pointer2cell(video_invert_rect) );
	fword("is-noname-cfunc");
	feval("to fb8-invertrect");

	PUSH((video.depth + 1) >> 3);
	feval("to depth-bytes");
	PUSH(video.rb);
	feval("to line-bytes");
	PUSH((ucell)fontdata);
	feval("to (romfont)");
	PUSH(FONT_HEIGHT);
	feval("to (romfont-height)");
	PUSH(FONT_WIDTH);
	feval("to (romfont-width)");
	PUSH(video.mvirt);
	feval("to qemu-video-addr");
	PUSH(video.w);
	feval("to qemu-video-width");
	PUSH(video.h);
	feval("to qemu-video-height");

#if defined(CONFIG_OFMEM) && defined(CONFIG_DRIVER_PCI)
        size = ((video.h * video.rb)  + 0xfff) & ~0xfff;

	ofmem_claim_phys( video.mphys, size, 0 );
	ofmem_claim_virt( video.mvirt, size, 0 );
	ofmem_map( video.mphys, video.mvirt, size, ofmem_arch_io_translation_mode(video.mphys) );
#endif

	for( i=0; i<256; i++ )
		video_set_color( i, i * 0x010101 );

	video_set_color( 254, 0xffffcc );
}
