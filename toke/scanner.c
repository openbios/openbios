/*
 *                     OpenBIOS - free your system! 
 *                         ( FCode tokenizer )
 *                          
 *  scanner.c - simple scanner for forth files.
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
#include <unistd.h>
#ifdef __GLIBC__
#define __USE_XOPEN_EXTENDED
#endif
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "toke.h"
#include "stack.h"
#include "stream.h"
#include "emit.h"
#include "dictionary.h"

#define ERROR    do { if (!noerrors) exit(-1);  } while (0)

extern u8 *start, *pc, *end, *opc, *ostart;
extern int verbose, noerrors;

u8   *statbuf=NULL;
u16  nextfcode;
u8   base=0x0a;

/* header pointers */
u8  *fcode_hdr=NULL;
u8  *pci_hdr=NULL;

/* pci data */
bool pci_is_last_image=TRUE;
bool pci_want_header=FALSE;
u16  pci_revision=0x0001;
u16  pci_vpd=0x0000;

bool offs16=TRUE;
static bool intok=FALSE, incolon=FALSE;
bool haveend=FALSE;

static u8 *skipws(u8 *str)
{
	while (str && (*str=='\t' || *str==' ' || *str=='\n' )) {
		if (*str=='\n') 
			lineno++;
		str++;
	}
	
	return str;
}

static u8 *firstchar(u8 needle, u8 *str)
{
	while (str && *str!=needle) {
		if (*str=='\n') 
			lineno++;
		str++;
	}

	return str;
}

static unsigned long get_word(void)
{
	size_t len;
	u8 *str;

	pc=skipws(pc);
	if (pc>=end)
		return 0;

	str=pc;
	while (str && *str && *str!='\n' && *str!='\t' && *str!=' ')
		str++;

	len=(size_t)(str-pc);
	if (len>1023) {
		printf("%s:%d: error: buffer overflow.\n", iname, lineno);
		ERROR;
	}

	memcpy(statbuf, pc, len); 
	statbuf[len]=0;

#ifdef DEBUG_SCANNER
	printf("%s:%d: debug: read token '%s', length=%ld\n",
			iname, lineno, statbuf, len);
#endif
	pc+=len;
	return len;
}

static unsigned long get_until(char needle)
{                                                                               
	u8 *safe;                                                         
	unsigned long len;                                                      

	safe=pc;
	pc=firstchar(needle,safe);
	if (pc>=end)
		return 0;

	len=(unsigned long)pc-(unsigned long)safe;
	if (len>1023) {
		printf("%s:%d: error: buffer overflow\n", iname, lineno);
		ERROR;
	}
	
	memcpy(statbuf, safe, len);
	statbuf[len]=0;
	return len;
}

static long parse_number(u8 *start, u8 **endptr, int lbase) 
{
	long val = 0;
	int negative = 0, curr;
	u8 *nptr=start;

	curr = *nptr;
	if (curr == '-') {
		negative=1;
		nptr++;
	}
	
	for (curr = *nptr; (curr = *nptr); nptr++) {
		if ( curr == '.' )
			continue;
		if ( curr >= '0' && curr <= '9')
			curr -= '0';
		else if (curr >= 'a' && curr <= 'f')
			curr += 10 - 'a';
		else if (curr >= 'A' && curr <= 'F')
			curr += 10 - 'A';
		else
			break;
		
		if (curr >= lbase)
			break;
		
		val *= lbase;
		val += curr;
	}

#ifdef DEBUG_SCANNER
	if (curr)
		printf( "%s:%d: warning: couldn't parse number '%s' (%d/%d)\n",
				iname, lineno, start,curr,lbase);
#endif

	if (endptr)
		*endptr=nptr;

	if (negative)
		return -val;
	return val;
}

