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
