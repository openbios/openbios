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
get_color( int col_ind )
{
	unsigned long col;
	if( !video.has_video || col_ind < 0 || col_ind > 255 )
		return 0;
	if( video.fb.depth == 8 )
		return col_ind;
	col = video.pal[col_ind];
	if( video.fb.depth == 24 || video.fb.depth == 32 )
		return col;
	if( video.fb.depth == 15 )
		return ((col>>9) & 0x7c00) | ((col>>6) & 0x03e0) | ((col>>3) & 0x1f);
	return 0;
}

void
set_color( int ind, unsigned long color )
{
	if( !video.has_video || ind < 0 || ind > 255 )
		return;
	video.pal[ind] = color;

#ifdef CONFIG_MOL
	if( video.fb.depth == 8 )
		OSI_SetColor( ind, color );
#elif defined(CONFIG_SPARC32)
#if defined(CONFIG_DEBUG_CONSOLE_VIDEO)
	if( video.fb.depth == 8 ) {
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

static void
startup_splash( void )
{
#ifdef CONFIG_MOL
	int fd, s, i, y, x, dx, dy;
	int width, height;
	char *pp, *p;
	char buf[64];
#endif

	/* only draw logo in 24-bit mode (for now) */
	if( video.fb.depth < 15 )
		return;
#ifdef CONFIG_MOL
	for( i=0; i<2; i++ ) {
		if( !BootHGetStrResInd("bootlogo", buf, sizeof(buf), 0, i) )
			return;
		*(!i ? &width : &height) = atol(buf);
	}

	if( (s=width * height * 3) > 0x20000 )
		return;

	if( (fd=open_io("pseudo:,bootlogo")) >= 0 ) {
		p = malloc( s );
		if( read_io(fd, p, s) != s )
			printk("bootlogo size error\n");
		close_io( fd );

		dx = (video.fb.w - width)/2;
		dy = (video.fb.h - height)/3;

		pp = (char*)video.fb.mvirt + dy * video.fb.rb + dx * (video.fb.depth >= 24 ? 4 : 2);

		for( y=0 ; y<height; y++, pp += video.fb.rb ) {
			if( video.fb.depth >= 24 ) {
				unsigned long *d = (unsigned long*)pp;
				for( x=0; x<width; x++, p+=3, d++ )
					*d = ((int)p[0] << 16) | ((int)p[1] << 8) | p[2];
			} else if( video.fb.depth == 15 ) {
				unsigned short *d = (unsigned short*)pp;
				for( x=0; x<width; x++, p+=3, d++ ) {
					int col = ((int)p[0] << 16) | ((int)p[1] << 8) | p[2];
					*d = ((col>>9) & 0x7c00) | ((col>>6) & 0x03e0) | ((col>>3) & 0x1f);
				}
			}
		}
		free( p );
	}
#else
	/* No bootlogo support yet on other platforms */
	return;
#endif
}

int
video_get_res( int *w, int *h )
{
	if( !video.has_video ) {
		*w = *h = 0;
		return -1;
	}
	*w = video.fb.w;
	*h = video.fb.h;
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

	fgcolor = get_color(fgcolor);
	bgcolor = get_color(bgcolor);
	d = video.fb.depth;
	depthbytes = (video.fb.depth + 1) >> 3;

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
		dst += video.fb.rb;
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

	bgcolor = get_color(bgcolor);
	fgcolor = get_color(fgcolor);

	if (!video.has_video || x < 0 || y < 0 || w <= 0 || h <= 0 ||
		x + w > video.fb.w || y + h > video.fb.h)
		return;

	pp = (char*)video.fb.mvirt + video.fb.rb * y;
	for( ; h--; pp += video.fb.rb ) {
		int ww = w;
		if( video.fb.depth == 24 || video.fb.depth == 32 ) {
			unsigned long *p = (unsigned long*)pp + x;
			while( ww-- ) {
				if (*p == fgcolor) {
					*p++ = bgcolor;
				} else if (*p == bgcolor) {
					*p++ = fgcolor;
				}
			}
		} else if( video.fb.depth == 16 || video.fb.depth == 15 ) {
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

void
draw_pixel( int x, int y, int colind )
{
	char *p = (char*)video.fb.mvirt + video.fb.rb * y;
	int color, d = video.fb.depth;

	if( x < 0 || y < 0 || x >= video.fb.w || y >=video.fb.h )
		return;
	color = get_color( colind );

	if( d >= 24 )
		*((unsigned long*)p + x) = color;
	else if( d >= 15 )
		*((short*)p + x) = color;
	else
		*(p + x) = color;
}

void
video_scroll( int height )
{
	int i, offs, size, *dest, *src;

        if (height <= 0 || height >= video.fb.h) {
                return;
        }
	offs = video.fb.rb * height;
	size = (video.fb.h * video.fb.rb - offs)/16;
	dest = (int*)video.fb.mvirt;
	src = (int*)(video.fb.mvirt + offs);

	for( i=0; i<size; i++ ) {
		dest[0] = src[0];
		dest[1] = src[1];
		dest[2] = src[2];
		dest[3] = src[3];
		dest += 4;
		src += 4;
	}
}

void
fill_rect( int col_ind, int x, int y, int w, int h )
{
	char *pp;
	unsigned long col = get_color(col_ind);

        if (!video.has_video || x < 0 || y < 0 || w <= 0 || h <= 0 ||
            x + w > video.fb.w || y + h > video.fb.h)
		return;

	pp = (char*)video.fb.mvirt + video.fb.rb * y;
	for( ; h--; pp += video.fb.rb ) {
		int ww = w;
		if( video.fb.depth == 24 || video.fb.depth == 32 ) {
			unsigned long *p = (unsigned long*)pp + x;
			while( ww-- )
				*p++ = col;
		} else if( video.fb.depth == 16 || video.fb.depth == 15 ) {
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
refresh_palette( void )
{
#ifdef CONFIG_MOL
	if( video.fb.depth == 8 )
		OSI_RefreshPalette();
#endif
}

static void
video_open(void)
{
	PUSH(-1);
}

/* ( addr len -- actual ) */
static void
video_write(void)
{
    char *addr;
    int len;

    len = POP();
    addr = (char *)cell2pointer(POP());

    console_draw_fstr(addr, len);
    PUSH(len);
}

void
init_video( unsigned long fb, int width, int height, int depth, int rb )
{
        int i;
#if defined(CONFIG_OFMEM) && defined(CONFIG_DRIVER_PCI)
        int size;
#endif
	phandle_t ph=0, saved_ph=0;

	video.fb.mphys = video.fb.mvirt = fb;

#if defined(CONFIG_SPARC64)
	/* Fix virtual address on SPARC64 somewhere else */
	video.fb.mvirt = 0xfe000000;
#endif

	video.fb.w = width;
	video.fb.h = height;
	video.fb.depth = depth;
	video.fb.rb = rb;

	saved_ph = get_cur_dev();
	while( (ph=dt_iterate_type(ph, "display")) ) {
		set_int_property( ph, "width", video.fb.w );
		set_int_property( ph, "height", video.fb.h );
		set_int_property( ph, "depth", video.fb.depth );
		set_int_property( ph, "linebytes", video.fb.rb );
		set_int_property( ph, "address", video.fb.mvirt );

		activate_dev(ph);

		if (!find_package_method("open", ph)) {
                        bind_func("open", video_open);
		}

		if (!find_package_method("write", ph)) {
                        bind_func("write", video_write);
		}

		molvideo_init();
	}
	video.has_video = 1;
	video.pal = malloc( 256 * sizeof(unsigned long) );
	activate_dev(saved_ph);

	PUSH(video.fb.mvirt);
	feval("to frame-buffer-adr");

	/* Set global variables ready for fb8-install */
	PUSH( pointer2cell(video_mask_blit) );
	fword("is-noname-cfunc");
	feval("to fb8-blitmask");
	PUSH( pointer2cell(video_invert_rect) );
	fword("is-noname-cfunc");
	feval("to fb8-invertrect");

	PUSH((video.fb.depth + 1) >> 3);
	feval("to depth-bytes");
	PUSH(video.fb.rb);
	feval("to line-bytes");
	PUSH((ucell)fontdata);
	feval("to (romfont)");
	PUSH(FONT_HEIGHT);
	feval("to (romfont-height)");
	PUSH(FONT_WIDTH);
	feval("to (romfont-width)");
	PUSH(video.fb.mvirt);
	feval("to qemu-video-addr");
	PUSH(video.fb.w);
	feval("to qemu-video-width");
	PUSH(video.fb.h);
	feval("to qemu-video-height");

#if defined(CONFIG_OFMEM) && defined(CONFIG_DRIVER_PCI)
        size = ((video.fb.h * video.fb.rb)  + 0xfff) & ~0xfff;

	ofmem_claim_phys( video.fb.mphys, size, 0 );
	ofmem_claim_virt( video.fb.mvirt, size, 0 );
	ofmem_map( video.fb.mphys, video.fb.mvirt, size, ofmem_arch_io_translation_mode(video.fb.mphys) );
#endif

	for( i=0; i<256; i++ )
		set_color( i, i * 0x010101 );

	set_color( 254, 0xffffcc );
	fill_rect( 254, 0, 0, video.fb.w, video.fb.h );

	refresh_palette();
	startup_splash();
}