static u8 *get_sequence(u8 *walk)
{
	u8 val, pval[3];
	
	pc++; /* skip the ( */
#ifdef DEBUG_SCANNER
	printf("%s:%d: debug: hex field:", iname, lineno);
#endif
	pval[1]=0; pval[2]=0;

	for(;;) {
		while (!isxdigit(*pc) && (*pc) != ')')
			pc++;

		pval[0]=*pc;
		if (pval[0]==')')
			break;

		pc++; /* this cannot be a \n */

		pval[1]=*pc;
		if ( *pc!=')' && *pc++=='\n' )
			lineno++;
				
		val=parse_number(pval, NULL, 16);
		*(walk++)=val;
#ifdef DEBUG_SCANNER
		printf(" %02x",val);
#endif
	}
#ifdef DEBUG_SCANNER
	printf("\n");
#endif

	return walk;
}

static unsigned long get_string(void)
{
	u8 *walk;
	unsigned long len;
	bool run=1;
	u8 c, val;
	
	walk=statbuf;
	while (run) {
		switch ((c=*pc)) {
		case '\\':
			switch ((c=*(++pc))) {
			case 'n':
				/* newline */
				*(walk++)='\n';
				break;
			case 't':
				/* tab */
				*(walk++)='\t';
				break;
			default:
				val=parse_number(pc, &pc, base);
#ifdef DEBUG_SCANNER
				if (verbose)
					printf( "%s:%d: debug: escape code "
						"0x%x\n",iname, lineno, val);
#endif
				*(walk++)=val;
			}
			break;
		case '\"':
			pc++; /* skip the " */

			/* printf("switching: %c\n",*pc); */
			switch(*pc) {
			case '(':
				walk=get_sequence(walk);
				break;
			case '"':
				*(walk++)='"';
				pc++;
				break;
			case 'n':
				*(walk++)='\n';
				pc++;
				break;
			case 'r':
				*(walk++)='\r';
				pc++;
				break;
			case 't':
				*(walk++)='\t';
				pc++;
				break;
			case 'f':
				*(walk++)='\f';
				pc++;
				break;
			case 'l':
				*(walk++)='\n';
				pc++;
				break;
			case 'b':
				*(walk++)=0x08;
				pc++;
				break;
			case '!':
				*(walk++)=0x07;
				pc++;
				break;
			case '^':
				pc++;
				c=toupper(*(pc++));
				*(walk++)=c-'A';
				break;
			case '\n':
				lineno++;
			case ' ':
			case '\t':
				run=0;
				break;
			default:
				*(walk++)=*(pc++);
				break;
			}
			break;
		default:
			*(walk++)=c;
		}
		if (*pc++=='\n') lineno++;
	}
	
	*(walk++)=0;
	
	if (pc>=end)
		return 0;
	
	len=(unsigned long)walk-(unsigned long)statbuf;
	if (len>1023) {
		printf("%s:%d: error: buffer overflow\n", iname, lineno);
		ERROR;
	}
#ifdef DEBUG_SCANNER
	if (verbose)
		printf("%s:%d: debug: scanned string: '%s'\n", 
					iname, lineno, statbuf);
#endif

	return len>255?255:len;
}

static int get_number(long *result)
{
	u8 lbase, *until;
	long val;

	lbase=intok?0x10:base;
	val=parse_number(statbuf,&until, lbase);
	
#ifdef DEBUG_SCANNER
	printf("%s:%d: debug: parsing number: base 0x%x, val 0x%lx, "
			"processed %ld of %ld bytes\n", iname, lineno, 
			lbase, val,(size_t)(until-statbuf), strlen((char *)statbuf));
#endif

	if (until==(statbuf+strlen((char *)statbuf))) {
		*result=val;
		return 0;
	}
		
	return -1;
}

void init_scanner(void)
{
	statbuf=malloc(1024);
	if (!statbuf) {
		printf ("no memory.\n");
		exit(-1);
	}
}

void exit_scanner(void)
{
	free(statbuf);
}

#define FLAG_EXTERNAL 0x01
#define FLAG_HEADERS  0x02
char *name, *alias;
int flags=0;

