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

#ifdef BOOTSTRAP
#include <stdint.h>
#else
typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;
typedef unsigned long long uint64_t;

typedef signed char     int8_t;
typedef short           int16_t;
typedef int             int32_t;
typedef long long       int64_t;
#endif

/* endianess */
#include "autoconf.h"

/* cell based types */
typedef int64_t   cell;
typedef uint64_t  ucell;

#define FMT_cell    "%lld"
#define FMT_ucell   "%llu"
#define FMT_ucellx  "%016llx"
#define FMT_ucellX  "%016llX"

#ifdef NEED_FAKE_INT128_T
typedef struct {
    uint64_t hi;
    uint64_t lo;
} blob_128_t;

typedef blob_128_t      dcell;
typedef blob_128_t     ducell;
#else
typedef __int128_t	dcell;
typedef __uint128_t    ducell;
#endif

#define bitspercell	(sizeof(cell)<<3)
#define bitsperdcell	(sizeof(dcell)<<3)

#define BITS		64

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
