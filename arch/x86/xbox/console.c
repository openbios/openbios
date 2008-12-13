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

#include "../../../modules/video.c"
#include "../../../modules/console.c"
