/*
 *   Creation Date: <2002/10/23 20:26:40 samuel>
 *   Time-stamp: <2004/01/07 19:39:15 samuel>
 *
 *	<video.c>
 *
 *	Mac-on-Linux display node
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
#include "libc/diskio.h"
#include "libopenbios/ofmem.h"
#include "drivers/drivers.h"
#include "packages/video.h"
#include "libopenbios/video.h"
#include "drivers/vga.h"


/************************************************************************/
/*	OF methods							*/
/************************************************************************/

DECLARE_NODE( video, 0, 0, "Tdisplay" );

#ifdef CONFIG_MOL
static void
molvideo_refresh_palette( void )
{

	if( VIDEO_DICT_VALUE(video.depth) == 8 )
		OSI_RefreshPalette();
}

static void
molvideo_hw_set_color( void )
{

	if( VIDEO_DICT_VALUE(video.depth) == 8 )
		OSI_SetColor( ind, color );
}
#endif

/* ( -- width height ) (?) */
static void
molvideo_dimensions( void )
{
	fword("screen-width");
	fword("screen-height");
}

/* ( table start count -- ) (?) */
static void
molvideo_set_colors( void )
{
	int count = POP();
	int start = POP();
	unsigned char *p = (unsigned char*)cell2pointer(POP());
	int i;

	for( i=0; i<count; i++, p+=3 ) {
		unsigned long col = (p[0] << 16) | (p[1] << 8) | p[2];
		video_set_color( i + start, col );
	}
}

/* ( r g b index -- ) */
static void
molvideo_color_bang( void )
{
	int index = POP();
	int b = POP();
	int g = POP();
	int r = POP();
	unsigned long col = ((r << 16) & 0xff0000) | ((g << 8) & 0x00ff00) | (b & 0xff);
	/* printk("color!: %08lx %08lx %08lx %08lx\n", r, g, b, index ); */
	video_set_color( index, col );
}

/* ( color_ind x y width height -- ) (?) */
static void
molvideo_fill_rect ( void )
{
	video_fill_rect();
}

/* ( -- ) - really should be reworked as draw-logo */
static void
molvideo_startup_splash( void )
{
#ifdef CONFIG_MOL
	int fd, s, i, y, x, dx, dy;
	int width, height;
	char *pp, *p;
	char buf[64];
#endif

	/* only draw logo in 24-bit mode (for now) */
	if( VIDEO_DICT_VALUE(video.depth) < 15 )
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

		dx = (VIDEO_DICT_VALUE(video.w) - width)/2;
		dy = (VIDEO_DICT_VALUE(video.h) - height)/3;

		pp = (char*)VIDEO_DICT_VALUE(video.mvirt) + dy * VIDEO_DICT_VALUE(video.rb) + dx * (VIDEO_DICT_VALUE(video.depth) >= 24 ? 4 : 2);

		for( y=0 ; y<height; y++, pp += VIDEO_DICT_VALUE(video.rb) ) {
			if( VIDEO_DICT_VALUE(video.depth) >= 24 ) {
				unsigned long *d = (unsigned long*)pp;
				for( x=0; x<width; x++, p+=3, d++ )
					*d = ((int)p[0] << 16) | ((int)p[1] << 8) | p[2];
			} else if( VIDEO_DICT_VALUE(video.depth) == 15 ) {
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


NODE_METHODS( video ) = {
#ifdef CONFIG_MOL
	{"hw-set-color",	molvideo_hw_set_color   },
	{"hw-refresh-palette",	molvideo_refresh_palette},
#endif
	{"dimensions",		molvideo_dimensions	},
	{"set-colors",		molvideo_set_colors	},
	{"fill-rectangle",	molvideo_fill_rect	},
	{"color!",		molvideo_color_bang	},
	{"startup-splash",	molvideo_startup_splash },
};


/************************************************************************/
/*	init 								*/
/************************************************************************/

void
molvideo_init(void)
{
	REGISTER_NODE( video );
}