static int create_word(void)
{
	unsigned long wlen;

	if (incolon) {
		printf("%s:%d: error: creating words not allowed "
			"in colon definition.\n", iname, lineno);
		ERROR;
	}
	
	if(nextfcode > 0xfff) {
		printf("%s:%d: error: maximum number of fcode words "
			"reached.\n", iname, lineno);
		ERROR;
	}
	
	wlen=get_word();
	name=strdup((char *)statbuf);

#ifdef DEBUG_SCANNER
	printf("%s:%d: debug: defined new word %s, fcode no 0x%x\n",
			iname, lineno, name, nextfcode);
#endif
	add_token(nextfcode, name);
	if (flags) {
		if (flags&FLAG_EXTERNAL)
			emit_token("external-token");
		else
			emit_token("named-token");
		emit_string((u8 *)name, wlen);
	} else
		emit_token("new-token");
	
	emit_fcode(nextfcode);
	nextfcode++;

	return 0;
}

static void encode_file( const char *filename )
{
	FILE *f;
	size_t s;
	int i=0;
	
	if( !(f=fopen(filename,"rb")) ) {
		printf("%s:%d: opening '%s':\n", iname, lineno, filename );
		ERROR;
		return;
	}
	while( (s=fread(statbuf, 1, 255, f)) ) {
		emit_token("b(\")");
		emit_string(statbuf, s);
		emit_token("encode-bytes");
		if( i++ )
			emit_token("encode+");
	}
	fclose( f );
}


