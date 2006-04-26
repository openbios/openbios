/*
 * Xbox framebuffer - Video + Console
 *
 * Copyright (C) 2005 Ed Schouten <ed@fxq.nl>, 
 * 		      Stefan Reinauer <stepan@openbios.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation
 */


#include "openbios/config.h"
#include "openbios/bindings.h"
#include "libc/diskio.h"

typedef struct osi_fb_info {
	unsigned long	mphys;
	int		rb, w, h, depth;
} osi_fb_info_t;

int Xbox_GetFBInfo (osi_fb_info_t *fb)
{
	fb->w = 640;
	fb->h = 480;
	fb->depth = 32;
	fb->rb = fb->w * 4; /* rgb + alpha */
	fb->mphys = phys_to_virt(0x3C00000); /* 60M - 64M */

	return 0;
}

#define openbios_GetFBInfo(x) Xbox_GetFBInfo(x)

#include "../../../modules/video.c"
#include "../../../modules/console.c"



