/*
 *                     OpenBIOS - free your system! 
 *                         ( FCode tokenizer )
 *                          
 *  stream.c - source program streaming from file.
 *  
 *  This program is part of a free implementation of the IEEE 1275-1994 
 *  Standard for Boot (Initialization Configuration) Firmware.
 *
 *  Copyright (C) 2001-2005 by Stefan Reinauer <stepan@openbios.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA, 02110-1301 USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#ifdef __GLIBC__
#define __USE_XOPEN_EXTENDED
#endif
#include <string.h>
#include <sys/stat.h>

#include "toke.h"
#include "stream.h"

#define OUTPUT_SIZE	131072

extern bool offs16;
extern u16 nextfcode;

u8 *start, *pc, *end;
char *iname;

/* output pointers */
u8 *ostart, *opc, *oend, *oname;
static unsigned int ilen;

unsigned int lineno;

int init_stream( const char *name)
{
	FILE *infile;
	unsigned int i;
	
	struct stat finfo;
	
	if (stat(name,&finfo))
		return -1;
	
	ilen=finfo.st_size;
	start=malloc(ilen+1);
	if (!start)
		return -1;

        infile=fopen(name,"r");
        if (!infile)
                return -1;

	if (fread(start, ilen, 1, infile)!=1) {
		free(start);
		return -1;
	}

	fclose(infile);
	
	/* no zeroes within the file. */
	for (i=0; i<ilen; i++) {
		/* start[i]=start[i]?start[i]:0x0a; */
		start[i] |= (((signed char)start[i] - 1) >> 7) & 0x0a;
	}
	start[ilen]=0;
	
	pc=start; 
	end=pc+ilen;

	iname=strdup(name);
	
	lineno=1;
	
	return 0;
	
}

int init_output( const char *in_name, const char *out_name )
{
	const char *ext;
  	unsigned int len; /* should this be size_t? */

	/* preparing output */

	if( out_name )
		oname = (u8 *)strdup( out_name );
	else {
		ext=strrchr(in_name, '.');
		len=ext ? (unsigned int)(ext-in_name) : (unsigned int)strlen(in_name) ;
		oname=malloc(len+4);
		memcpy(oname, in_name, len);
		oname[len] = 0;
		strcat((char *)oname, ".fc");
	}
	
	/* output buffer size. this is 128k per default now, but we
	 * could reallocate if we run out. KISS for now.
	 */
	ostart=malloc(OUTPUT_SIZE);
	if (!ostart) {
		free(oname);
		free(start);
		return -1;
	}

	opc=oend=ostart;
	oend+=OUTPUT_SIZE;
	
	/* We're beginning a new output file, so we have to
	 * start our own fcode numbers at 0x800 again. 
	 */
	nextfcode=0x800;
	
	return 0;
}

int close_stream(void)
{
	free(start);
	free(iname);
	return 0;	
}

int close_output(void)
{
	FILE *outfile;
	unsigned int len;

	len=(unsigned long)opc-(unsigned long)ostart;
	
	outfile=fopen((char *)oname,"w");
	if (!outfile) {
		printf("toke: error opening output file.\n");
		return -1;
	}
	
	if(fwrite(ostart, len, 1, outfile)!=1)
		printf ("toke: error while writing output.\n");

	fclose(outfile);

	printf("toke: wrote %d bytes to bytecode file '%s'\n", len, oname);
	
	free(oname);
	return 0;
}

