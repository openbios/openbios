/*
 *   Creation Date: <1999/07/06 15:45:12 samuel>
 *   Time-stamp: <2002/10/20 16:31:48 samuel>
 *
 *	<partition_table.h>
 *
 *	Headers describing the partition table
 *
 *   Copyright (C) 1999, 2002 Samuel Rydh (samuel@ibrium.se)
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation
 *
 */

#ifndef _H_PARTITION_TABLE
#define _H_PARTITION_TABLE

/* This information is based upon IM vol V. */

#define DESC_MAP_SIGNATURE	0x4552

typedef struct {
	long		ddBlock;		/* first block of driver */
	short		ddSize;			/* driver size in blocks */
	short		ddType;			/* 1 & -1 for SCSI */
} driver_entry_t;

typedef struct { /* Block 0 of a device */
	short		sbSig;			/* always 0x4552 */
	short		sbBlockSize;		/* 512 */
	long		sbBlkCount;		/* #blocks on device */
	short		sbDevType;    		/* 0 */
	short		sbDevID;      		/* 0 */
	long		sbData;      		/* 0 */
	short		sbDrvrCount;		/* #driver descriptors */

	/* driver entries goes here */
	driver_entry_t	drivers[61] __attribute__ ((packed));

	short		filler1;
	long		filler2;
} desc_map_t;

typedef struct { /* Partition descriptor */
	short		pmSig;			/* always 0x504d 'PM' */
	short		pmSigPad;		/* 0 */
	ulong		pmMapBlkCnt;		/* #blocks in partition map */
	ulong		pmPyPartStart;		/* first physical block of part. */
	ulong		pmPartBlkCnt;		/* #blocks in partition */
	char		pmPartName[32];		/* partition name */
	char		pmPartType[32];		/* partition type */

	/* these fields may or may not be used */
	ulong		pmLgDataStart;
	ulong		pmDataCnt;
	ulong		pmPartStatus;
	ulong		pmLgBootStart;
	ulong		pmBootSize;
	ulong		pmBootLoad;
	ulong		pmBootLoad2;
	ulong		pmBootEntry;
	ulong		pmBootEntry2;
	ulong		pmBootCksum;
	char		pmProcessor[16];

	char		filler[376];		/* might contain extra information */
} part_entry_t;


#endif   /* _H_PARTITION_TABLE */
