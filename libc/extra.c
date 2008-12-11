/*
 *   Creation Date: <2003/10/18 13:52:32 samuel>
 *   Time-stamp: <2003/10/18 13:54:24 samuel>
 *
 *	<extra.c>
 *
 *	Libc extras
 *
 *   Copyright (C) 2003 Samuel Rydh (samuel@ibrium.se)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   version 2
 *
 */

#include "openbios/config.h"
#include "libc/string.h"

/* strncpy without 0-pad */
char *
strncpy_nopad( char *dest, const char *src, size_t n )
{
	int len = MIN( n, strlen(src)+1 );
	return memcpy( dest, src, len );
}
