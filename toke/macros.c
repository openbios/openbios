/*
 *                     OpenBIOS - free your system!
 *                         ( FCode tokenizer )
 *
 *  macros.c - macro initialization and functions.
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
#if defined(__linux__) && ! defined(__USE_BSD)
#define __USE_BSD
#endif
#include <string.h>
#include <errno.h>

#include "toke.h"

typedef struct macro {
	u8  *name;
	u8  *alias;
	struct macro *next;
} macro_t;

static macro_t *macros=NULL;

char *lookup_macro(char *name)
{
	macro_t *curr;
	
	for (curr=macros; curr!=NULL; curr=curr->next)
		if (!strcasecmp(name,(char *)curr->name))
			break;

	return curr?(char *)curr->alias:NULL;
}

int add_macro(char *name, char *alias)
{
	macro_t *curr;

	curr=malloc(sizeof(macro_t));
	if(!curr) {
		printf("Out of memory while adding macro.\n");
		exit(-ENOMEM);
	}

	curr->next=macros;
	curr->name=(u8 *)name;
	curr->alias=(u8 *)alias;

	macros=curr;
	return 0;
}

void init_macros(void) 
{
	add_macro( "eval",	"evaluate");
	add_macro( "(.)", 	"dup abs <# u#s swap sign u#>");
	add_macro( "<<", 	"lshift");
	add_macro( ">>",	"rshift");
	add_macro( "?",		"@ .");
	add_macro( "1+",	"1 +");
	add_macro( "1-",	"1 -");
	add_macro( "2+",	"2 +");
	add_macro( "2-",	"2 -");
	/* add_macro( "abort\"",	"-2 throw"); */
	add_macro( "accept",	"span @ -rot expect span @ swap span !");
	add_macro( "allot",	"0 max 0 ?do 0 c, loop");
	add_macro( "blank",	"bl fill");
	add_macro( "/c*",	"chars");
	add_macro( "ca1+",	"char+");
	add_macro( "carret",	"h# d");
	add_macro( ".d",	"base @ swap h# a base ! . base !");
	add_macro( "decode-bytes", ">r over r@ + swap r@ - rot r>");
	add_macro( "3drop",	"drop 2drop");
	add_macro( "3dup",	"2 pick 2 pick 2 pick");
	add_macro( "erase",	"0 fill");
	add_macro( "false",	"0");
	add_macro( ".h",	"base @ swap h# 10 base ! . base !");
	add_macro( "linefeed",	"h# a");
	add_macro( "/n*",	"cells");
	add_macro( "na1+",	"cell+");
	add_macro( "not",	"invert");
	add_macro( "s.",	"(.) type space");
	add_macro( "space",	"bl emit");
	add_macro( "spaces",	"0 max 0 ?do space loop");
	add_macro( "struct",	"0");
	add_macro( "true",	"-1");
	add_macro( "(u.)",	"<# u#s u#>");

	/* H.7 FCode name changes */
	add_macro( "decode-2int",		"parse-2int");
	add_macro( "delete-attribute",		"delete-property");
	add_macro( "get-inherited-attribute",	"get-inherited-property");
	add_macro( "get-my-attribute",		"get-my-property");
	add_macro( "get-package-attribute",	"get-package-property");
	add_macro( "attribute",	  "property");
	add_macro( "flip",	  "wbflip");
	add_macro( "is",	  "to");
	add_macro( "lflips",	  "lwflips");
	add_macro( "map-sbus",	  "map-low");
	add_macro( "u*x",	  "um*");
	add_macro( "version",	  "fcode-revision");
	add_macro( "wflips",	  "wbflips");
	add_macro( "x+",	  "d+");
	add_macro( "x-",	  "d-");
	add_macro( "xdr+",	  "encode+");
	add_macro( "xdrbytes",	  "encode-bytes");
	add_macro( "xdrint",	  "encode-int");
	add_macro( "xdrphys",	  "encode-phys");
	add_macro( "xdrstring",	  "encode-string");
	add_macro( "xdrtoint",	  "decode-int");
	add_macro( "xdrtostring", "decode-string");
	add_macro( "xu/mod",	  "um/mod");
	
	/* non standard but often used macros */
	add_macro( "name",	"device-name");
	add_macro( "endif",	"then");
}
