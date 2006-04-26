/*
 *                     OpenBIOS - free your system! 
 *                            ( detokenizer )
 *                          
 *  decode.c - contains output wrappers for fcode words.  
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
#include <ctype.h>

#include "detok.h"
#include "stream.h"

/* Dictionary function prototypes */
char *lookup_token(u16 number);
int  add_token(u16 number, char *name);

extern u16 fcode;

bool offs16=TRUE;
int indent, verbose=0, decode_all=0, linenumbers=0, linenum;
u32 fclen;

static u8 *unnamed=(u8 *)"(unnamed-fcode)";

void pretty_string(u8 *string)
{
	u8 c;
	unsigned int i;
	bool qopen=TRUE;
	
	printf("\" ");

	for (i=1; i<1+(unsigned int)string[0]; i++) {
		c=string[i];
		if (isprint(c)) {
			if (!qopen) {
				printf(" )");
				qopen=TRUE;
			}
			printf("%c",c);
		} else {
			if (qopen) {
				printf("\"(");
				qopen=FALSE;
			}
			printf(" %02x",c);
		}
	}
	if (!qopen)
		printf(" )");
	printf("\"");
}

static void decode_indent(void) 
{
	int i;
#ifdef DEBUG_INDENT
	if (indent<0) {
		printf("detok: error in indentation code.\n");
		indent=0;
	}
#endif
	for (i=0; i<indent; i++)
		printf ("    ");
}

static void decode_lines(void)
{
	if (linenumbers) {
		printf("%6d: ",linenumbers==1?linenum:get_streampos());
		linenum++;
	}
}

static void output_token(void)
{
	u8 *tname=lookup_token(fcode);
	
	printf ("%s ", tname);

	/* The fcode number is interesting if
	 *  a) detok is in verbose mode
	 *  b) the token has no name.
	 */
	if (verbose || tname == unnamed)
		printf("[0x%03x] ", fcode);
}

static s16 decode_offset(void)
{
        s16 offs;

        offs=get_offset();
	output_token();
	printf("0x%x\n",offs);
	return offs;
}

static void decode_default(void)
{
	output_token();
	printf ("\n");
}

static void new_token(void)
{
	u16 token;
	output_token();
	token=get_token();
	printf("0x%03x\n",token);
	add_token(token, unnamed);
}

static void named_token(void)
{
	u16 token;
	u8* string;

	output_token();
	/* get forth string ( [len] [char0] ... [charn] ) */
	string=get_string();
	token=get_token();

	printf("%s 0x%03x\n", string+1, token);
	
	add_token(token,string+1);
}

static void external_token(void)
{
	u16 token;
	u8* string;

	output_token();
	/* get forth string ( [len] [char0] ... [charn] ) */
	string=get_string();
	token=get_token();

	printf("%s 0x%03x\n", string+1, token);

	add_token(token,string+1);
}

static void bquote(void)
{
	u8 *string;

	/* get forth string ( [len] [char0] ... [charn] ) */
	string=get_string();
	output_token();
	pretty_string(string);
	printf("\n");
	free(string);
}

static void blit(void)
{
	u32 lit;

	lit=get_num32();
	output_token();
	printf("0x%x\n",lit);
}

static void offset16(void)
{
	decode_default();
	offs16=TRUE;
}

static void decode_branch(void)
{
	s16 offs;
	offs=decode_offset();
	if (offs>=0)
		indent++;
	else
		indent--;
}

static void decode_two(void)
{
	u16 token;

	output_token();
	token=get_token();
	decode_default();
}

static void decode_start(void)
{
	u8  fcformat;
	u16 fcchecksum, checksum=0;
	long pos;
	u32 i;

	decode_default();
	
	decode_lines();
        fcformat=get_num8();
	printf("  format:    0x%02x\n", fcformat);
 
	decode_lines();
        fcchecksum=get_num16();
        /* missing: check for checksum correctness. */
 
        fclen=get_num32(); /* skip len */
	pos=get_streampos();
	
	for (i=0; i<fclen-pos; i++)
		checksum+=get_num8();
	
	set_streampos(pos-4);
	
	printf("  checksum:  0x%04x (%sOk)\n", fcchecksum,
			fcchecksum==checksum?"":"not ");

	decode_lines();
	fclen=get_num32();
        printf("  len:       0x%x (%d bytes)\n", fclen, fclen);
}


void decode_token(u16 token)
{
	switch (token) {
	case 0x0b5:
		new_token();
		break;
	case 0x0b6:
		named_token();
		break;
	case 0x0ca:
		external_token();
		break;
	case 0x012:
		bquote();
		break;
	case 0x010:
		blit();
		break;
	case 0x0cc:
		offset16();
		break;
	case 0x013: /* bbranch */
	case 0x014: /* b?branch */
		decode_branch();
		break;
	case 0x0b7: /* b(:) */
	case 0x0b1: /* b(<mark) */
	case 0x0c4: /* b(case) */
		decode_default();
		indent++;
		break;
	case 0x0c2: /* b(;) */
	case 0x0b2: /* b(>resolve) */
	case 0x0c5: /* b(endcase) */
		decode_default();
		indent--;
		break;
	case 0x015: /* b(loop) */
	case 0x016: /* b(+loop) */
	case 0x0c6: /* b(endof) */
		decode_offset();
		indent--;
		break;
	case 0x017: /* b(do) */
	case 0x018: /* b/?do) */
	case 0x01c: /* b(of) */
		decode_offset();
		indent++;
		break;
	case 0x011: /* b(') */
	case 0x0c3: /* b(to) */
		decode_two();
		break;
	case 0x0f0: /* start0 */
	case 0x0f1: /* start1 */
	case 0x0f2: /* start2 */
	case 0x0f3: /* start4 */
		decode_start();
		break;
	case 0x0fd: /* version1 */
		decode_start();
		offs16=FALSE;
		break;
	default:
		decode_default();
	}
}

int detokenize(void)
{
	u16 token;
	
	if (linenumbers)
		linenum=1;
	
	do {
		decode_lines();
		decode_indent();
		token=get_token();
		decode_token(token);
	} while ((token||decode_all) && ((get_streampos()-1)<=fclen));

	decode_lines();
	printf ("\\ detokenizing finished after %d of %d bytes.\n",
			get_streampos(), fclen );

	return 0;
}
