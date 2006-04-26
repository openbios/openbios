/*
 *                     OpenBIOS - free your system! 
 *                            ( detokenizer )
 *                          
 *  stream.c - FCode program bytecode streaming from file.
 *  
 *  This program is part of a free implementation of the IEEE 1275-1994 
 *  Standard for Boot (Initialization Configuration) Firmware.
 *
 *  Copyright (C) 2001-2005 Stefan Reinauer, <stepan@openbios.org>
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
#include <sys/stat.h>

#include "detok.h"

extern bool offs16;
extern u32  fcpos;

u16 fcode;
u8  inbyte;

static u8 *indata, *pc, *max;

int init_stream(char *name)
{
	FILE *infile;
	struct stat finfo;
	
	if (stat(name,&finfo))
		return -1;
	
	indata=malloc(finfo.st_size);
	if (!indata)
		return -1;

        infile=fopen(name,"r");
        if (!infile)
                return -1;

	if (fread(indata, finfo.st_size, 1, infile)!=1) {
		free(indata);
		return -1;
	}

	fclose(infile);
       
	pc=indata; 
	max=pc+finfo.st_size;
	
	return 0;
}

int close_stream(void)
{
	free(indata);
	return 0;	
}

int get_streampos(void)
{
	return (int)((long)pc-(long)indata);
}

void set_streampos(long pos)
{
	pc=indata+pos;
}

static int get_byte(void)
{
	inbyte=*pc;
	pc++;
	
        if (pc>max) {
                printf ("\nUnexpected end of file.\n");
                return 0;
        }

	return 1;
}
 
u16 get_token(void)
{
        u16 tok;
        get_byte();
        tok=inbyte;
        if (tok != 0x00 && tok < 0x10) {
                get_byte();
                tok<<=8;
                tok|=inbyte;
        }
        fcode=tok;
        return tok;
}

u32 get_num32(void)
{
        u32 ret;
 
        get_byte();
        ret=inbyte<<24;
        get_byte();
        ret|=(inbyte<<16);
        get_byte();
        ret|=(inbyte<<8);
        get_byte();
        ret|=inbyte;
 
        return ret;
}

u16 get_num16(void)
{
        u16 ret;
 
        get_byte();
        ret=inbyte<<8;
        get_byte();
        ret|=inbyte;
 
        return ret;
}
 
u8 get_num8(void)
{
        get_byte();
        return(inbyte);
}

u16 get_offset(void)
{
        if (offs16)
                return (get_num16());
 
        return (get_num8());
}


int scnt=0;
u8 *get_string(void)
{
        u8 *data;
        u8 size;
        unsigned int i;
 
        get_byte();
        size=inbyte;

	scnt++;

        data=malloc(size+2);
	if (!data) printf ("No more memory.\n");
        data[0]=size;
 
        for (i=1; i<size+1; i++) {
                get_byte();
                data[i]=inbyte;
        }
        data[i]=0;
        return data;
 
}