static void handle_internal(u16 tok)
{
	unsigned long wlen;
	long offs1,offs2;
	u16 itok;
	
#ifdef DEBUG_SCANNER
	printf("%s:%d: debug: tokenizing control word '%s'\n",
						iname, lineno, statbuf);
#endif
	switch (tok) {
	case BEGIN:
		emit_token("b(<mark)");
		dpush((unsigned long)opc);
		break;

	case BUFFER:
		create_word();
		emit_token("b(buffer:)");
		break;

	case CONST:
		create_word();
		emit_token("b(constant)");
		break;

	case COLON:
		create_word();
		emit_token("b(:)");
		incolon=TRUE;
		break;
	
	case SEMICOLON:
		emit_token("b(;)");
		incolon=FALSE;
		break;

	case CREATE:
		create_word();
		emit_token("b(create)");
		break;

	case DEFER:
		create_word();
		emit_token("b(defer)");
		break;

	case FIELD:
		create_word();
		emit_token("b(field)");
		break;

	case VALUE:
		create_word();
		emit_token("b(value)");
		break;
		
	case VARIABLE:
		create_word();
		emit_token("b(variable)");
		break;

	case EXTERNAL:
		flags=FLAG_EXTERNAL;
		break;

	case TOKENIZE:
		emit_token("b(')");
		break;

	case AGAIN:
		emit_token("bbranch");
		offs1=dpop()-(unsigned long)opc;
		emit_offset(offs1);
		break;

	case ALIAS:
		wlen=get_word();
		name=strdup((char *)statbuf);
		wlen=get_word();
		alias=strdup((char *)statbuf);
		if(lookup_macro(name))
			printf("%s:%d: warning: duplicate alias\n", 
							iname, lineno);
		add_macro(name,alias);
		break;

	case CONTROL:
		wlen=get_word();
		emit_token("b(lit)");
		emit_num32(statbuf[0]&0x1f);
		break;

	case DO:
		emit_token("b(do)");
		dpush((unsigned long)opc);
		emit_offset(0);
		dpush((unsigned long)opc);
		break;

	case CDO:
		emit_token("b(?do)");
		dpush((unsigned long)opc);
		emit_offset(0);
		dpush((unsigned long)opc);
		break;

	case ELSE:
		offs2=dpop();
		emit_token("bbranch");
		dpush((unsigned long)opc);
		emit_offset(0);
		emit_token("b(>resolve)");
		offs1=(unsigned long)opc;
		opc=(u8 *)offs2;
		offs2=offs1-(unsigned long)opc;
		emit_offset(offs2);
		opc=(u8 *)offs1;
		break;

	case CASE:
		emit_token("b(case)");
		dpush(0);
		break;

	case ENDCASE:
		/* first emit endcase, then calculate offsets. */
		emit_token("b(endcase)");
		
		offs1=(unsigned long)opc;
		
		offs2=dpop();
		while (offs2) {
			u16 tmp;
			
			opc=(u8 *)(ostart+offs2);

			/* we're using an fcode offset to temporarily
			 * store our linked list. At this point we take
			 * the signed fcode offset as unsigned as we know
			 * that the offset will always be >0. This code
			 * is still broken for any case statement that
			 * occurs after the fcode bytecode grows larger 
			 * than 64kB
			 */
			tmp=receive_offset();

			offs2=(unsigned long)offs1-(unsigned long)opc;

#if defined(DEBUG_SCANNER)
			printf ("%s:%d: debug: endcase offset 0x%lx\n",
				iname,lineno, (unsigned long)offs2);
#endif
			emit_offset(offs2);
			offs2=tmp;
		}
		opc=(u8 *)offs1;
		break;

	case OF:
		emit_token("b(of)");
		dpush((unsigned long)(opc-ostart));
		emit_offset(0);
		break;

	case ENDOF:
		offs1=dpop();
		emit_token("b(endof)");
		
		offs2=dpop();
		dpush((unsigned long)(opc-ostart));
		emit_offset(offs2);
		
		offs2=(unsigned long)(opc-ostart);
		opc=(u8 *)(ostart+offs1);
		offs1=offs2-offs1;
		emit_offset(offs1);
		
		opc=(u8 *)(ostart+offs2);
		break;
		
	case HEADERLESS:
		flags=0;
		break;
	
	case HEADERS:
		flags=FLAG_HEADERS;
		break;

	case DECIMAL:
		/* in a definition this is expanded as macro "10 base !" */
		if (incolon) {
			emit_token("b(lit)");
			emit_num32(0x0a);
			emit_token("base");
			emit_token("!");
		} else
			base=10;
		break;
		
	case HEX:
		if (incolon) {
			emit_token("b(lit)");
			emit_num32(0x10);
			emit_token("base");
			emit_token("!");
		} else
			base=16;
		break;

	case OCTAL:
		if (incolon) {
			emit_token("b(lit)");
			emit_num32(0x08);
			emit_token("base");
			emit_token("!");
		} else
			base=8;
		break;

	case OFFSET16:
		if (!offs16)
			printf("%s:%d: switching to "
				"16bit offsets.\n", iname, lineno);
		emit_token("offset16");
		offs16=TRUE;
		break;

	case IF:
		emit_token("b?branch");
		dpush((unsigned long)opc);
		emit_offset(0);
		break;

	case CLEAVE:
	case LEAVE:
		emit_token("b(leave)");
		break;
		
	case LOOP:
		emit_token("b(loop)");
		offs1=dpop();
		offs2=offs1-(unsigned long)opc;
		emit_offset(offs2);
		offs1=(unsigned long)opc;
		opc=(u8 *)dpop();
		offs2=offs1-(unsigned long)opc;
		emit_offset(offs2);
		opc=(u8 *)offs1;
		break;
		
	case CLOOP:
		emit_token("b(+loop)");
		offs1=dpop();
		offs2=offs1-(unsigned long)opc;
		emit_offset(offs2);
		offs1=(unsigned long)opc;
		opc=(u8 *)dpop();
		offs2=offs1-(unsigned long)opc;
		emit_offset(offs2);
		opc=(u8 *)offs1;
		break;

	case GETTOKEN:
		emit_token("b(')");
		wlen=get_word();
		itok=lookup_token((char *)statbuf);
		if (itok==0xffff) {
			printf("%s:%d: error: no such word '%s' in [']\n",
					iname, lineno, statbuf);
			ERROR;
		}
		/* FIXME check size, u16 or token */
		emit_fcode(itok);
		break;
		
	case ASCII:
		wlen=get_word();
		emit_token("b(lit)");
		emit_num32(statbuf[0]);
		break;

	case CHAR:
		if (incolon)
			printf("%s:%d: warning: CHAR cannot be used inside "
				"of a colon definition.\n", iname, lineno);
		wlen=get_word();
		emit_token("b(lit)");
		emit_num32(statbuf[0]);
		break;

	case CCHAR:
		wlen=get_word();
		emit_token("b(lit)");
		emit_num32(statbuf[0]);
		break;
		
	case UNTIL:
		emit_token("b?branch");
		emit_offset(dpop()-(unsigned long)opc);
		break;

	case WHILE:
		emit_token("b?branch");
		dpush((unsigned long)opc);
		emit_offset(0);
		break;
		
	case REPEAT:
		emit_token("bbranch");
		offs2=dpop();
		offs1=dpop();
		offs1-=(unsigned long)opc;
		emit_offset(offs1);
		
		emit_token("b(>resolve)");
		offs1=(unsigned long)opc;
		opc=(u8 *)offs2;
		emit_offset(offs1-offs2);
		opc=(u8 *)offs1;
		break;
		
	case THEN:
		emit_token("b(>resolve)");
		offs1=(unsigned long)opc;
		opc=(u8 *)dpop();
		offs2=offs1-(unsigned long)opc;
		emit_offset(offs2);
		opc=(u8 *)offs1;
		break;

	case TO:
		emit_token("b(to)");
		break;

	case FLOAD:
		{
			u8 *oldstart, *oldpc, *oldend;
			char *oldiname;
			int oldlineno;

			wlen=get_word();
			
			oldstart=start; oldpc=pc; oldend=end; 
			oldiname=iname; oldlineno=lineno;
			
			init_stream((char *)statbuf);
			tokenize();
			close_stream();
			
			iname=oldiname; lineno=oldlineno;
			end=oldend; pc=oldpc; start=oldstart;
		}
		break;

	case STRING:
		if (*pc++=='\n') lineno++;
		wlen=get_string();
		emit_token("b(\")");
		emit_string(statbuf,wlen-1);
		break;

	case PSTRING:
		if (*pc++=='\n') lineno++;
		wlen=get_string();
		emit_token("b(\")");
		emit_string(statbuf,wlen-1);
		emit_token("type");
		break;

	case PBSTRING:
		if (*pc++=='\n') lineno++;
		get_until(')');
		emit_token("b(\")");
		emit_string(statbuf,strlen((char *)statbuf)-1);
		emit_token("type");
		break;

	case SSTRING:
		if (*pc++=='\n') lineno++;
		get_until('"');
		emit_token("b(\")");
		emit_string(statbuf,strlen((char *)statbuf)-1);
		pc++; /* skip " */
		break;

	case HEXVAL:
		{
			u8 basecpy=base;
			long val;
			
			base=0x10;
			wlen=get_word();
			if (!get_number(&val)) {
				emit_token("b(lit)");
				emit_num32(val);
			} else {
				printf("%s:%d: warning: illegal value in h#"
						" ignored\n", iname, lineno);
			}
			base=basecpy;
		}
		break;
		
	case DECVAL:
		{
			u8 basecpy=base;
			long val;

			base=0x0a;
			wlen=get_word();
			if (!get_number(&val)) {
				emit_token("b(lit)");
				emit_num32(val);
			} else {
				printf("%s:%d: warning: illegal value in d#"
						" ignored\n", iname, lineno);
			}
			
			base=basecpy;
		}
		break;
		
	case OCTVAL:
		{
			u8 basecpy=base;
			long val;

			base=0x08;
			wlen=get_word();
			if (!get_number(&val)) {
				emit_token("b(lit)");
				emit_num32(val);
			} else {
				printf("%s:%d: warning: illegal value in o#"
						" ignored\n", iname, lineno);
			}

			base=basecpy;
		}
		break;
	case BEGINTOK:
		intok=TRUE;
		break;

	case EMITBYTE:
		if (intok)
			emit_byte(dpop());
		else 
			printf ("%s:%d: warning: emit-byte outside tokenizer"
					" scope\n", iname, lineno);
		break;

	case NEXTFCODE:
		if (intok)
			nextfcode=dpop();
		else
			printf("%s:%d: warning: next-fcode outside tokenizer"
					" scope\n", iname, lineno);
		break;
		
	case ENDTOK:
		intok=FALSE;
		break;
	
	case VERSION1:
	case FCODE_V1:
		printf("%s:%d: using version1 header\n", iname, lineno);
		emit_token("version1");
		fcode_hdr=opc;
		emit_fcodehdr();
		offs16=FALSE;
		break;
	
	case START1:
	case FCODE_V2:
	case FCODE_V3: /* Full IEEE 1275 */
		emit_token("start1");
		fcode_hdr=opc;
		emit_fcodehdr();
		offs16=TRUE;
		break;
		
	case START0:
		printf ("%s:%d: warning: spread of 0 not supported.", 
				iname, lineno);
		emit_token("start0");
		fcode_hdr=opc;
		emit_fcodehdr();
		offs16=TRUE;
		break;
		
	case START2:
		printf ("%s:%d: warning: spread of 2 not supported.", 
			iname, lineno);
		emit_token("start2");
		fcode_hdr=opc;
		emit_fcodehdr();
		offs16=TRUE;
		break;
		
	case START4:
		printf ("%s:%d: warning: spread of 4 not supported.", 
			iname, lineno);
		emit_token("start4");
		fcode_hdr=opc;
		emit_fcodehdr();
		offs16=TRUE;
		break;
		
	case END0:
	case FCODE_END:
		haveend=TRUE;
		emit_token("end0");
		break;

	case END1:
		haveend=TRUE;
		emit_token("end1");
		break;
		
	case RECURSIVE:
		break;

	case PCIHDR:
		{
			u32 classid=dpop();
			u16 did=dpop();
			u16 vid=dpop();
			
			pci_hdr=opc;
			emit_pcihdr(vid, did, classid);
			
			printf("%s:%d: PCI header vendor id=0x%04x, "
				"device id=0x%04x, class=%06x\n",
				iname, lineno, vid, did, classid);
		}
		break;
	
	case PCIEND:
		if (!pci_hdr) {
			printf("%s:%d: error: pci-header-end/pci-end "
				"without pci-header\n", iname, lineno);
			ERROR;
		}
		finish_pcihdr();
		break;

	case PCIREV:
		pci_revision=dpop();
		printf("%s:%d: PCI header revision=0x%04x\n", 
				iname, lineno, pci_revision);
		break;

	case NOTLAST:
		pci_is_last_image=FALSE;
		printf("%s:%d: PCI header not last image!\n",
				iname, lineno);
		break;
		
	case ABORTTXT:
		/* ABORT" is not to be used in FCODE drivers
		 * but Apple drivers do use it. Therefore we
		 * allow it. We push the specified string to
		 * the stack, do -2 THROW and hope that THROW
		 * will correctly unwind the stack.
		 */
		
		printf("%s:%d: warning: ABORT\" in fcode not defined by "
				"IEEE 1275-1994.\n", iname, lineno);
		if (*pc++=='\n') lineno++;
		wlen=get_string();
		
#ifdef BREAK_COMPLIANCE
		/* IF */
		emit_token("b?branch");
		
		dpush((unsigned long)opc);
		emit_offset(0);
#endif
		emit_token("b(\")");
		emit_string(statbuf,wlen-1);
#ifdef BREAK_COMPLIANCE
		emit_token("type");
#endif
		emit_token("-2");
		emit_token("throw");
#ifdef BREAK_COMPLIANCE
		/* THEN */
		emit_token("b(>resolve)");
		offs1=(unsigned long)opc;
		opc=(u8 *)dpop();
		offs2=offs1-(unsigned long)opc;
		emit_offset(offs2);
		opc=(u8 *)offs1;
#endif
		break;

	case FCODE_DATE:
		{
			time_t tt;
			char fcode_date[11];
			
			tt=time(NULL);
			strftime(fcode_date, 11, "%m/%d.%Y", 
					localtime(&tt));
			emit_token("b(\")");
			emit_string((u8 *)fcode_date,10);
		}	
		break;

	case FCODE_TIME:
		{
			time_t tt;
			char fcode_time[9];
			
			tt=time(NULL);
			strftime(fcode_time, 9, "%H:%M:%S", 
					localtime(&tt));
			emit_token("b(\")");
			emit_string((u8 *)fcode_time,8);
		}
		break;

	case ENCODEFILE:
		wlen=get_word();
		encode_file( (char*)statbuf );
		break;

	default:
		printf("%s:%d: error: Unimplemented control word '%s'\n",
				iname, lineno, statbuf);
		ERROR;
	}
}

