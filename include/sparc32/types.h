/* tag: data types for forth engine
 *
 * Copyright (C) 2003-2005 Patrick Mauritz, Stefan Reinauer
 *
 * See the file "COPYING" for further information about
 * the copyright and warranty status of this work.
 */

#ifndef __TYPES_H
#define __TYPES_H

#include "mconfig.h"

#include <stdint.h>

/* endianess */
#ifdef HAVE_ENDIAN_H
#include <endian.h>
#else
#include <machine/endian.h>
#endif

/* cell based types */

typedef int32_t		cell;
typedef uint32_t	ucell;
typedef int64_t		dcell;
typedef uint64_t	ducell;

#define bitspercell	(sizeof(cell)<<3)
#define bitsperdcell	(sizeof(dcell)<<3)

#define BITS		32

/* size named types */

typedef unsigned char   u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long long u64;

typedef signed char	s8;
typedef short		s16;
typedef int		s32;
typedef long long	s64;

#endif
