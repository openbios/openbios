/* 
 *	<console.c>
 *	
 *	Simple text console
 *	
 *   Copyright (C) 2002, 2003 Samuel Rydh (samuel@ibrium.se)
 *   Copyright (C) 2005 Stefan Reinauer <stepan@openbios.org>
 *   
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation
 *   
 */

#include "openbios/config.h"
#include "openbios/bindings.h"

#include "font_8x8.c"

#define FONT_ADJ_HEIGHT	 (FONT_HEIGHT + 2)
#define NCOLS	80
#define NROWS	48

static struct {
	int	inited;
	int	physw, physh;
	int	w,h;

	int	x,y;
	char	*buf;

	int	cursor_on;
} cons;

static int
get_conschar( int x, int y )
{
	if( (uint)x < cons.w && (uint)y < cons.h )
		return cons.buf[y*cons.w + x];
	return ' ';
}

static void
draw_char( uint h, uint v )
{
	char *c = fontdata;
	int x, y, xx, rskip, m;
	int invert = (h==cons.x && v==cons.y && cons.cursor_on);
	int ch = get_conschar( h, v );
	
	while( h >= cons.w || v >= cons.h )
		return;
	
	h *= FONT_WIDTH;
	v *= FONT_ADJ_HEIGHT;

	rskip = (FONT_WIDTH > 8)? 2 : 1;
	c += rskip * (unsigned int)(ch & 0xff) * FONT_HEIGHT;

	for( x=0; x<FONT_WIDTH; x++ ) {
		xx = x % 8;
		if( x && !xx )
			c++;
		m = (1<<(7-xx));
		for( y=0; y<FONT_HEIGHT; y++ ){
			int col = ((!(c[rskip*y] & m)) != invert) ? 254 : 0;
			draw_pixel( h+x, v+y+1, col );
		}
		draw_pixel( h+x, v, 254 );
		draw_pixel( h+x, v+FONT_HEIGHT+1, 254 );
	}
}

static void
show_cursor( int show )
{
	if( cons.cursor_on == show )
		return;
	cons.cursor_on = show;
	draw_char( cons.x, cons.y );
}


static void
draw_line( int n )
{
	int i;

	if( n >= cons.h || n < 0 )
		return;
	for( i=0; i<cons.w; i++ )
		draw_char( i, n );
}

#if 0
static void
refresh( void )
{
	int i;       
	for( i=0; i<cons.h; i++ )
		draw_line(i);
}
#endif

static int
console_init( void ) 
{
	if( video_get_res(&cons.physw,&cons.physh) < 0 )
		return -1;

	set_color( 0, 0 );
	
	cons.w = cons.physw/FONT_WIDTH;
	cons.h = cons.physh/FONT_ADJ_HEIGHT;
	cons.buf = malloc( cons.w * cons.h );
	cons.inited = 1;	
	cons.x = cons.y = 0;
	return 0;
}

void
console_close( void ) 
{
 	if( !cons.inited )
		return; 
	free( cons.buf );
	cons.inited = 0;	
}

static void
rec_char( int ch, int x, int y )
{
	if( (uint)x < cons.w && (uint)y < cons.h ) {
		cons.buf[y*cons.w + x] = ch;
		draw_char( x, y );
	}
}

static void
scroll1( void )
{
	osi_fb_info_t fb;
	int i, x, offs, size, *dest, *src;
	
	openbios_GetFBInfo( &fb );

	offs = fb.rb * FONT_ADJ_HEIGHT;
	size = (fb.h * fb.rb - offs)/16;
	dest = (int*)fb.mphys;
	src = (int*)(fb.mphys + offs);

	for( i=0; i<size; i++ ) {
		dest[0] = src[0];
		dest[1] = src[1];
		dest[2] = src[2];
		dest[3] = src[3];
		dest += 4;
		src += 4;
	}
	for( x=0; x<cons.w; x++ )
		cons.buf[(cons.h-1)*cons.w + x] = 0;
	draw_line(cons.h-1);
}

int
console_draw_str( const char *str )
{
	static const char *ignore[] = { "[1;37m", "[2;40m", NULL };
	int ch, y, x, i;

	if( !cons.inited && console_init() )
		return -1;

	show_cursor(0);
	while( (ch=*str++) ) {
		if( ch == 12 )
			continue;
		if( ch == '\n' || cons.x >= cons.w ) {
			cons.x=0, cons.y++;
			continue;
		}
		if( cons.y >= cons.h ) {
			for( y=0; y<cons.h-1; y++ )
				for( x=0; x<cons.w; x++ )
					cons.buf[y*cons.w + x] = cons.buf[(y+1)*cons.w + x];
			cons.y = cons.h-1;
			cons.x = 0;
			scroll1();
		}
		if( ch == '\r' )
			continue;
		if( ch == '\e' ) {
			for( i=0; ignore[i] && strncmp(ignore[i], str, strlen(ignore[i])); i++ )
				;
			if( ignore[i] )
				str += strlen(ignore[i]);
			continue;
		}
		if( ch == 8 ) {
			if( cons.x )
				cons.x--;
			continue;
		}
		rec_char( ch, cons.x++, cons.y );
	}
	show_cursor(1);
	return 0;
}