void tokenize(void)
{
	unsigned long wlen;
	u16 tok;
	u8 *mac;
	long val;
	
	while ((wlen=get_word())!=0) {

		/* Filter comments */
		switch (statbuf[0]) {
		case '\\':
			pc-=wlen;
			get_until('\n');
			continue;
			
		case '(':
			/* only a ws encapsulated '(' is a stack comment */
			if (statbuf[1])
				break;
			
			pc-=wlen;
			get_until(')');
			if (*pc++=='\n') lineno++;
#ifdef DEBUG_SCANNER
			printf ("%s:%d: debug: stack diagram: %s)\n",
						iname, lineno, statbuf);
#endif
			continue;
		}

		/* Check whether a macro with given name exists */
		
		mac=(u8 *)lookup_macro((char *)statbuf);
		if(mac) {
			u8 *oldstart, *oldpc, *oldend;
#ifdef DEBUG_SCANNER
			printf("%s:%d: debug: macro %s folds out to sequence"
				" '%s'\n", iname, lineno, statbuf, mac);
#endif	
			oldstart=start; oldpc=pc; oldend=end;
			start=pc=end=mac;
			end+=strlen((char *)mac);
			
			tokenize();

			end=oldend; pc=oldpc; start=oldstart;
			
			continue;
		}

		/* Check whether it's a non-fcode forth construct */
		
		tok=lookup_fword((char *)statbuf);
		if (tok!=0xffff) {
#ifdef DEBUG_SCANNER
			printf("%s:%d: debug: matched internal opcode 0x%04x\n",
					iname, lineno, tok);
#endif
			handle_internal(tok);
#ifdef DEBUG_SCANNER
			if (verbose)
				printf("%s:%d: debug: '%s' done.\n",
					iname,lineno,statbuf);
#endif
			continue;
		}
		
		/* Check whether it's one of the defined fcode words */
		
		tok=lookup_token((char *)statbuf);
		if (tok!=0xffff) {
#ifdef DEBUG_SCANNER
			printf("%s:%d: debug: matched fcode no 0x%04x\n",
					iname, lineno, tok);
#endif
			emit_fcode(tok);
			continue;
		}
		
		/* It's not a word or macro - is it a number? */
		
		if (!get_number(&val)) {
			if (intok)
				dpush(val);
			else {
				emit_fcode(lookup_token("b(lit)"));
				emit_num32(val);
			}
			continue;
		}

		/* could not identify - bail out. */
		
		printf("%s:%d: error: word '%s' is not in dictionary.\n",
				iname, lineno, statbuf);
		ERROR;
	}
}

