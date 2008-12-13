/*
 *   Creation Date: <2002/10/29 18:59:05 samuel>
 *   Time-stamp: <2003/12/28 22:51:11 samuel>
 *
 *	<console.c>
 *
 *	Simple text console
 *
 *   Copyright (C) 2002, 2003, 2004 Samuel Rydh (samuel@ibrium.se)
 *   Copyright (C) 2005 Stefan Reinauer <stepan@openbios.org>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation
 *
 */

#include "openbios/config.h"
#include "openbios/bindings.h"
#include "libc/diskio.h"
#include "osi_calls.h"
#include "ofmem.h"
#include "mol/mol.h"
#include "boothelper_sh.h"
#include "video_sh.h"

#define openbios_GetFBInfo(x) OSI_GetFBInfo(x)

#include "../../../modules/video.c"
#include "../../../modules/console.c"
